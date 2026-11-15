#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
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
  const int beforeDiff=calculateScore(beforePlayer,state.weather,state.horn[who])-
                       calculateScore(beforeOpponent,state.weather,state.horn[1-who]);
  const int afterDiff=afterPlayer.score-afterOpponent.score;
  const int delta=afterDiff-beforeDiff;
  const int spentCards = std::max(0, static_cast<int>(beforePlayer.hand.size()) - static_cast<int>(afterPlayer.hand.size()));
  double value = delta - spentCards * (state.round == 1 ? 0.45 : 0.22);
  const auto card = std::find_if(beforePlayer.hand.begin(), beforePlayer.hand.end(), [&](const Card& c) { return c.id == action.cardId; });
  if (card != beforePlayer.hand.end()) {
    if (card->spy) value += 4.5; // two replacement draws are a large resource swing
    if (card->medic && !action.selectedCardId.empty()) value += 1.0;
    if (card->muster) value += 0.35 * static_cast<double>(beforePlayer.deck.size());
    if (has(*card,"avenger") || has(*card,"avenger_kambi")) value += 50.0;
  }
  if (!action.selectedCardId.empty() && card != beforePlayer.hand.end() && card->kind == Kind::Decoy) {
    for (const auto& row : beforePlayer.board) for (const auto& target : row)
      if (target.id == action.selectedCardId) value += target.power * 0.45;
  }
  out.push_back({action, value});
}
void addLeaderCandidate(std::vector<Candidate>& out, const Game& state, int who,
                       Row target = Row::None, std::vector<std::string> selection = {}) {
  AiAction action; action.activateLeader=true; action.target=target; action.leaderSelection=std::move(selection);
  Game after=state;
  if(!activateLeader(after,who,action.target,action.leaderSelection)) return;
  const auto& beforePlayer=state.players[who];
  const auto& beforeOpponent=state.players[1-who];
  const auto& afterPlayer=after.players[who];
  const auto& afterOpponent=after.players[1-who];
  const int beforeDiff=calculateScore(beforePlayer,state.weather,state.horn[who])-
                       calculateScore(beforeOpponent,state.weather,state.horn[1-who]);
  const int afterDiff=afterPlayer.score-afterOpponent.score;
  const int handDelta=static_cast<int>(afterPlayer.hand.size())-static_cast<int>(beforePlayer.hand.size());
  double value=(afterDiff-beforeDiff)+handDelta*0.65-0.15;
  if(has(beforePlayer.leader,"emhyr_emperor") && !beforeOpponent.hand.empty()) value+=1.0;
  if(has(beforePlayer.leader,"crach_an_craite"))
    value+=0.15*static_cast<double>(beforePlayer.graveyard.size()+beforeOpponent.graveyard.size());
  out.push_back({std::move(action),value});
}
void addLeaderCandidates(std::vector<Candidate>& out, const Game& state, int who) {
  const auto& player=state.players[who];
  if(!player.leaderAbilityAvailable || player.leaderAbilityCancelled) return;
  const auto it=std::find_if(player.leader.abilities.begin(),player.leader.abilities.end(),[](const std::string& id){
    return id.rfind("foltest_",0)==0 || id.rfind("emhyr_",0)==0 || id.rfind("eredin_",0)==0 ||
           id.rfind("francesca_",0)==0 || id=="crach_an_craite";
  });
  if(it==player.leader.abilities.end()) return;
  const auto& id=*it;
  if(id=="foltest_king" || id=="emhyr_imperial" || id=="eredin_king" || id=="francesca_pureblood") {
    const bool anyWeather=id=="eredin_king";
    const std::string wanted=id=="foltest_king"?"Impenetrable Fog":id=="emhyr_imperial"?"Torrential Rain":id=="francesca_pureblood"?"Biting Frost":"";
    for(const auto& card:player.deck) if(card.kind==Kind::Weather && (anyWeather || card.name==wanted))
      addLeaderCandidate(out,state,who,Row::None,{card.id});
  } else if(id=="emhyr_relentless") {
    for(const auto& card:state.players[1-who].graveyard) if(card.kind==Kind::Unit && !card.hero)
      addLeaderCandidate(out,state,who,Row::None,{card.id});
  } else if(id=="eredin_bringer_of_death") {
    for(const auto& card:player.graveyard) if(card.kind==Kind::Unit && !card.hero)
      addLeaderCandidate(out,state,who,Row::None,{card.id});
  } else if(id=="eredin_destroyer") {
    if(player.hand.size()<2 || player.deck.empty()) return;
    for(std::size_t a=0;a<player.hand.size();++a) for(std::size_t b=a+1;b<player.hand.size();++b)
      for(const auto& draw:player.deck)
        addLeaderCandidate(out,state,who,Row::None,{player.hand[a].id,player.hand[b].id,draw.id});
  } else if(id=="emhyr_whiteflame" || id=="emhyr_invader" || id=="eredin_treacherous" ||
            id=="francesca_daisy" || id=="king_bran") {
    return;
  } else addLeaderCandidate(out,state,who);
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
  if (own.hand.empty() && !own.leaderAbilityAvailable) return {true,{},{},{}};
  if (opponentPassed && leading) return {true,{},{},{}};
  if (!opponentPassed && leading) {
    if (difficulty == Difficulty::Easy && roll(game) % 4 == 0) return {true,{},{},{}};
    if (difficulty == Difficulty::Normal && (own.hand.size() <= other.hand.size() || own.score - other.score >= 7)) return {true,{},{},{}};
    if (difficulty == Difficulty::Hard && (own.hand.size() <= other.hand.size() || own.score - other.score >= 4)) return {true,{},{},{}};
  }

  auto candidates = legalCandidates(game, who);
  addLeaderCandidates(candidates,game,who);
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
