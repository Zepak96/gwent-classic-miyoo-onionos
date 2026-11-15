#include "gwent/game.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
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
  for (std::size_t n = deck.size(); n > 1; --n) {
    const std::size_t other = nextRandom(g) % n;
    std::swap(deck[n - 1], deck[other]);
  }
}
int cardPower(const Card& c, const std::vector<Card>& row, bool weather, bool horn) {
  if (c.hero) return c.power; // Heroes ignore all row effects.
  int power = weather ? std::min(c.power, 1) : c.power;
  const int moraleCount = static_cast<int>(std::count_if(row.begin(), row.end(), [](const Card& x) { return x.morale || hasAbility(x, "morale"); }));
  power += std::max(0, moraleCount - ((c.morale || hasAbility(c, "morale")) ? 1 : 0));
  if (c.bond && !c.group.empty()) {
    const auto copies = std::count_if(row.begin(), row.end(), [&](const Card& x) { return x.group == c.group; });
    if (copies > 1) power *= static_cast<int>(copies);
  }
  const int hornSources = static_cast<int>(horn) + static_cast<int>(std::count_if(row.begin(), row.end(), [](const Card& x) { return hasAbility(x, "horn"); }));
  if (hornSources > (hasAbility(c, "horn") ? 1 : 0)) power *= 2;
  return power;
}
void recompute(Game& g) {
  for (int owner = 0; owner < 2; ++owner) g.players[owner].score = calculateScore(g.players[owner], g.weather, g.horn[owner]);
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
void applyMuster(Player& player, const Card& played) {
  if ((!played.muster && !hasAbility(played, "muster")) || played.group.empty()) return;
  auto deploy = [&](std::vector<Card>& source) {
    for (auto it = source.begin(); it != source.end();) {
      if ((it->muster || hasAbility(*it, "muster")) && it->group == played.group) {
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
    const int power = cardPower(unit, row, g.weather[rowIndexValue], g.horn[1 - owner][rowIndexValue]);
    rowTotal += power;
    if (!unit.hero) strongest = std::max(strongest, power);
  }
  if (rowTotal < 10 || strongest < 0) return;
  std::vector<int> remove;
  for (int i = 0; i < static_cast<int>(row.size()); ++i)
    if (row[i].kind == Kind::Unit && !row[i].hero && cardPower(row[i], row, g.weather[rowIndexValue], g.horn[1 - owner][rowIndexValue]) == strongest)
      remove.push_back(i);
  auto& grave = g.players[1 - owner].graveyard;
  for (auto i = remove.rbegin(); i != remove.rend(); ++i) {
    grave.push_back(row[*i]);
    row.erase(row.begin() + *i);
  }
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
      if (c.kind == Kind::Unit) total += cardPower(c, p.board[r], weather[r], horn[r]);
  return total;
}

void beginMatch(Game& g, std::vector<Card> playerDeck, Faction playerFaction,
                std::vector<Card> opponentDeck, Faction opponentFaction,
                std::uint64_t seed, int firstPlayer) {
  g = Game{};
  g.rngState = seed ? seed : 1;
  g.firstPlayer = (firstPlayer == 0 || firstPlayer == 1) ? firstPlayer : static_cast<int>(nextRandom(g) % 2);
  for (int i = 0; i < 2; ++i) {
    auto& p = g.players[i];
    p.deck = i == 0 ? std::move(playerDeck) : std::move(opponentDeck);
    p.faction = i == 0 ? playerFaction : opponentFaction;
    shuffleDeck(g, p.deck);
    for (int n = 0; n < 10 && !p.deck.empty(); ++n) {
      p.hand.push_back(p.deck.back()); p.deck.pop_back();
    }
  }
  g.currentPlayer = g.firstPlayer;
  g.phase = Phase::Mulligan;
}

bool replaceMulliganCards(Game& g, int who, const std::vector<std::string>& ids) {
  if (g.phase != Phase::Mulligan || who < 0 || who > 1 || g.players[who].mulliganDone || ids.size() > 2)
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
  // Validate the whole selection before changing any state.
  for (const auto& card : replacements) {
    auto found = findCard(player.hand, card.id);
    player.hand.erase(found);
    player.deck.insert(player.deck.begin(), card);
  }
  for (std::size_t i = 0; i < replacements.size() && !player.deck.empty(); ++i) {
    player.hand.push_back(player.deck.back()); player.deck.pop_back();
  }
  player.mulliganDone = true;
  if (g.players[0].mulliganDone && g.players[1].mulliganDone) {
    g.currentPlayer = g.firstPlayer;
    g.phase = g.currentPlayer == 0 ? Phase::PlayerTurn : Phase::OpponentTurn;
  }
  return true;
}

bool playCard(Game& g, int who, const std::string& id, Row target, const std::string& selectedId) {
  if (who < 0 || who > 1 || who != g.currentPlayer || g.players[who].passed ||
      g.phase == Phase::RoundResult || g.phase == Phase::MatchResult) return false;
  auto& p = g.players[who];
  auto it = findCard(p.hand, id);
  if (it == p.hand.end()) return false;
  const Card c = *it;
  if (c.kind == Kind::Leader) return false;
  Row deployRow = c.row;
  if (c.agile && (target == Row::Melee || target == Row::Ranged)) deployRow = target;
  if (c.kind == Kind::Unit && !validRow(deployRow)) return false;
  if (c.kind == Kind::CommanderHorn && !validRow(target)) return false;
  if (c.kind == Kind::Mardroeme && !validRow(target)) return false;
  const bool typedWeather = hasAbility(c, "frost") || hasAbility(c, "fog") || hasAbility(c, "rain") || hasAbility(c, "storm");
  if (c.kind == Kind::Weather && !typedWeather && !validRow(target)) return false;

  auto boardTarget = p.board[0].end();
  int targetRow = -1;
  if (c.kind == Kind::Decoy) {
    for (int r = 0; r < 3; ++r) {
      auto found = findCard(p.board[r], selectedId);
      if (found != p.board[r].end()) { boardTarget = found; targetRow = r; break; }
    }
    if (selectedId.empty() || targetRow < 0 || boardTarget->kind != Kind::Unit || boardTarget->hero) return false;
  }
  auto graveTarget = findCard(p.graveyard, selectedId);
  const bool medicCanRetrieve = c.kind == Kind::Unit && c.medic && !selectedId.empty() &&
    graveTarget != p.graveyard.end() && graveTarget->kind == Kind::Unit && !graveTarget->hero;
  if (c.kind == Kind::Unit && c.medic && !selectedId.empty() && !medicCanRetrieve) return false;

  p.hand.erase(it);
  switch (c.kind) {
    case Kind::Weather:
      if (hasAbility(c, "frost")) g.weather[0] = true;
      else if (hasAbility(c, "fog")) g.weather[1] = true;
      else if (hasAbility(c, "rain")) g.weather[2] = true;
      else if (hasAbility(c, "storm")) { g.weather[1] = true; g.weather[2] = true; }
      else g.weather[rowIndex(target)] = true;
      g.weatherCards.push_back({c, who}); break;
    case Kind::ClearWeather:
      g.weather.fill(false);
      for (const auto& active : g.weatherCards) g.players[active.owner].graveyard.push_back(active.card);
      g.weatherCards.clear(); p.graveyard.push_back(c); break;
    case Kind::Mardroeme:
      transformBerserkers(p, rowIndex(target)); p.graveyard.push_back(c); break;
    case Kind::Scorch: {
      int strongest = -1;
      std::array<std::array<std::vector<int>, 3>, 2> remove;
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r)
        for (int n = 0; n < static_cast<int>(g.players[owner].board[r].size()); ++n) {
          const auto& unit = g.players[owner].board[r][n];
          const int power = unit.kind == Kind::Unit ? cardPower(unit, g.players[owner].board[r], g.weather[r], g.horn[owner][r]) : -1;
          if (!unit.hero) strongest = std::max(strongest, power);
        }
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r)
        for (int n = 0; n < static_cast<int>(g.players[owner].board[r].size()); ++n) {
          const auto& unit = g.players[owner].board[r][n];
          if (unit.kind == Kind::Unit && !unit.hero && cardPower(unit, g.players[owner].board[r], g.weather[r], g.horn[owner][r]) == strongest)
            remove[owner][r].push_back(n);
        }
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r) {
        auto& row = g.players[owner].board[r];
        for (auto n = remove[owner][r].rbegin(); n != remove[owner][r].rend(); ++n) {
          g.players[owner].graveyard.push_back(row[*n]);
          row.erase(row.begin() + *n);
        }
      }
      p.graveyard.push_back(c); break;
    }
    case Kind::CommanderHorn:
      if (g.horn[who][rowIndex(target)]) { p.hand.push_back(c); return false; }
      g.horn[who][rowIndex(target)] = true; p.graveyard.push_back(c); break;
    case Kind::Decoy:
      p.hand.push_back(*boardTarget);
      *boardTarget = c; // The zero-strength Decoy stays in the replaced unit's battlefield slot.
      break;
    case Kind::Unit:
      if (c.spy) {
        g.players[1 - who].board[rowIndex(deployRow)].push_back(c);
        for (int i = 0; i < 2 && !p.deck.empty(); ++i) { p.hand.push_back(p.deck.back()); p.deck.pop_back(); }
      } else p.board[rowIndex(deployRow)].push_back(c);
      if (c.muster) applyMuster(p, c);
      if (medicCanRetrieve) {
        const Card revived = *graveTarget;
        p.graveyard.erase(graveTarget);
        const int reviveRow = rowIndex(revived.row);
        if (reviveRow >= 0 && reviveRow < 3) p.board[reviveRow].push_back(revived);
      }
      if (hasAbility(c, "scorch_c")) scorchEnemyRow(g, who, 0);
      if (hasAbility(c, "scorch_r")) scorchEnemyRow(g, who, 1);
      if (hasAbility(c, "scorch_s")) scorchEnemyRow(g, who, 2);
      if (hasAbility(c, "mardroeme")) transformBerserkers(p, rowIndex(deployRow));
      break;
    case Kind::Leader:
      p.hand.push_back(c);
      return false;
  }
  endTurn(g);
  return true;
}

bool pass(Game& g, int who) {
  if (who < 0 || who > 1 || who != g.currentPlayer || g.phase == Phase::RoundResult ||
      g.phase == Phase::MatchResult || g.players[who].passed) return false;
  g.players[who].passed = true;
  endTurn(g);
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
  for (const auto& active : g.weatherCards) g.players[active.owner].graveyard.push_back(active.card);
  g.weatherCards.clear();
  g.weather.fill(false);
  for (auto& ownerHorn : g.horn) ownerHorn.fill(false);
  for (int owner = 0; owner < 2; ++owner) {
    auto& player = g.players[owner];
    player.passed = false;
    // Monsters keep one random unit from their side; all other units go to their owner's graveyard.
    int keepRow = -1, keepIndex = -1;
    if (player.faction == Faction::Monsters) {
      int count = 0;
      for (int r = 0; r < 3; ++r) for (const auto& unit : player.board[r]) if (unit.kind == Kind::Unit) ++count;
      if (count) {
        int pick = static_cast<int>(nextRandom(g) % static_cast<std::uint32_t>(count));
        for (int r = 0; r < 3 && keepRow < 0; ++r) for (int n = 0; n < static_cast<int>(player.board[r].size()); ++n)
          if (player.board[r][n].kind == Kind::Unit && pick-- == 0) { keepRow = r; keepIndex = n; break; }
      }
    }
    for (int r = 0; r < 3; ++r) {
      for (int n = static_cast<int>(player.board[r].size()) - 1; n >= 0; --n) {
        if (r == keepRow && n == keepIndex) continue;
        player.graveyard.push_back(player.board[r][n]);
        player.board[r].erase(player.board[r].begin() + n);
      }
    }
    if (winner >= 0 && owner == winner && player.faction == Faction::NorthernRealms && !player.deck.empty()) {
      player.hand.push_back(player.deck.back()); player.deck.pop_back();
    }
  }
  if (g.round == 3) {
    for (auto& player : g.players) if (player.faction == Faction::Skellige) {
      for (int resurrected = 0; resurrected < 2; ++resurrected) {
        std::vector<int> units;
        for (int i = 0; i < static_cast<int>(player.graveyard.size()); ++i)
          if (player.graveyard[i].kind == Kind::Unit) units.push_back(i);
        if (units.empty()) break;
        const int chosen = units[nextRandom(g) % units.size()];
        Card unit = player.graveyard[chosen]; player.graveyard.erase(player.graveyard.begin() + chosen);
        int r = rowIndex(unit.row);
        if (unit.agile && (r < 0 || r > 1)) r = static_cast<int>(nextRandom(g) % 2);
        if (r >= 0 && r < 3) player.board[r].push_back(unit);
      }
    }
  }
  g.currentPlayer = g.firstPlayer;
  g.phase = g.currentPlayer == 0 ? Phase::PlayerTurn : Phase::OpponentTurn;
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
  if (leader.deckCopies == 0) return "leader is not available in the deck pool";
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
    return card.referenceIndex == sourceIndex && card.kind == Kind::Leader && card.deckCopies > 0;
  });
  if (found == catalog.end()) return false;
  leader = *found;
  return true;
}
}
