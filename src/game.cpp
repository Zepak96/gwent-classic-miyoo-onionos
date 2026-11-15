#include "gwent/game.hpp"
#include <algorithm>
#include <numeric>

namespace gwent {
namespace {
int rowIndex(Row row) { return static_cast<int>(row); }
void recompute(Game& g) {
  for (auto& p : g.players) p.score = calculateScore(p, g.weather, g.horn);
}
void endTurn(Game& g) {
  recompute(g);
  const int acted = g.currentPlayer;
  if (g.players[0].passed && g.players[1].passed) {
    if (g.players[0].score > g.players[1].score) ++g.players[0].roundWins;
    else if (g.players[1].score > g.players[0].score) ++g.players[1].roundWins;
    else if (g.players[0].faction == Faction::Nilfgaard && g.players[1].faction != Faction::Nilfgaard)
      ++g.players[0].roundWins;
    else if (g.players[1].faction == Faction::Nilfgaard && g.players[0].faction != Faction::Nilfgaard)
      ++g.players[1].roundWins;
    else { ++g.players[0].roundWins; ++g.players[1].roundWins; }
    g.phase = (g.players[0].roundWins >= 2 || g.players[1].roundWins >= 2)
      ? Phase::MatchResult : Phase::RoundResult;
    return;
  }
  // A player who has passed cannot be made active again during this round.
  // The player who has not passed retains the turn until they pass as well.
  const int other = 1 - acted;
  g.currentPlayer = g.players[other].passed ? acted : other;
  g.phase = g.currentPlayer == 0 ? Phase::PlayerTurn : Phase::OpponentTurn;
}
}

int calculateScore(const Player& p, const std::array<bool,3>& weather,
                   const std::array<bool,3>& horn) {
  int total = 0;
  for (int r=0; r<3; ++r) {
    const auto& row = p.board[r];
    int rowTotal=0;
    for (const auto& c : row) {
      int value = c.power;
      if (weather[r] && !c.hero) value = 1;
      if (c.morale) ++value;
      if (c.bond && !c.group.empty()) {
        int count=0; for (const auto& peer : row) if (peer.group == c.group) ++count;
        value *= count;
      }
      rowTotal += value;
    }
    const bool hasHorn = horn[r] || std::any_of(row.begin(),row.end(),[](const Card& c){return c.kind==Kind::CommanderHorn;});
    total += rowTotal * (hasHorn ? 2 : 1);
  }
  return total;
}

bool playCard(Game& g, int who, const std::string& id, Row target) {
  if (who < 0 || who > 1 || g.phase == Phase::MatchResult || g.phase == Phase::RoundResult ||
      g.players[who].passed || who != g.currentPlayer) return false;
  auto& p = g.players[who];
  auto it = std::find_if(p.hand.begin(),p.hand.end(),[&](const Card& c){return c.id==id;});
  if (it == p.hand.end()) return false;
  Card c=*it;
  if ((c.kind==Kind::Unit && c.row==Row::None && target==Row::None) ||
      ((c.kind==Kind::Weather || c.kind==Kind::CommanderHorn) &&
       (target==Row::None || rowIndex(target)<0 || rowIndex(target)>2))) return false;
  p.hand.erase(it);
  switch(c.kind) {
    case Kind::Weather:
      g.weather[rowIndex(target)]=true; p.graveyard.push_back(c); break;
    case Kind::ClearWeather: g.weather.fill(false); p.graveyard.push_back(c); break;
    case Kind::Scorch: {
      int maxPower=0;
      for (const auto& pl:g.players) for (const auto& row:pl.board) for (const auto& unit:row)
        if (!unit.hero) maxPower=std::max(maxPower,unit.power);
      for (auto& pl:g.players) for (auto& row:pl.board)
        for (auto u=row.begin();u!=row.end();) if (!u->hero && u->power==maxPower) { pl.graveyard.push_back(*u); u=row.erase(u); } else ++u;
      p.graveyard.push_back(c); break;
    }
    case Kind::CommanderHorn:
      g.horn[rowIndex(target)]=true; p.graveyard.push_back(c); break;
    case Kind::Decoy: p.graveyard.push_back(c); break; // TODO: target selection and return to hand.
    case Kind::Unit:
      if(c.spy) { g.players[1-who].board[rowIndex(c.row)].push_back(c); }
      else p.board[rowIndex(c.row)].push_back(c);
      break;
  }
  endTurn(g);
  return true;
}
bool pass(Game& g, int who) {
  if(who<0||who>1||who!=g.currentPlayer||g.phase==Phase::RoundResult||g.phase==Phase::MatchResult||g.players[who].passed) return false;
  g.players[who].passed=true; endTurn(g); return true;
}
bool startNextRound(Game& g) {
  if(g.phase!=Phase::RoundResult) return false;
  ++g.round; g.weather.fill(false); g.horn.fill(false);
  for(auto& p:g.players) { p.passed=false; p.score=0; for(auto& row:p.board) { p.graveyard.insert(p.graveyard.end(),row.begin(),row.end()); row.clear(); } }
  g.currentPlayer = g.players[0].roundWins < g.players[1].roundWins ? 0 : 1;
  g.phase=g.currentPlayer==0?Phase::PlayerTurn:Phase::OpponentTurn; return true;
}
std::string validateDeck(const std::vector<Card>& deck, Faction faction) {
  if(deck.size()<22) return "deck requires at least 22 cards";
  int units=0; for(const auto& c:deck) { if(c.kind==Kind::Unit) ++units; if(c.faction!=Faction::Neutral && c.faction!=faction) return "card belongs to another faction"; }
  if(units<22) return "deck requires at least 22 unit cards";
  return {};
}
}
