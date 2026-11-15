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
static Card referenceCard(const std::string& name);
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

  Player orderedEffects;
  auto bondedA=unit("bonded-a",4); bondedA.bond=true; bondedA.group="same-name";
  auto bondedB=unit("bonded-b",4); bondedB.bond=true; bondedB.group="same-name";
  auto rally=unit("rally",1); rally.morale=true;
  orderedEffects.board[0]={bondedA,bondedB,rally};
  assert(calculateScore(orderedEffects,{false,false,false},{false,false,false})==19); // Bond multiplies before Morale is added.
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
  assert(!playCard(medic,0,"medic")); // A valid target must be selected before committing the play.
  assert(playCard(medic,0,"medic",Row::None,"revive"));
  assert(medic.players[0].board[0].size()==2 && medic.players[0].graveyard.empty());

  Game medicHero; auto mh=unit("medic-hero",2); mh.medic=true; medicHero.players[0].hand.push_back(mh);
  auto forbiddenHero=unit("forbidden-hero",10); forbiddenHero.hero=true;
  medicHero.players[0].graveyard.push_back(forbiddenHero);
  assert(!playCard(medicHero,0,"medic-hero",Row::None,"forbidden-hero"));

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

  Game storm; storm.players[0].hand.push_back(referenceCard("Skellige Storm"));
  assert(playCard(storm,0,storm.players[0].hand[0].id));
  assert(!storm.weather[0] && storm.weather[1] && storm.weather[2]);

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
  const auto choiceA=chooseAiAction(a,0);
  const auto choiceB=chooseAiAction(b,0);
  assert(choiceA.pass==choiceB.pass && choiceA.cardId==choiceB.cardId && choiceA.target==choiceB.target &&
         choiceA.activateLeader==choiceB.activateLeader && choiceA.leaderSelection==choiceB.leaderSelection);

  Game leaderChoice;
  leaderChoice.players[0].leader.id="foltest_siegemaster";
  leaderChoice.players[0].leader.kind=Kind::Leader;
  leaderChoice.players[0].leader.abilities={"foltest_siegemaster"};
  leaderChoice.players[0].leaderAbilityAvailable=true;
  leaderChoice.players[0].board[2].push_back(unit("ai-siege",6,Row::Siege));
  leaderChoice.players[0].hand.push_back(unit("ai-small-play",1));
  const auto leaderAction=chooseAiAction(leaderChoice,0);
  assert(leaderAction.activateLeader && leaderAction.leaderSelection.empty());

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
    const auto action=chooseAiAction(game,actor);
    const bool ok=action.pass ? pass(game,actor) : action.activateLeader ? activateLeader(game,actor,action.target,action.leaderSelection) :
      playCard(game,actor,action.cardId,action.target,action.selectedCardId);
    assert(ok); ++actions;
  }
  assert(game.phase==Phase::MatchResult && actions<500);
}
static Card leader(std::string id) {
  Card card; card.id=id; card.name=id; card.kind=Kind::Leader; card.faction=Faction::NorthernRealms; card.abilities={id}; return card;
}
static Card referenceCard(const std::string& name) {
  const auto& catalog=referenceCardCatalog();
  const auto found=std::find_if(catalog.begin(),catalog.end(),[&](const Card& card){return card.name==name;});
  assert(found!=catalog.end());
  return *found;
}
static void avengerCards() {
  Game decoy;
  decoy.players[0].hand.push_back(referenceCard("Decoy"));
  decoy.players[0].board[1].push_back(referenceCard("Cow"));
  assert(playCard(decoy,0,decoy.players[0].hand[0].id,Row::None,decoy.players[0].board[1][0].id));
  assert(decoy.players[0].hand.back().name=="Cow");
  assert(std::any_of(decoy.players[0].board[0].begin(),decoy.players[0].board[0].end(),[](const Card& card){return card.name=="Bovine Defense Force";}));

  Game scorch;
  Card globalScorch; globalScorch.id="global-scorch"; globalScorch.kind=Kind::Scorch;
  scorch.players[0].hand.push_back(globalScorch);
  scorch.players[1].board[0].push_back(referenceCard("Kambi"));
  assert(playCard(scorch,0,"global-scorch"));
  assert(std::any_of(scorch.players[1].board[0].begin(),scorch.players[1].board[0].end(),[](const Card& card){return card.name=="Hemdall";}));
  assert(std::any_of(scorch.players[1].graveyard.begin(),scorch.players[1].graveyard.end(),[](const Card& card){return card.name=="Kambi";}));

  Game roundEnd;
  roundEnd.players[0].board[0].push_back(referenceCard("Kambi"));
  assert(pass(roundEnd,0)); assert(pass(roundEnd,1));
  assert(roundEnd.phase==Phase::RoundResult && startNextRound(roundEnd));
  assert(std::any_of(roundEnd.players[0].board[0].begin(),roundEnd.players[0].board[0].end(),[](const Card& card){return card.name=="Hemdall";}));
}
static void leaderAbilities() {
  std::vector<Card> deck;
  for(int i=0;i<24;++i) deck.push_back(unit("leader-test-"+std::to_string(i),1));
  Game passives;
  beginMatchWithLeaders(passives,deck,leader("francesca_daisy"),deck,leader("king_bran"),0x1234,0);
  assert(passives.players[0].hand.size()==11 && passives.players[0].deck.size()==13);
  passives.players[1].board[0].push_back(unit("bran-weather",9));
  assert(calculateScore(passives.players[1],{true,false,false},{false,false,false})==4);

  Game cancellation;
  beginMatchWithLeaders(cancellation,deck,leader("foltest_lord"),deck,leader("emhyr_whiteflame"),0x4321,0);
  assert(!cancellation.players[0].leaderAbilityAvailable && cancellation.players[0].leaderAbilityCancelled);
  assert(!cancellation.players[1].leaderAbilityAvailable && cancellation.players[1].leaderAbilityCancelled);

  Game spies;
  beginMatchWithLeaders(spies,deck,leader("eredin_treacherous"),deck,leader("emhyr_emperor"),0x5678,0);
  auto spyA=unit("spy-a",3); spyA.spy=true;
  auto spyB=unit("spy-b",4); spyB.spy=true;
  spies.players[0].board[0].push_back(spyA); spies.players[1].board[0].push_back(spyB);
  assert(calculateScore(spies.players[0],spies.weather,spies.horn[0])==6 &&
         calculateScore(spies.players[1],spies.weather,spies.horn[1])==8);
  assert(spies.players[0].board[0][0].power==3); // Passive modifies scoring, not base card data.

  Game cancelledPassive;
  beginMatchWithLeaders(cancelledPassive,deck,leader("eredin_treacherous"),deck,leader("emhyr_whiteflame"),0x789a,0);
  cancelledPassive.players[0].board[0].push_back(spyA);
  assert(!cancelledPassive.players[0].doubleSpyPower &&
         calculateScore(cancelledPassive.players[0],cancelledPassive.weather,cancelledPassive.horn[0])==3);

  Game weather;
  weather.phase=Phase::PlayerTurn; weather.players[0].leader=leader("foltest_king");
  weather.players[0].leaderAbilityAvailable=true;
  Card fog; fog.id="deck-fog"; fog.kind=Kind::Weather; fog.abilities={"fog"};
  fog.name="Impenetrable Fog";
  weather.players[0].deck.push_back(fog);
  assert(activateLeader(weather,0));
  assert(weather.weather[1] && weather.weatherCards.size()==1 && weather.players[0].deck.empty());

  Game exactWeather;
  exactWeather.players[0].leader=leader("foltest_king"); exactWeather.players[0].leaderAbilityAvailable=true;
  exactWeather.players[0].deck={referenceCard("Skellige Storm"),referenceCard("Impenetrable Fog")};
  assert(activateLeader(exactWeather,0));
  assert(exactWeather.weather[1] && !exactWeather.weather[2] && exactWeather.weatherCards[0].card.name=="Impenetrable Fog");

  Game horn;
  horn.players[0].leader=leader("foltest_siegemaster"); horn.players[0].leaderAbilityAvailable=true;
  horn.players[0].board[2].push_back(unit("siege",5));
  assert(activateLeader(horn,0));
  assert(horn.players[0].score==10 && !horn.players[0].leaderAbilityAvailable);

  Game rowScorch;
  rowScorch.players[0].leader=leader("foltest_steelforged"); rowScorch.players[0].leaderAbilityAvailable=true;
  rowScorch.players[1].board[2]={unit("siege-6",6,Row::Siege),unit("siege-4",4,Row::Siege)};
  assert(activateLeader(rowScorch,0));
  assert(rowScorch.players[1].board[2].size()==1 && rowScorch.players[1].graveyard.size()==1);

  Game draw;
  draw.players[0].leader=leader("emhyr_relentless"); draw.players[0].leaderAbilityAvailable=true;
  draw.players[1].graveyard.push_back(unit("stolen-unit",7));
  assert(activateLeader(draw,0,Row::None,{"stolen-unit"}));
  assert(draw.players[0].hand.size()==1 && draw.players[1].graveyard.empty());

  Game discard;
  discard.players[0].leader=leader("eredin_destroyer"); discard.players[0].leaderAbilityAvailable=true;
  discard.players[0].hand={unit("discard-a",1),unit("discard-b",2)};
  discard.players[0].deck={unit("draw-a",3),unit("draw-b",4)};
  assert(activateLeader(discard,0,Row::None,{"discard-a","discard-b","draw-a"}));
  assert(discard.players[0].hand.size()==1 && discard.players[0].hand[0].id=="draw-a");
  assert(discard.players[0].graveyard.size()==2 && discard.players[0].deck.size()==1);

  Game crach;
  crach.players[0].leader=leader("crach_an_craite"); crach.players[0].leaderAbilityAvailable=true;
  crach.players[0].graveyard.push_back(unit("own-grave",1));
  crach.players[1].graveyard.push_back(unit("enemy-grave",1));
  assert(activateLeader(crach,0));
  assert(crach.players[0].deck.size()==1 && crach.players[1].deck.size()==1);
  assert(crach.players[0].graveyard.empty() && crach.players[1].graveyard.empty());
}
int main() { scoreRules(); turnsAndRounds(); abilitiesAndValidation(); openingHandAndMulligan(); referenceCatalog(); aiUsesNoHiddenCardsAndCompletesMatches(); avengerCards(); leaderAbilities(); }
