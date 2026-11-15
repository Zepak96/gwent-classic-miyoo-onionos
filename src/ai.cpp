#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace gwent {
namespace {
struct Candidate { AiAction action; double value{0.0}; };
bool has(const Card& c, const std::string& ability) {
  return std::find(c.abilities.begin(), c.abilities.end(), ability) != c.abilities.end();
}
std::uint32_t roll(Game& g) {
  auto x = static_cast<std::uint32_t>(g.rngState ? g.rngState : 1);
  x ^= x << 13; x ^= x >> 17; x ^= x << 5; g.rngState = x; return x;
}
bool wouldWinRound(const Game& g, int who) {
  const auto& a = g.players[who]; const auto& b = g.players[1-who];
  if (a.score != b.score) return a.score > b.score;
  return a.faction == Faction::Nilfgaard && b.faction != Faction::Nilfgaard;
}
void addCandidate(std::vector<Candidate>& out, const Game& state, int who, const AiAction& action) {
  Game after = state;
  if (!playCard(after, who, action.cardId, action.target, action.selectedCardId)) return;
  const auto& beforePlayer = state.players[who];
  const auto& beforeOpponent = state.players[1-who];
  const auto& afterPlayer = after.players[who];
  const auto& afterOpponent = after.players[1-who];
  const int delta = (afterPlayer.score - afterOpponent.score) - (beforePlayer.score - beforeOpponent.score);
  const int spentCards = std::max(0, static_cast<int>(beforePlayer.hand.size()) - static_cast<int>(afterPlayer.hand.size()));
  double value = delta - spentCards * (state.round == 1 ? 0.45 : 0.22);
  const auto card = std::find_if(beforePlayer.hand.begin(), beforePlayer.hand.end(), [&](const Card& c) { return c.id == action.cardId; });
  if (card != beforePlayer.hand.end()) {
    if (card->spy) value += 4.5; // two replacement draws are a large resource swing
    if (card->medic && !action.selectedCardId.empty()) value += 1.0;
    if (card->muster) value += 0.35 * static_cast<double>(beforePlayer.deck.size());
  }
  if (!action.selectedCardId.empty() && card != beforePlayer.hand.end() && card->kind == Kind::Decoy) {
    for (const auto& row : beforePlayer.board) for (const auto& target : row)
      if (target.id == action.selectedCardId) value += target.power * 0.45;
  }
  out.push_back({action, value});
}
std::vector<Candidate> legalCandidates(const Game& state, int who) {
  std::vector<Candidate> result;
  const auto& player = state.players[who];
  for (const auto& card : player.hand) {
    if (card.kind == Kind::Leader) continue;
    AiAction action; action.cardId = card.id;
    if (card.kind == Kind::Decoy) {
      for (const auto& row : player.board) for (const auto& target : row)
        if (target.kind == Kind::Unit && !target.hero) { action.selectedCardId = target.id; addCandidate(result,state,who,action); }
      continue;
    }
    if (card.kind == Kind::Unit && card.medic) {
      bool foundTarget = false;
      for (const auto& target : player.graveyard) if (target.kind == Kind::Unit && !target.hero) {
        action.selectedCardId = target.id; addCandidate(result,state,who,action); foundTarget = true;
      }
      if (!foundTarget) { action.selectedCardId.clear(); addCandidate(result,state,who,action); }
      continue;
    }
    if (card.kind == Kind::CommanderHorn || card.kind == Kind::Mardroeme) {
      for (int row = 0; row < 3; ++row) {
        if (card.kind == Kind::CommanderHorn && state.horn[who][row]) continue;
        if (card.kind == Kind::Mardroeme && std::none_of(player.board[row].begin(),player.board[row].end(),[](const Card& c){return c.berserker;})) continue;
        action.target = static_cast<Row>(row); addCandidate(result,state,who,action);
      }
      continue;
    }
    if (card.kind == Kind::Weather && !(has(card,"frost") || has(card,"fog") || has(card,"rain") || has(card,"storm"))) {
      for (int row = 0; row < 3; ++row) { action.target = static_cast<Row>(row); addCandidate(result,state,who,action); }
      continue;
    }
    if (card.kind == Kind::Unit && card.agile) {
      for (int row = 0; row < 2; ++row) { action.target=static_cast<Row>(row); addCandidate(result,state,who,action); }
      continue;
    }
    addCandidate(result,state,who,action);
  }
  return result;
}
}

AiAction chooseAiAction(Game& game, int who, Difficulty difficulty) {
  if (who < 0 || who > 1 || who != game.currentPlayer ||
      (game.phase != Phase::PlayerTurn && game.phase != Phase::OpponentTurn)) return {true,{},{},{}};
  auto& own = game.players[who];
  const auto& other = game.players[1-who];
  const bool leading = wouldWinRound(game, who);
  const bool opponentPassed = other.passed;
  if (own.hand.empty()) return {true,{},{},{}};
  if (opponentPassed && leading) return {true,{},{},{}};
  if (!opponentPassed && leading) {
    if (difficulty == Difficulty::Easy && roll(game) % 4 == 0) return {true,{},{},{}};
    if (difficulty == Difficulty::Normal && (own.hand.size() <= other.hand.size() || own.score - other.score >= 7)) return {true,{},{},{}};
    if (difficulty == Difficulty::Hard && (own.hand.size() <= other.hand.size() || own.score - other.score >= 4)) return {true,{},{},{}};
  }

  auto candidates = legalCandidates(game, who);
  if (candidates.empty()) return {true,{},{},{}};
  if (difficulty == Difficulty::Easy) return candidates[roll(game) % candidates.size()].action;

  auto best = std::max_element(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
    return a.value < b.value;
  });
  if (difficulty == Difficulty::Hard && !opponentPassed && !leading && own.hand.size() > 1) {
    // Preserve a small reserve in early rounds when the current play does not change the round.
    if (game.round == 1 && best->value <= 0.0 && roll(game) % 3 == 0) return {true,{},{},{}};
  }
  return best->action;
}
}
