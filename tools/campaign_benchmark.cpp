#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace gwent;
#include "campaign_drafts.hpp"
int main() {
  try {
    std::cout<<"deck,matches,wins,draws,losses\n";
    for(const auto& draft:campaignDrafts) {
      auto cards=makeReferenceDeck(draft);Card leader;
      if(!referenceLeader(draft.leaderReferenceIndex,leader)||!validateDeck(cards,leader,draft.faction).empty())
        throw std::runtime_error("Illegal campaign draft: "+draft.id);
      int wins=0,draws=0,losses=0;
      for(const auto& control:referencePremadeDecks())for(int seed=1;seed<=12;++seed)for(int side=0;side<2;++side) {
        Card controlLeader;referenceLeader(control.leaderReferenceIndex,controlLeader);
        const auto controlCards=makeReferenceDeck(control);Game game;
        beginMatchWithLeaders(game,side?controlCards:cards,side?controlLeader:leader,
          side?cards:controlCards,side?leader:controlLeader,0xCA0000+seed,seed%2);
        if(game.scoiataelChoicePending&&!chooseScoiataelFirstPlayer(game,seed%2))throw std::runtime_error("First player choice");
        if(!performAiMulligan(game,0)||!performAiMulligan(game,1))throw std::runtime_error("Mulligan");
        int actions=0;
        while(game.phase!=Phase::MatchResult&&actions++<600) {
          if(game.phase==Phase::RoundResult){if(!startNextRound(game))throw std::runtime_error("Round setup");continue;}
          if(game.phase==Phase::RoundSetup) {
            const int owner=game.skelligeRevivePlayer;
            const auto& grave=game.players[owner].graveyard;
            const auto c=std::find_if(grave.begin(),grave.end(),[&](const Card& c){return std::find(game.skelligeAgileReviveIds.begin(),game.skelligeAgileReviveIds.end(),c.id)!=game.skelligeAgileReviveIds.end();});
            if(c==grave.end()||!chooseSkelligeRevive(game,owner,c->id,c->agile?Row::Melee:Row::None))throw std::runtime_error("Skellige choice");
            continue;
          }
          const int actor=game.currentPlayer;const auto a=chooseAiAction(game,actor);
          const bool legal=a.resolveMedic?chooseMedicRevive(game,actor,a.selectedCardId,a.target):
            a.pass?pass(game,actor):a.activateLeader?activateLeader(game,actor,a.target,a.leaderSelection):playCard(game,actor,a.cardId,a.target,a.selectedCardId);
          if(!legal)throw std::runtime_error("Illegal AI action: "+draft.id);
        }
        if(game.phase!=Phase::MatchResult)throw std::runtime_error("Stalled match: "+draft.id);
        if(game.matchWinner==2)++draws;else if(game.matchWinner==side)++wins;else ++losses;
      }
      std::cout<<draft.id<<','<<wins+draws+losses<<','<<wins<<','<<draws<<','<<losses<<'\n';
    }
  } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
