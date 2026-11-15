#include "gwent/game.hpp"
#include <algorithm>
#include <numeric>

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
int cardPower(const Card& c, const std::vector<Card>& row, bool weather, bool horn) {
  if (c.hero) return c.power; // Heroes ignore all row effects.
  int power = weather ? std::min(c.power, 1) : c.power;
  const int moraleCount = static_cast<int>(std::count_if(row.begin(), row.end(), [](const Card& x) { return x.morale; }));
  power += std::max(0, moraleCount - (c.morale ? 1 : 0));
  if (c.bond && !c.group.empty()) {
    const auto copies = std::count_if(row.begin(), row.end(), [&](const Card& x) { return x.group == c.group; });
    if (copies > 1) power *= static_cast<int>(copies);
  }
  if (horn) power *= 2;
  return power;
}
void recompute(Game& g) {
  for (int owner = 0; owner < 2; ++owner) g.players[owner].score = calculateScore(g.players[owner], g.weather, g.horn);
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
  if (!played.muster || played.group.empty()) return;
  auto deploy = [&](std::vector<Card>& source) {
    for (auto it = source.begin(); it != source.end();) {
      if (it->muster && it->group == played.group) {
        const int r = rowIndex(it->row);
        if (r >= 0 && r < 3) player.board[r].push_back(*it);
        it = source.erase(it);
      } else ++it;
    }
  };
  deploy(player.hand);
  deploy(player.deck);
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

bool playCard(Game& g, int who, const std::string& id, Row target, const std::string& selectedId) {
  if (who < 0 || who > 1 || who != g.currentPlayer || g.players[who].passed ||
      g.phase == Phase::RoundResult || g.phase == Phase::MatchResult) return false;
  auto& p = g.players[who];
  auto it = findCard(p.hand, id);
  if (it == p.hand.end()) return false;
  const Card c = *it;
  Row deployRow = c.row;
  if (c.agile && (target == Row::Melee || target == Row::Ranged)) deployRow = target;
  if (c.kind == Kind::Unit && !validRow(deployRow)) return false;
  if ((c.kind == Kind::Weather || c.kind == Kind::CommanderHorn) && !validRow(target)) return false;

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
      g.weather[rowIndex(target)] = true; p.graveyard.push_back(c); break;
    case Kind::ClearWeather: g.weather.fill(false); p.graveyard.push_back(c); break;
    case Kind::Scorch: {
      int strongest = -1;
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r)
        for (int n = 0; n < static_cast<int>(g.players[owner].board[r].size()); ++n) {
          const auto& unit = g.players[owner].board[r][n];
          const int power = unit.kind == Kind::Unit ? cardPower(unit, g.players[owner].board[r], g.weather[r], g.horn[r]) : -1;
          if (!unit.hero) strongest = std::max(strongest, power);
        }
      for (int owner = 0; owner < 2; ++owner) for (int r = 0; r < 3; ++r) {
        auto& row = g.players[owner].board[r];
        for (auto unit = row.begin(); unit != row.end();) {
          if (unit->kind == Kind::Unit && !unit->hero && cardPower(*unit, row, g.weather[r], g.horn[r]) == strongest) {
            g.players[owner].graveyard.push_back(*unit); unit = row.erase(unit);
          } else ++unit;
        }
      }
      p.graveyard.push_back(c); break;
    }
    case Kind::CommanderHorn:
      if (g.horn[rowIndex(target)]) { p.hand.push_back(c); return false; }
      g.horn[rowIndex(target)] = true; p.graveyard.push_back(c); break;
    case Kind::Decoy:
      p.hand.push_back(*boardTarget);
      p.board[targetRow].erase(boardTarget);
      p.graveyard.push_back(c); break;
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
      break;
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
  g.firstPlayer = winner >= 0 ? 1 - winner : previousStarter;
  ++g.round;
  g.weather.fill(false); g.horn.fill(false);
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
          if (player.graveyard[i].kind == Kind::Unit && !player.graveyard[i].hero) units.push_back(i);
        if (units.empty()) break;
        const int chosen = units[nextRandom(g) % units.size()];
        Card unit = player.graveyard[chosen]; player.graveyard.erase(player.graveyard.begin() + chosen);
        int r = rowIndex(unit.row);
        if (unit.agile && r > 1) r = static_cast<int>(nextRandom(g) % 2);
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
  for (const auto& c : deck) {
    if (c.kind == Kind::Unit) ++units; else ++specials;
    if (c.faction != Faction::Neutral && c.faction != faction) return "card belongs to another faction";
  }
  if (units < 22) return "deck requires at least 22 unit cards";
  if (specials > 10) return "deck allows at most 10 special cards";
  return {};
}
}
