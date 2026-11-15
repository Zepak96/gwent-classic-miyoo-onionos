#include "gwent/game.hpp"
#include "gwent/catalog.hpp"
#include "gwent/ai.hpp"
#include <algorithm>
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

  Player rowA, rowB;
  rowA.board[0].push_back(unit("a-4",4)); rowB.board[0].push_back(unit("b-5",5));
  assert(calculateScore(rowA,{false,false,false},{true,false,false})==8);
  assert(calculateScore(rowB,{false,false,false},{false,false,false})==5); // a horn only affects its owner's side

  auto hero = unit("hero", 10); hero.hero = true;
  p.board[0].push_back(hero);
  assert(calculateScore(p, {true,false,false}, {true,false,false}) == 12); // weather and horn do not affect Heroes

  Player morale;
  auto moraleUnit = unit("morale", 1); moraleUnit.morale = true;
  morale.board[0] = {moraleUnit, unit("ally", 4)};
  assert(calculateScore(morale, {false,false,false}, {false,false,false}) == 6); // morale boosts allies, not itself

  Player dandelion;
  auto hornUnit = unit("dandelion",2); hornUnit.abilities={"horn"};
  dandelion.board[0]={hornUnit,unit("ally",4)};
  assert(calculateScore(dandelion,{false,false,false},{false,false,false})==10);

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
  assert(startNextRound(g)); assert(g.currentPlayer==0); // the previous round's winner starts
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
  assert(decoy.players[0].board[0].size()==1 && decoy.players[0].board[0][0].kind==Kind::Decoy);
  assert(decoy.players[0].hand.size()==1 && decoy.players[0].hand[0].id=="return");

  Game medic; auto m=unit("medic",2); m.medic=true; medic.players[0].hand.push_back(m);
  medic.players[0].graveyard.push_back(unit("revive",6));
  assert(playCard(medic,0,"medic",Row::None,"revive"));
  assert(medic.players[0].board[0].size()==2 && medic.players[0].graveyard.empty());

  Game muster; auto leader=unit("muster1",4); leader.muster=true; leader.group="crone";
  auto copy=unit("muster2",4,Row::Ranged); copy.muster=true; copy.group="crone";
  muster.players[0].hand.push_back(leader); muster.players[0].deck.push_back(copy);
  assert(playCard(muster,0,"muster1"));
  assert(muster.players[0].deck.empty() && muster.players[0].board[0].size()==1 && muster.players[0].board[1].size()==1);

  Game weather; Card frost; frost.id="frost"; frost.kind=Kind::Weather; frost.abilities={"frost"};
  weather.players[0].hand.push_back(frost); weather.players[0].board[0].push_back(unit("cold",8));
  assert(playCard(weather,0,"frost"));
  assert(weather.weather[0] && weather.weatherCards.size()==1 && weather.players[0].graveyard.empty());
  Card clear; clear.id="clear"; clear.kind=Kind::ClearWeather; weather.players[1].hand.push_back(clear);
  assert(playCard(weather,1,"clear"));
  assert(!weather.weather[0] && weather.weatherCards.empty());
  assert(weather.players[0].graveyard.size()==1 && weather.players[1].graveyard.size()==1);

  Game horns; Card commanderHorn; commanderHorn.id="horn"; commanderHorn.kind=Kind::CommanderHorn;
  horns.players[0].hand.push_back(commanderHorn);
  horns.players[0].board[0].push_back(unit("friendly",4));
  horns.players[1].board[0].push_back(unit("enemy",5));
  assert(playCard(horns,0,"horn",Row::Melee));
  assert(horns.players[0].score==8 && horns.players[1].score==5);

  Game rowScorch; auto scorcher=unit("scorcher",4); scorcher.abilities={"scorch_c"}; rowScorch.players[0].hand.push_back(scorcher);
  rowScorch.players[1].board[0]={unit("target-6",6),unit("target-4",4)};
  assert(playCard(rowScorch,0,"scorcher"));
  assert(rowScorch.players[1].board[0].size()==1 && rowScorch.players[1].board[0][0].id=="target-4");
  assert(rowScorch.players[1].graveyard.size()==1);

  std::vector<Card> deck(22,unit("u",1));
  assert(validateDeck(deck,Faction::NorthernRealms).empty());
  deck.push_back(Card{"wrong","",Kind::Unit,Row::Melee,Faction::Nilfgaard,1});
  assert(!validateDeck(deck,Faction::NorthernRealms).empty());
}
static void openingHandAndMulligan() {
  std::vector<Card> a, b;
  for (int i = 0; i < 24; ++i) { a.push_back(unit("a" + std::to_string(i), i % 10)); b.push_back(unit("b" + std::to_string(i), i % 10)); }
  Game g; beginMatch(g, a, Faction::NorthernRealms, b, Faction::Monsters, 0x12345678, 1);
  assert(g.phase == Phase::Mulligan && g.currentPlayer == 1);
  assert(g.players[0].hand.size() == 10 && g.players[0].deck.size() == 14);
  const auto first = g.players[0].hand[0].id;
  const auto second = g.players[0].hand[1].id;
  assert(!replaceMulliganCards(g, 0, {first, "not-in-hand"}));
  assert(g.players[0].hand.size() == 10 && !g.players[0].mulliganDone);
  assert(replaceMulliganCards(g, 0, {first, second}));
  assert(g.players[0].hand.size() == 10 && g.players[0].deck.size() == 14);
  assert(std::find_if(g.players[0].hand.begin(), g.players[0].hand.end(), [&](const Card& c){return c.id==first;}) == g.players[0].hand.end());
  assert(replaceMulliganCards(g, 1, {}));
  assert(g.phase == Phase::OpponentTurn && g.currentPlayer == 1);
}
static void referenceCatalog() {
  const auto& catalog = referenceCardCatalog();
  assert(catalog.size() == 258);
  assert(std::count_if(catalog.begin(), catalog.end(), [](const Card& c){return c.kind==Kind::Leader;}) >= 20);
  auto geralt = std::find_if(catalog.begin(), catalog.end(), [](const Card& c){return c.name=="Geralt of Rivia";});
  assert(geralt != catalog.end() && geralt->muster && geralt->group=="Roach");
  auto roach = std::find_if(catalog.begin(), catalog.end(), [](const Card& c){return c.name=="Roach";});
  assert(roach != catalog.end());
  auto blueStripes = std::find_if(catalog.begin(), catalog.end(), [](const Card& c){return c.name=="Blue Stripes Commando";});
  assert(blueStripes != catalog.end());
  auto triple = makeReferenceDeck(std::vector<std::pair<int,int>>{{blueStripes->referenceIndex,3}});
  assert(triple.size()==3 && triple[0].group=="Blue Stripes Commando");
  auto leader = std::find_if(catalog.begin(), catalog.end(), [](const Card& c){return c.kind==Kind::Leader && c.faction==Faction::NorthernRealms;});
  assert(leader != catalog.end());
  std::vector<Card> deck(22, unit("plain",1));
  assert(validateDeck(deck,*leader,Faction::NorthernRealms).empty());

  const auto& recipes = referencePremadeDecks();
  assert(recipes.size()==10);
  for (const auto& recipe : recipes) {
    const auto premade = makeReferenceDeck(recipe);
    Card chosenLeader;
    assert(!premade.empty() && referenceLeader(recipe.leaderReferenceIndex, chosenLeader));
    assert(validateDeck(premade, chosenLeader, recipe.faction).empty());
  }

  Game mardroeme; Card potion; potion.id="potion"; potion.kind=Kind::Mardroeme;
  mardroeme.players[0].hand.push_back(potion);
  auto berserker=unit("bear-source",4); berserker.name="Berserker"; berserker.berserker=true;
  mardroeme.players[0].board[0].push_back(berserker);
  assert(playCard(mardroeme,0,"potion",Row::Melee));
  assert(mardroeme.players[0].board[0][0].name=="Transformed Vildkaarl");
  assert(mardroeme.players[0].board[0][0].power==14);
}
static void aiUsesNoHiddenCardsAndCompletesMatches() {
  Game a, b;
  a.players[0].hand.push_back(unit("visible",4)); b.players[0].hand.push_back(unit("visible",4));
  a.players[1].hand.push_back(unit("hidden-A",1)); b.players[1].hand.push_back(unit("hidden-B",15));
  a.players[1].hand[0].name="private hand A"; b.players[1].hand[0].name="private hand B";
  const auto choiceA=chooseAiAction(a,0,Difficulty::Hard);
  const auto choiceB=chooseAiAction(b,0,Difficulty::Hard);
  assert(choiceA.pass==choiceB.pass && choiceA.cardId==choiceB.cardId && choiceA.target==choiceB.target);

  const auto& recipes=referencePremadeDecks();
  const auto deck0=makeReferenceDeck(recipes[0]);
  const auto deck1=makeReferenceDeck(recipes[2]);
  Game game;
  beginMatch(game,deck0,recipes[0].faction,deck1,recipes[2].faction,0x987654,0);
  assert(replaceMulliganCards(game,0,{})); assert(replaceMulliganCards(game,1,{}));
  int actions=0;
  while(game.phase!=Phase::MatchResult && actions<500) {
    if(game.phase==Phase::RoundResult) { assert(startNextRound(game)); continue; }
    const int actor=game.currentPlayer;
    const auto action=chooseAiAction(game,actor,Difficulty::Hard);
    const bool ok=action.pass ? pass(game,actor) : playCard(game,actor,action.cardId,action.target,action.selectedCardId);
    assert(ok); ++actions;
  }
  assert(game.phase==Phase::MatchResult && actions<500);
}
int main() { scoreRules(); turnsAndRounds(); abilitiesAndValidation(); openingHandAndMulligan(); referenceCatalog(); aiUsesNoHiddenCardsAndCompletesMatches(); }
