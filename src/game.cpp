#include "gwent/game.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
#include <limits>
#include <map>

namespace gwent {
namespace {
bool validRow(Row row) { return row >= Row::Melee && row <= Row::Siege; }
int rowIndex(Row row) { return static_cast<int>(row); }
std::uint32_t nextRandom(Game& g) {
  // xorshift32 gives reproducible, serializable game randomness without a dependency.
  auto x = static_cast<std::uint32_t>(g.rngState ? g.rngState : 1);
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  g.rngState = x;
  return x;
}
auto findCard(std::vector<Card>& cards, const std::string& id) {
  return std::find_if(cards.begin(), cards.end(), [&](const Card& c) { return c.id == id; });
}
bool hasAbility(const Card& card, const std::string& ability) {
  return std::find(card.abilities.begin(), card.abilities.end(), ability) != card.abilities.end();
}
void shuffleDeck(Game& g, std::vector<Card>& deck) {
  // Deck.initialize() builds the reference deck by inserting each card at a
  // random position. Keep that order and RNG consumption instead of applying
  // a conventional Fisher-Yates shuffle after the entire vector is built.
  std::vector<Card> source=std::move(deck);
  deck.clear();
  deck.reserve(source.size());
  for(auto& card:source) {
    deck.push_back(std::move(card));
    const std::size_t index=nextRandom(g)%deck.size();
    std::swap(deck[index],deck.back());
  }
}
void addCardRandom(Game& g,std::vector<Card>& deck,const Card& card) {
  deck.push_back(card);
  const std::size_t index=nextRandom(g)%deck.size();
  std::swap(deck[index],deck.back());
}
int cardPower(const Card& c, const std::vector<Card>& row, bool weather, bool horn,
              bool halfWeather = false, bool doubleSpyPower = false) {
  if (c.hero) return c.power; // Heroes ignore all row effects.
  const int weatherFloor = halfWeather ? c.power / 2 : 1;
  int power = weather ? std::min(c.power, weatherFloor) : c.power;
  if (doubleSpyPower && c.spy) power *= 2;
  // Source Row.updateState() counts Bond-bearing cards by printed name;
  // Row.calcCardScore() multiplies every card with that name by that count.
  const auto bondCount = std::count_if(row.begin(), row.end(), [&](const Card& x) {
    return x.name == c.name && (x.bond || hasAbility(x, "bond"));
  });
  if (bondCount > 1) power *= static_cast<int>(bondCount);
  const int moraleCount = static_cast<int>(std::count_if(row.begin(), row.end(), [](const Card& x) { return x.morale || hasAbility(x, "morale"); }));
  power += std::max(0, moraleCount - ((c.morale || hasAbility(c, "morale")) ? 1 : 0));
  const int hornSources = static_cast<int>(horn) + static_cast<int>(std::count_if(row.begin(), row.end(), [](const Card& x) { return hasAbility(x, "horn"); }));
  if (hornSources > (hasAbility(c, "horn") ? 1 : 0)) power *= 2;
  return power;
}
void recompute(Game& g) {
  for (int owner = 0; owner < 2; ++owner) g.players[owner].score = calculateScore(g.players[owner], g.weather, g.horn[owner]);
}
bool isCombatUnit(const Card& card) {
  return card.kind==Kind::Unit && !card.hero && (validRow(card.row) || card.agile);
}
int agileTargetRow(Game& g,int owner,const Card& card) {
  auto score=[&](int row) {
    Player simulated=g.players[owner];
    simulated.board[row].push_back(card);
    return calculateScore(simulated,g.weather,g.horn[owner])-g.players[owner].score;
  };
  const int ranged=score(1),melee=score(0);
  if(ranged>melee)return 1;
  if(melee>ranged)return 0;
  return static_cast<int>(nextRandom(g)%2);
}
void applyWeather(Game& g, int owner, const Card& card, Row fallback = Row::None) {
  // The reference weather container permits one active copy of each weather
  // card. Further copies are discarded without changing the active effect.
  if(std::any_of(g.weatherCards.begin(),g.weatherCards.end(),[&](const ActiveWeather& active){
       return active.card.name==card.name;
     })) {
    g.players[owner].graveyard.push_back(card);
    return;
  }
  bool typed=false;
  if (hasAbility(card, "frost")) { g.weather[0] = true; typed=true; }
  if (hasAbility(card, "fog")) { g.weather[1] = true; typed=true; }
  if (hasAbility(card, "rain")) { g.weather[2] = true; typed=true; }
  if (hasAbility(card, "storm")) { g.weather[1] = true; g.weather[2] = true; typed=true; }
  if (!typed && validRow(fallback)) g.weather[rowIndex(fallback)] = true;
  g.weatherCards.push_back({card, owner});
}
void clearWeather(Game& g) {
  g.weather.fill(false);
  for (const auto& active : g.weatherCards) g.players[active.owner].graveyard.push_back(active.card);
  g.weatherCards.clear();
}
void triggerAvenger(Game& g, int owner, const Card& removed) {
  const std::string summonName=hasAbility(removed,"avenger")?"Bovine Defense Force":
                               hasAbility(removed,"avenger_kambi")?"Hemdall":"";
  if(summonName.empty()) return;
  const auto& catalog=referenceCardCatalog();
  const auto summoned=std::find_if(catalog.begin(),catalog.end(),[&](const Card& c){return c.name==summonName;});
  if(summoned==catalog.end()) return;
  Card unit=*summoned;
  unit.id="summoned_"+removed.id;
  g.players[owner].board[rowIndex(unit.row)].push_back(std::move(unit));
}
void removeBoardCard(Game& g, int owner, int row, int index) {
  auto& cards=g.players[owner].board[row];
  Card removed=cards[index];
  cards.erase(cards.begin()+index);
  // Avenger tokens (Bovine Defense Force / Hemdall) schedule their own
  // removal from the graveyard 1001 ms after they are destroyed in the
  // reference game. The native rules engine resolves actions atomically, so
  // omit these generated-only cards from the graveyard rather than exposing
  // a token that Medic could revive on a later turn.
  const bool temporaryAvengerToken=removed.id.rfind("summoned_",0)==0;
  if(!temporaryAvengerToken)g.players[owner].graveyard.push_back(removed);
  triggerAvenger(g,owner,removed);
}
void finishRound(Game& g) {
  recompute(g);
  int winner = -1;
  if (g.players[0].score > g.players[1].score) winner = 0;
  if (g.players[1].score > g.players[0].score) winner = 1;
  if (winner < 0 && g.players[0].score == g.players[1].score) {
    if (g.players[0].faction == Faction::Nilfgaard && g.players[1].faction != Faction::Nilfgaard) winner = 0;
    else if (g.players[1].faction == Faction::Nilfgaard && g.players[0].faction != Faction::Nilfgaard) winner = 1;
  }
  if (winner >= 0) {
    ++g.players[winner].roundWins;
    ++g.players[1 - winner].roundLosses;
  } else {
    // A draw costs both players a gem in the Witcher 3 minigame.
    ++g.players[0].roundLosses;
    ++g.players[1].roundLosses;
  }
  if (g.players[0].roundLosses >= 2 || g.players[1].roundLosses >= 2) {
    g.phase = Phase::MatchResult;
    if (g.players[0].roundLosses == g.players[1].roundLosses) g.matchWinner = 2;
    else g.matchWinner = g.players[0].roundLosses >= 2 ? 1 : 0;
  } else g.phase = Phase::RoundResult;
}
void endTurn(Game& g) {
  const int acted = g.currentPlayer;
  if (g.players[0].passed && g.players[1].passed) { finishRound(g); return; }
  const int other = 1 - acted;
  // Once one side has passed, the other side keeps taking turns until it passes.
  g.currentPlayer = g.players[other].passed ? acted : other;
  g.phase = g.currentPlayer == 0 ? Phase::PlayerTurn : Phase::OpponentTurn;
  recompute(g);
}
bool canAct(const Player& player) {
  return !player.hand.empty() || player.leaderAbilityAvailable;
}
void startRoundTurn(Game& g) {
  for(auto& player:g.players)if(!canAct(player))player.passed=true;
  if(g.players[0].passed&&g.players[1].passed){finishRound(g);return;}
  if(g.players[g.currentPlayer].passed)g.currentPlayer=1-g.currentPlayer;
  g.phase=g.currentPlayer==0?Phase::PlayerTurn:Phase::OpponentTurn;
}
void finishActionTurn(Game& g) {
  auto& player=g.players[g.currentPlayer];
  if(!player.passed&&!canAct(player))player.passed=true;
  endTurn(g);
}
void applyMuster(Player& player, const Card& played) {
  if ((!played.muster && !hasAbility(played, "muster")) || played.group.empty()) return;
  const auto matchesMusterGroup = [&](const Card& candidate) {
    if (candidate.group == played.group) return true;
    if (candidate.name == played.group) return true;
    return candidate.name.rfind(played.group + " - ", 0) == 0;
  };
  auto deploy = [&](std::vector<Card>& source) {
    for (auto it = source.begin(); it != source.end();) {
      if (matchesMusterGroup(*it)) {
        const int r = rowIndex(it->row);
        if (r >= 0 && r < 3) player.board[r].push_back(*it);
        it = source.erase(it);
      } else ++it;
    }
  };
  deploy(player.hand);
  deploy(player.deck);
}
void scorchEnemyRow(Game& g, int owner, int rowIndexValue) {
  auto& row = g.players[1 - owner].board[rowIndexValue];
  int rowTotal = 0, strongest = -1;
  for (const auto& unit : row) {
    if (unit.kind != Kind::Unit) continue;
    const int power = cardPower(unit, row, g.weather[rowIndexValue], g.horn[1 - owner][rowIndexValue],
                                g.players[1-owner].halfWeather, g.players[1-owner].doubleSpyPower);
    rowTotal += power;
    if (!unit.hero) strongest = std::max(strongest, power);
  }
  if (rowTotal < 10 || strongest < 0) return;
  std::vector<int> remove;
  for (int i = 0; i < static_cast<int>(row.size()); ++i)
    if (row[i].kind == Kind::Unit && !row[i].hero &&
        cardPower(row[i], row, g.weather[rowIndexValue], g.horn[1 - owner][rowIndexValue],
                  g.players[1-owner].halfWeather, g.players[1-owner].doubleSpyPower) == strongest)
      remove.push_back(i);
  for (auto i = remove.rbegin(); i != remove.rend(); ++i)
    removeBoardCard(g,1-owner,rowIndexValue,*i);
}
void transformBerserkers(Player& player, int rowNumber) {
  if (rowNumber < 0 || rowNumber > 2) return;
  auto& row = player.board[rowNumber];
  for (auto& card : row) {
    if (!card.berserker && !hasAbility(card, "berserker")) continue;
    const std::string targetName = card.name == "Young Berserker" ? "Transformed Young Vildkaarl" : "Transformed Vildkaarl";
    const auto& catalog = referenceCardCatalog();
    auto target = std::find_if(catalog.begin(), catalog.end(), [&](const Card& candidate) {
      return candidate.name == targetName;
    });
    if (target == catalog.end()) continue;
    const std::string oldId = card.id;
    card = *target;
    card.id = oldId;
  }
}
}

int calculateScore(const Player& p, const std::array<bool, 3>& weather,
                   const std::array<bool, 3>& horn) {
  int total = 0;
  for (int r = 0; r < 3; ++r)
    for (const auto& c : p.board[r])
      if (c.kind == Kind::Unit) total += cardPower(c, p.board[r], weather[r], horn[r], p.halfWeather, p.doubleSpyPower);
  return total;
}

int calculateCardPower(const Card& card, const std::vector<Card>& row, bool weather, bool horn,
                       bool halfWeather, bool doubleSpyPower) {
  return cardPower(card,row,weather,horn,halfWeather,doubleSpyPower);
}

namespace {
void beginMatchImpl(Game& g,std::vector<Card> playerDeck,Faction playerFaction,
                    std::vector<Card> opponentDeck,Faction opponentFaction,
                    const Card* playerLeader,const Card* opponentLeader,
                    std::uint64_t seed,int firstPlayer) {
  g = Game{};
  g.rngState = seed ? seed : 1;
  g.firstPlayer = (firstPlayer == 0 || firstPlayer == 1) ? firstPlayer : 0;
  for (int i = 0; i < 2; ++i) {
    auto& p = g.players[i];
    p.deck = i == 0 ? std::move(playerDeck) : std::move(opponentDeck);
    p.faction = i == 0 ? playerFaction : opponentFaction;
    if(i==0&&playerLeader)p.leader=*playerLeader;
    if(i==1&&opponentLeader)p.leader=*opponentLeader;
    shuffleDeck(g, p.deck);
  }

  // Game.startGame() runs initPlayers and gameStart faction/leader effects
  // before the coin toss and before the alternating ten-card deal. In
  // particular Francesca Daisy draws the first card before the opening deal.
  g.phase=Phase::Mulligan;
  resolveLeaderPassives(g);
  const bool scoiataelChooses=(playerFaction==Faction::Scoiatael)!=(opponentFaction==Faction::Scoiatael);
  if(firstPlayer!=0&&firstPlayer!=1&&!scoiataelChooses)
    g.firstPlayer=static_cast<int>(nextRandom(g)%2);
  for(int n=0;n<10;++n)for(int i=0;i<2;++i) {
    auto& deck=g.players[i].deck;
    if(deck.empty())continue;
    g.players[i].hand.push_back(std::move(deck.front()));
    deck.erase(deck.begin());
  }
  g.currentPlayer = g.firstPlayer;
}
}

void beginMatch(Game& g, std::vector<Card> playerDeck, Faction playerFaction,
                std::vector<Card> opponentDeck, Faction opponentFaction,
                std::uint64_t seed, int firstPlayer) {
  beginMatchImpl(g,std::move(playerDeck),playerFaction,std::move(opponentDeck),opponentFaction,
                 nullptr,nullptr,seed,firstPlayer);
}

bool replaceMulliganCards(Game& g, int who, const std::vector<std::string>& ids) {
  if (g.phase != Phase::Mulligan || g.scoiataelChoicePending || who < 0 || who > 1 || g.players[who].mulliganDone || ids.size() > 2)
    return false;
  auto& player = g.players[who];
  std::vector<Card> replacements;
  for (const auto& id : ids) {
    if (std::find_if(replacements.begin(), replacements.end(), [&](const Card& c) { return c.id == id; }) != replacements.end())
      return false;
    auto card = findCard(player.hand, id);
    if (card == player.hand.end()) return false;
    replacements.push_back(*card);
  }
  // Match Deck.swap(): return one selected card through Deck.addCardRandom(),
  // draw the deck's front card, and put it back at the same hand index before
  // processing the next selection.
  for (const auto& selected : replacements) {
    auto found=findCard(player.hand,selected.id);
    const auto handIndex=static_cast<std::size_t>(found-player.hand.begin());
    Card discarded=*found;
    player.hand.erase(found);
    addCardRandom(g,player.deck,discarded);
    Card drawn=std::move(player.deck.front());
    player.deck.erase(player.deck.begin());
    player.hand.insert(player.hand.begin()+static_cast<std::ptrdiff_t>(handIndex),std::move(drawn));
  }
  player.mulliganDone = true;
  if (g.players[0].mulliganDone && g.players[1].mulliganDone) {
    g.currentPlayer = g.firstPlayer;
    startRoundTurn(g);
  }
  return true;
}

void resolveLeaderPassives(Game& g) {
  const bool whiteFlame = hasAbility(g.players[0].leader, "emhyr_whiteflame") ||
                          hasAbility(g.players[1].leader, "emhyr_whiteflame");
  for (auto& player : g.players) player.leaderAbilityCancelled = whiteFlame;
  const std::vector<std::string> passiveIds={"emhyr_whiteflame","emhyr_invader","eredin_treacherous","francesca_daisy","king_bran"};
  for (int who = 0; who < 2; ++who) {
    auto& player=g.players[who];
    if (player.leader.kind != Kind::Leader) continue;
    const bool passive=std::any_of(passiveIds.begin(),passiveIds.end(),[&](const std::string& id){return hasAbility(player.leader,id);});
    player.leaderAbilityAvailable=!passive && !player.leaderAbilityCancelled;
    if (player.leaderPassivesApplied) continue;
    player.leaderPassivesApplied=true;
    if (whiteFlame) continue;
    if (hasAbility(player.leader,"emhyr_invader")) g.randomRespawn=true;
    if (hasAbility(player.leader,"eredin_treacherous"))
      for (auto& affected : g.players) affected.doubleSpyPower=true;
    if (hasAbility(player.leader,"king_bran")) player.halfWeather=true;
    if (hasAbility(player.leader,"francesca_daisy") && !player.deck.empty()) {
      player.hand.push_back(std::move(player.deck.front())); player.deck.erase(player.deck.begin());
    }
  }
  if(!g.factionAbilitiesInitialized) {
    g.factionAbilitiesInitialized=true;
    const auto& me=g.players[0];
    const auto& opponent=g.players[1];
    // The reference skips faction setup when both decks are Scoia'tael.
    if(me.faction!=Faction::Scoiatael || opponent.faction!=Faction::Scoiatael) {
      if(me.faction==Faction::Scoiatael) {
        g.firstPlayer=0;
        g.scoiataelChoicePending=true;
      } else if(opponent.faction==Faction::Scoiatael) {
        g.firstPlayer=(nextRandom(g)%2==0)?1:0;
      }
    }
  }
}

bool chooseScoiataelFirstPlayer(Game& g,int firstPlayer) {
  if(g.phase!=Phase::Mulligan || !g.scoiataelChoicePending || (firstPlayer!=0 && firstPlayer!=1))return false;
  g.firstPlayer=firstPlayer;
  g.scoiataelChoicePending=false;
  return true;
}

bool assignLeader(Game& g, int who, const Card& leader) {
  if (who < 0 || who > 1 || g.phase != Phase::Mulligan || leader.kind != Kind::Leader) return false;
  g.players[who].leader=leader;
  g.players[who].faction=leader.faction;
  if (g.players[0].leader.kind==Kind::Leader && g.players[1].leader.kind==Kind::Leader)
    resolveLeaderPassives(g);
  return true;
}

void beginMatchWithLeaders(Game& g, std::vector<Card> playerDeck, Card playerLeader,
                           std::vector<Card> opponentDeck, Card opponentLeader,
                           std::uint64_t seed, int firstPlayer) {
  beginMatchImpl(g,std::move(playerDeck),playerLeader.faction,std::move(opponentDeck),opponentLeader.faction,
                 &playerLeader,&opponentLeader,seed,firstPlayer);
}

bool playCard(Game& g, int who, const std::string& id, Row target, const std::string& selectedId) {
  if (who < 0 || who > 1 || who != g.currentPlayer || g.players[who].passed ||
      (g.phase != Phase::PlayerTurn && g.phase != Phase::OpponentTurn)) return false;
  auto& p = g.players[who];
  auto it = findCard(p.hand, id);
  if (it == p.hand.end()) return false;
  Card c = *it;
  if (c.kind == Kind::Leader) return false;
  Row deployRow = c.row;
  if (c.agile && (target == Row::Melee || target == Row::Ranged)) deployRow = target;
  if (c.kind == Kind::Unit && !validRow(deployRow)) return false;
  if (c.kind == Kind::CommanderHorn && !validRow(target)) return false;
  if (c.kind == Kind::CommanderHorn && p.specialOccupied[rowIndex(target)]) return false;
  if (c.kind == Kind::Mardroeme && (!validRow(target) || p.specialOccupied[rowIndex(target)])) return false;
  const bool typedWeather = hasAbility(c, "frost") || hasAbility(c, "fog") || hasAbility(c, "rain") || hasAbility(c, "storm");
  if (c.kind == Kind::Weather && !typedWeather && !validRow(target)) return false;

  auto boardTarget = p.board[0].end();
  int targetRow = -1;
  if (c.kind == Kind::Decoy) {
    for (int r = 0; r < 3; ++r) {
      auto found = findCard(p.board[r], selectedId);
      if (found != p.board[r].end()) { boardTarget = found; targetRow = r; break; }
    }
    if (selectedId.empty() || targetRow < 0 || !isCombatUnit(*boardTarget)) return false;
  }
  int medicIndex = -1;
  int medicReviveRow = -1;
  if (c.kind == Kind::Unit && c.medic) {
    if (g.randomRespawn) {
      std::vector<int> units;
      for (int i=0;i<static_cast<int>(p.graveyard.size());++i)
        if (isCombatUnit(p.graveyard[i])) units.push_back(i);
      if (!units.empty()) medicIndex=units[nextRandom(g)%units.size()];
    } else if (!selectedId.empty()) {
      const auto found=findCard(p.graveyard,selectedId);
      if(found==p.graveyard.end() || !isCombatUnit(*found)) return false;
      medicIndex=static_cast<int>(found-p.graveyard.begin());
    } else if(std::any_of(p.graveyard.begin(),p.graveyard.end(),isCombatUnit)) return false;
    if(medicIndex>=0 && p.graveyard[static_cast<std::size_t>(medicIndex)].agile) {
      if(target!=Row::Melee && target!=Row::Ranged)return false;
      medicReviveRow=rowIndex(target);
    }
  }

  p.hand.erase(it);
  switch (c.kind) {
    case Kind::Weather:
      applyWeather(g, who, c, target); break;
    case Kind::ClearWeather:
      clearWeather(g); p.graveyard.push_back(c); break;
    case Kind::Mardroeme:
      p.specialOccupied[rowIndex(target)]=true; p.mardroeme[rowIndex(target)]=true;
      transformBerserkers(p, rowIndex(target)); p.graveyard.push_back(c); break;
    case Kind::Scorch: {
      int strongest = -1;
      std::array<std::array<std::vector<int>, 3>, 2> remove;
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r)
        for (int n = 0; n < static_cast<int>(g.players[owner].board[r].size()); ++n) {
          const auto& unit = g.players[owner].board[r][n];
          const int power = unit.kind == Kind::Unit ? cardPower(unit, g.players[owner].board[r], g.weather[r], g.horn[owner][r], g.players[owner].halfWeather, g.players[owner].doubleSpyPower) : -1;
          if (!unit.hero) strongest = std::max(strongest, power);
        }
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r)
        for (int n = 0; n < static_cast<int>(g.players[owner].board[r].size()); ++n) {
          const auto& unit = g.players[owner].board[r][n];
          if (unit.kind == Kind::Unit && !unit.hero && cardPower(unit, g.players[owner].board[r], g.weather[r], g.horn[owner][r], g.players[owner].halfWeather, g.players[owner].doubleSpyPower) == strongest)
            remove[owner][r].push_back(n);
        }
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r) {
        for (auto n = remove[owner][r].rbegin(); n != remove[owner][r].rend(); ++n)
          removeBoardCard(g,owner,r,*n);
      }
      p.graveyard.push_back(c); break;
    }
    case Kind::CommanderHorn:
      p.specialOccupied[rowIndex(target)]=true; g.horn[who][rowIndex(target)] = true; p.graveyard.push_back(c); break;
    case Kind::Decoy: {
      const Card returned=*boardTarget;
      p.hand.push_back(returned);
      *boardTarget = c; // The zero-strength Decoy stays in the replaced unit's battlefield slot.
      triggerAvenger(g,who,returned);
      break;
    }
    case Kind::Unit:
      if (c.spy) {
        g.players[1 - who].board[rowIndex(deployRow)].push_back(c);
        for (int i = 0; i < 2 && !p.deck.empty(); ++i) { p.hand.push_back(std::move(p.deck.front())); p.deck.erase(p.deck.begin()); }
      } else p.board[rowIndex(deployRow)].push_back(c);
      if (c.muster) {
        applyMuster(p, c);
        for(int row=0;row<3;++row)if(p.mardroeme[row])transformBerserkers(p,row);
      }
      if (medicIndex >= 0) {
        const Card revived = p.graveyard[medicIndex];
        p.graveyard.erase(p.graveyard.begin()+medicIndex);
        int reviveRow = revived.agile ? medicReviveRow : rowIndex(revived.row);
        if (revived.agile && reviveRow < 0) reviveRow = 0;
        if (reviveRow >= 0 && reviveRow < 3) p.board[reviveRow].push_back(revived);
      }
      if (hasAbility(c, "scorch_c")) scorchEnemyRow(g, who, 0);
      if (hasAbility(c, "scorch_r")) scorchEnemyRow(g, who, 1);
      if (hasAbility(c, "scorch_s")) scorchEnemyRow(g, who, 2);
      if (hasAbility(c, "mardroeme")) {
        p.mardroeme[rowIndex(deployRow)]=true;
        transformBerserkers(p, rowIndex(deployRow));
      }
      if (g.players[who].mardroeme[rowIndex(deployRow)]) transformBerserkers(g.players[who], rowIndex(deployRow));
      break;
    case Kind::Leader:
      p.hand.push_back(c);
      return false;
  }
  finishActionTurn(g);
  return true;
}

bool pass(Game& g, int who) {
  if (who < 0 || who > 1 || who != g.currentPlayer ||
      (g.phase != Phase::PlayerTurn && g.phase != Phase::OpponentTurn) || g.players[who].passed) return false;
  g.players[who].passed = true;
  endTurn(g);
  return true;
}

bool activateLeader(Game& g, int who, Row target, const std::vector<std::string>& selectedIds) {
  if (who<0 || who>1 || who!=g.currentPlayer || g.players[who].passed ||
      (g.phase!=Phase::PlayerTurn && g.phase!=Phase::OpponentTurn)) return false;
  auto& player=g.players[who];
  if (!player.leaderAbilityAvailable || player.leaderAbilityCancelled) return false;
  const auto ability=std::find_if(player.leader.abilities.begin(),player.leader.abilities.end(),[](const std::string& id){
    return id.rfind("foltest_",0)==0 || id.rfind("emhyr_",0)==0 || id.rfind("eredin_",0)==0 ||
           id.rfind("francesca_",0)==0 || id=="crach_an_craite" || id=="king_bran";
  });
  if (ability==player.leader.abilities.end()) return false;
  const std::string& id=*ability;
  const std::string selected=selectedIds.empty()?std::string{}:selectedIds.front();
  bool canActivate=true;
  auto playWeatherFromDeck=[&](const std::string& weatherName, bool anyWeather) {
    auto found=player.deck.end();
    if (!selected.empty()) found=findCard(player.deck,selected);
    else found=std::find_if(player.deck.begin(),player.deck.end(),[&](const Card& c){
      const bool weather=c.kind==Kind::Weather;
      return weather && (anyWeather || c.name==weatherName);
    });
    if(found==player.deck.end() || found->kind!=Kind::Weather || (!anyWeather && found->name!=weatherName)) return false;
    Card weather=*found; player.deck.erase(found); applyWeather(g,who,weather); return true;
  };
  if (id=="foltest_king") playWeatherFromDeck("Impenetrable Fog",false);
  else if (id=="foltest_lord") clearWeather(g);
  else if (id=="foltest_siegemaster") { if(player.specialOccupied[2]) canActivate=false; else {player.specialOccupied[2]=true;g.horn[who][2]=true;} }
  else if (id=="foltest_steelforged") scorchEnemyRow(g,who,2);
  else if (id=="foltest_son") scorchEnemyRow(g,who,1);
  else if (id=="emhyr_imperial") playWeatherFromDeck("Torrential Rain",false);
  else if (id=="emhyr_emperor") {
    auto& known=g.revealedCards[who]; known.clear();
    std::vector<int> indices(g.players[1-who].hand.size());
    for(int i=0;i<static_cast<int>(indices.size());++i) indices[i]=i;
    for(int i=0;i<3 && !indices.empty();++i) {
      const int pick=static_cast<int>(nextRandom(g)%indices.size());
      known.push_back(g.players[1-who].hand[indices[pick]]);
      indices.erase(indices.begin()+pick);
    }
  }
  else if (id=="emhyr_relentless") {
    auto& grave=g.players[1-who].graveyard;
    auto found=selected.empty()?grave.end():findCard(grave,selected);
    if(found==grave.end() || !isCombatUnit(*found)) canActivate=false;
    else { player.hand.push_back(*found); grave.erase(found); }
  }
  else if (id=="emhyr_whiteflame" || id=="emhyr_invader" || id=="eredin_treacherous" || id=="francesca_daisy" || id=="king_bran") canActivate=false;
  else if (id=="eredin_commander") { if(player.specialOccupied[0]) canActivate=false; else {player.specialOccupied[0]=true;g.horn[who][0]=true;} }
  else if (id=="eredin_bringer_of_death") {
    auto& grave=player.graveyard;
    auto found=selected.empty()?grave.end():findCard(grave,selected);
    if(found==grave.end() || !isCombatUnit(*found)) canActivate=false;
    else { player.hand.push_back(*found); grave.erase(found); }
  }
  else if (id=="eredin_destroyer") {
    if(player.hand.size()<2 || player.deck.empty()) canActivate=false;
    else {
      std::vector<std::string> discardIds;
      if(selectedIds.size()>=3) discardIds={selectedIds[0],selectedIds[1]};
      else canActivate=false;
      if(canActivate) {
        auto first=findCard(player.hand,discardIds[0]); auto second=findCard(player.hand,discardIds[1]);
        if(first==player.hand.end() || second==player.hand.end() || first==second) canActivate=false;
        else {
          auto draw=findCard(player.deck,selectedIds[2]);
          if(draw==player.deck.end()) canActivate=false;
          else {
            Card a=*first, b=*second;
            Card drawn=*draw;
            player.hand.erase(std::remove_if(player.hand.begin(),player.hand.end(),[&](const Card& c){return c.id==a.id||c.id==b.id;}),player.hand.end());
            player.graveyard.push_back(a); player.graveyard.push_back(b);
            player.hand.push_back(std::move(drawn)); player.deck.erase(draw);
          }
        }
      }
    }
  }
  else if (id=="eredin_king") {
    if(selected.empty())canActivate=false;
    else canActivate=playWeatherFromDeck("",true);
  }
  else if (id=="eredin_treacherous") canActivate=false;
  else if (id=="francesca_queen") scorchEnemyRow(g,who,0);
  else if (id=="francesca_beautiful") { if(player.specialOccupied[1]) canActivate=false; else {player.specialOccupied[1]=true;g.horn[who][1]=true;} }
  else if (id=="francesca_pureblood") playWeatherFromDeck("Biting Frost",false);
  else if (id=="francesca_hope") {
    std::vector<Card> agile;
    for(int r=0;r<2;++r) for(const auto& c:player.board[r]) if(c.agile) agile.push_back(c);
    if(agile.size()>20) canActivate=false;
    else if(!agile.empty()) {
      std::array<std::vector<Card>,2> base;
      for(int r=0;r<2;++r) for(const auto& c:player.board[r]) if(!c.agile) base[r].push_back(c);
      int bestScore=std::numeric_limits<int>::min();
      std::array<std::vector<Card>,2> bestRows=base;
      const std::uint64_t arrangements=std::uint64_t{1}<<agile.size();
      for(std::uint64_t mask=0;mask<arrangements;++mask) {
        Player virtualPlayer=player; virtualPlayer.board[0]=base[0]; virtualPlayer.board[1]=base[1];
        for(std::size_t n=0;n<agile.size();++n) virtualPlayer.board[(mask>>n)&1].push_back(agile[n]);
        const int score=calculateScore(virtualPlayer,g.weather,g.horn[who]);
        if(score>bestScore) { bestScore=score; bestRows={virtualPlayer.board[0],virtualPlayer.board[1]}; }
      }
      player.board[0]=std::move(bestRows[0]); player.board[1]=std::move(bestRows[1]);
    }
  }
  else if (id=="francesca_daisy" || id=="king_bran") canActivate=false;
  else if (id=="crach_an_craite") {
    // Source Board.toDeck() calls Deck.addCardRandom() for each grave card.
    // This leaves the deck untouched when a grave is empty and randomizes
    // each returned card as it is added instead of reshuffling the whole deck.
    for(auto& p:g.players) {
      for(const auto& card:p.graveyard)addCardRandom(g,p.deck,card);
      p.graveyard.clear();
    }
  } else canActivate=false;

  if (!canActivate) return false;
  player.leaderAbilityAvailable=false;
  finishActionTurn(g);
  return true;
}

bool startNextRound(Game& g) {
  if (g.phase != Phase::RoundResult) return false;
  const int previousStarter = g.firstPlayer;
  int winner = -1;
  if (g.players[0].score > g.players[1].score) winner = 0;
  if (g.players[1].score > g.players[0].score) winner = 1;
  if (winner < 0 && g.players[0].faction == Faction::Nilfgaard && g.players[1].faction != Faction::Nilfgaard) winner = 0;
  if (winner < 0 && g.players[1].faction == Faction::Nilfgaard && g.players[0].faction != Faction::Nilfgaard) winner = 1;
  g.firstPlayer = winner >= 0 ? winner : previousStarter;
  ++g.round;
  clearWeather(g);
  g.weather.fill(false);
  for (auto& ownerHorn : g.horn) ownerHorn.fill(false);
  for (int owner = 0; owner < 2; ++owner) {
    auto& player = g.players[owner];
    player.passed = false;
    player.mardroeme.fill(false);
    player.specialOccupied.fill(false);
    // Monsters keep one random unit from their side; all other units go to their owner's graveyard.
    int keepRow = -1, keepIndex = -1;
    if (player.faction == Faction::Monsters) {
      int count = 0;
      for (int r = 0; r < 3; ++r) for (const auto& unit : player.board[r])
        if (unit.kind == Kind::Unit && !unit.hero) ++count;
      if (count) {
        int pick = static_cast<int>(nextRandom(g) % static_cast<std::uint32_t>(count));
        for (int r = 0; r < 3 && keepRow < 0; ++r) for (int n = 0; n < static_cast<int>(player.board[r].size()); ++n)
          if (player.board[r][n].kind == Kind::Unit && !player.board[r][n].hero && pick-- == 0) { keepRow = r; keepIndex = n; break; }
      }
    }
    for (int r = 0; r < 3; ++r) {
      for (int n = static_cast<int>(player.board[r].size()) - 1; n >= 0; --n) {
        if (r == keepRow && n == keepIndex) continue;
        removeBoardCard(g,owner,r,n);
      }
    }
    if (winner >= 0 && owner == winner && player.faction == Faction::NorthernRealms && !player.deck.empty()) {
      player.hand.push_back(std::move(player.deck.front())); player.deck.erase(player.deck.begin());
    }
  }
  g.skelligeRevivesRemaining=0;
  g.skelligeRevivePlayer=-1;
  g.skelligeAgileReviveIds.clear();
  if (g.round == 3) for (int owner=0;owner<2;++owner) {
    auto& player=g.players[owner];
    if(player.faction!=Faction::Skellige)continue;
    int available=static_cast<int>(std::count_if(player.graveyard.begin(),player.graveyard.end(),isCombatUnit));
    const int count=std::min(2,available);
    std::vector<Card> selected;
    for(int revived=0;revived<count;++revived) {
      std::vector<int> units;
      for(int i=0;i<static_cast<int>(player.graveyard.size());++i)
        if(isCombatUnit(player.graveyard[i]))units.push_back(i);
      const int chosen=units[nextRandom(g)%units.size()];
      Card unit=player.graveyard[chosen]; player.graveyard.erase(player.graveyard.begin()+chosen);
      selected.push_back(std::move(unit));
    }
    for(auto& unit:selected) {
      if(owner==0 && unit.agile) {
        g.skelligeAgileReviveIds.push_back(unit.id);
        player.graveyard.push_back(std::move(unit));
      } else {
        const int row=unit.agile?agileTargetRow(g,owner,unit):rowIndex(unit.row);
        player.board[row].push_back(std::move(unit));
      }
    }
  }
  if(!g.skelligeAgileReviveIds.empty()) {
    g.skelligeRevivesRemaining=static_cast<int>(g.skelligeAgileReviveIds.size());
    g.skelligeRevivePlayer=0;
  }
  g.currentPlayer = g.firstPlayer;
  if(g.skelligeRevivesRemaining>0)g.phase=Phase::RoundSetup;
  else startRoundTurn(g);
  recompute(g);
  return true;
}

bool chooseSkelligeRevive(Game& g,int who,const std::string& cardId,Row target) {
  if(g.phase!=Phase::RoundSetup || who!=g.skelligeRevivePlayer || who<0 || who>1 ||
     g.skelligeRevivesRemaining<=0)return false;
  if(std::find(g.skelligeAgileReviveIds.begin(),g.skelligeAgileReviveIds.end(),cardId)==g.skelligeAgileReviveIds.end())return false;
  auto& grave=g.players[who].graveyard;
  const auto found=findCard(grave,cardId);
  if(found==grave.end() || !isCombatUnit(*found) || !found->agile)return false;
  int row=rowIndex(found->row);
  if(found->agile) {
    if(target!=Row::Melee && target!=Row::Ranged)return false;
    row=rowIndex(target);
  } else if(target!=Row::None && target!=found->row)return false;
  const Card revived=*found;
  grave.erase(found);
  g.players[who].board[row].push_back(revived);
  g.skelligeAgileReviveIds.erase(std::find(g.skelligeAgileReviveIds.begin(),g.skelligeAgileReviveIds.end(),cardId));
  --g.skelligeRevivesRemaining;
  if(g.skelligeRevivesRemaining==0) {
    g.skelligeRevivePlayer=-1;
    g.skelligeAgileReviveIds.clear();
    startRoundTurn(g);
  }
  recompute(g);
  return true;
}

std::string validateDeck(const std::vector<Card>& deck, Faction faction) {
  if (deck.size() < 22) return "deck requires at least 22 cards";
  int units = 0, specials = 0;
  std::map<int, int> referenceCounts;
  std::map<int, int> referenceLimits;
  for (const auto& c : deck) {
    if (c.kind == Kind::Leader) continue;
    if (c.deckCopies == 0) return "card is not available in the deck pool";
    if (c.kind == Kind::Unit) ++units; else ++specials;
    if (c.faction != Faction::Neutral && c.faction != faction) return "card belongs to another faction";
    if (c.referenceIndex >= 0) {
      ++referenceCounts[c.referenceIndex];
      referenceLimits[c.referenceIndex] = std::max(referenceLimits[c.referenceIndex], c.deckCopies);
    }
  }
  if (units < 22) return "deck requires at least 22 unit cards";
  if (specials > 10) return "deck allows at most 10 special cards";
  for (const auto& entry : referenceCounts)
    if (entry.second > referenceLimits[entry.first]) return "deck exceeds the available copy count for a card";
  return {};
}

std::string validateDeck(const std::vector<Card>& deck, const Card& leader, Faction faction) {
  if (leader.kind != Kind::Leader) return "deck must have exactly one leader card";
  if (leader.faction != Faction::Neutral && leader.faction != faction) return "leader belongs to another faction";
  return validateDeck(deck, faction);
}

std::vector<Card> makeReferenceDeck(const std::vector<std::pair<int, int>>& cardCounts) {
  const auto& catalog = referenceCardCatalog();
  std::vector<Card> deck;
  std::vector<bool> used(catalog.size(), false);
  for (const auto& selection : cardCounts) {
    if (selection.second < 0) return {};
    for (int copy = 0; copy < selection.second; ++copy) {
      auto found = catalog.end();
      for (auto it = catalog.begin(); it != catalog.end(); ++it) {
        const auto index = static_cast<std::size_t>(it - catalog.begin());
        if (!used[index] && it->referenceIndex == selection.first && it->deckCopies > 0 && it->kind != Kind::Leader) {
          found = it; used[index] = true; break;
        }
      }
      if (found == catalog.end()) return {};
      deck.push_back(*found);
    }
  }
  return deck;
}
std::vector<Card> makeReferenceDeck(const ReferenceDeck& recipe) {
  return makeReferenceDeck(recipe.cardCounts);
}
bool referenceLeader(int sourceIndex, Card& leader) {
  const auto& catalog = referenceCardCatalog();
  const auto found = std::find_if(catalog.begin(), catalog.end(), [&](const Card& card) {
    return card.referenceIndex == sourceIndex && card.kind == Kind::Leader;
  });
  if (found == catalog.end()) return false;
  leader = *found;
  return true;
}
}
