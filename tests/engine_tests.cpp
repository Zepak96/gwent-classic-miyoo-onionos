#include "gwent/game.hpp"
#include <cassert>
#include <utility>

using namespace gwent;
static Card unit(std::string id, int power, Row row = Row::Melee) {
  Card c; c.id = std::move(id); c.kind = Kind::Unit; c.power = power; c.row = row; return c;
}
static void scoreRules() {
  Player p;
  auto plain = unit("plain", 8);
  p.board[0].push_back(plain);
  assert(calculateScore(p, {false,false,false}, {false,false,false}) == 8);
  assert(calculateScore(p, {true,false,false}, {false,false,false}) == 1);
  assert(calculateScore(p, {false,false,false}, {true,false,false}) == 16);

  auto hero = unit("hero", 10); hero.hero = true;
  p.board[0].push_back(hero);
  assert(calculateScore(p, {true,false,false}, {true,false,false}) == 12); // weather and horn do not affect Heroes

  Player morale;
  auto moraleUnit = unit("morale", 1); moraleUnit.morale = true;
  morale.board[0] = {moraleUnit, unit("ally", 4)};
  assert(calculateScore(morale, {false,false,false}, {false,false,false}) == 6); // morale boosts allies, not itself

  Player bond;
  auto a = unit("bond-a", 4); a.bond = true; a.group = "commando";
  auto b = unit("bond-b", 4); b.bond = true; b.group = "commando";
  auto c = unit("bond-c", 4); c.bond = true; c.group = "commando";
  bond.board[0] = {a,b,c};
  assert(calculateScore(bond, {false,false,false}, {false,false,false}) == 36); // three same-name copies each score 4 x 3
}
static void turnsAndRounds() {
  Game g; g.players[0].hand.push_back(unit("a", 8)); g.players[1].hand.push_back(unit("b", 3));
  assert(playCard(g,0,"a")); assert(g.currentPlayer==1);
  assert(pass(g,1)); assert(g.currentPlayer==0 && !g.players[0].passed);
  assert(pass(g,0)); assert(g.phase==Phase::RoundResult);
  assert(g.players[0].roundWins==1 && g.players[1].roundLosses==1);
  assert(startNextRound(g)); assert(g.currentPlayer==1); // previous round's loser starts
  assert(!startNextRound(g));

  Game tie; tie.players[0].hand.push_back(unit("ta", 2)); tie.players[1].hand.push_back(unit("tb", 2));
  assert(playCard(tie,0,"ta")); assert(playCard(tie,1,"tb"));
  assert(pass(tie,0)); assert(pass(tie,1));
  assert(tie.phase==Phase::RoundResult && tie.players[0].roundLosses==1 && tie.players[1].roundLosses==1);

  Game nilfgaard; nilfgaard.players[0].faction=Faction::Nilfgaard;
  nilfgaard.players[0].hand.push_back(unit("na",2)); nilfgaard.players[1].hand.push_back(unit("nb",2));
  assert(playCard(nilfgaard,0,"na")); assert(playCard(nilfgaard,1,"nb"));
  assert(pass(nilfgaard,0)); assert(pass(nilfgaard,1));
  assert(nilfgaard.players[0].roundWins==1 && nilfgaard.players[1].roundLosses==1);
}
static void abilitiesAndValidation() {
  Game spy; auto s=unit("spy",3); s.spy=true; spy.players[0].hand.push_back(s);
  spy.players[0].deck={unit("draw1",1),unit("draw2",1)};
  assert(playCard(spy,0,"spy"));
  assert(spy.players[1].board[0].size()==1 && spy.players[0].hand.size()==2 && spy.players[0].deck.empty());

  Game decoy; decoy.players[0].hand.push_back(Card{"decoy","Decoy",Kind::Decoy});
  decoy.players[0].board[0].push_back(unit("return",5));
  assert(playCard(decoy,0,"decoy",Row::None,"return"));
  assert(decoy.players[0].board[0].empty() && decoy.players[0].hand.size()==1 && decoy.players[0].hand[0].id=="return");

  Game medic; auto m=unit("medic",2); m.medic=true; medic.players[0].hand.push_back(m);
  medic.players[0].graveyard.push_back(unit("revive",6));
  assert(playCard(medic,0,"medic",Row::None,"revive"));
  assert(medic.players[0].board[0].size()==2 && medic.players[0].graveyard.empty());

  Game muster; auto leader=unit("muster1",4); leader.muster=true; leader.group="crone";
  auto copy=unit("muster2",4,Row::Ranged); copy.muster=true; copy.group="crone";
  muster.players[0].hand.push_back(leader); muster.players[0].deck.push_back(copy);
  assert(playCard(muster,0,"muster1"));
  assert(muster.players[0].deck.empty() && muster.players[0].board[0].size()==1 && muster.players[0].board[1].size()==1);

  std::vector<Card> deck(22,unit("u",1));
  assert(validateDeck(deck,Faction::NorthernRealms).empty());
  deck.push_back(Card{"wrong","",Kind::Unit,Row::Melee,Faction::Nilfgaard,1});
  assert(!validateDeck(deck,Faction::NorthernRealms).empty());
}
int main() { scoreRules(); turnsAndRounds(); abilitiesAndValidation(); }
