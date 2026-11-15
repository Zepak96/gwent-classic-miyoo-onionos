#include "gwent/game.hpp"
#include <cassert>
int main(){
  using namespace gwent;
  Player p; Card a; a.id="unit"; a.kind=Kind::Unit; a.power=8; a.row=Row::Melee; p.board[0].push_back(a);
  assert(calculateScore(p,{false,false,false},{false,false,false})==8);
  assert(calculateScore(p,{true,false,false},{false,false,false})==1);
  assert(calculateScore(p,{false,false,false},{true,false,false})==16);
  Game g; g.players[0].hand.push_back(a); g.players[1].hand.push_back(Card{"b","",Kind::Unit,Row::Melee,Faction::Neutral,3});
  assert(playCard(g,0,"unit")); assert(g.currentPlayer==1); assert(pass(g,1)); assert(g.players[0].passed==false);
  assert(g.currentPlayer==0);
  assert(pass(g,0));
  assert(g.phase==Phase::RoundResult);
  return 0;
}
