#include "gwent/game.hpp"
#include "gwent/catalog.hpp"
#include "gwent/ai.hpp"
#include "gwent/save.hpp"
#include "gwent/deck.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
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
  auto a = referenceCard("Blue Stripes Commando"); a.id="bond-a";
  auto b = referenceCard("Blue Stripes Commando"); b.id="bond-b";
  auto c = referenceCard("Blue Stripes Commando"); c.id="bond-c";
  bond.board[0] = {a,b,c};
  assert(calculateScore(bond, {false,false,false}, {false,false,false}) == 24); // three same-name copies each score 4 x 2

  Player distinctBondNames;
  auto differentA=unit("bond-a",4); differentA.name="Blue Stripes Commando"; differentA.bond=true; differentA.group="shared";
  auto differentB=unit("bond-b",4); differentB.name="Blue Stripes Scout"; differentB.bond=true; differentB.group="shared";
  distinctBondNames.board[0]={differentA,differentB};
  assert(calculateScore(distinctBondNames,{false,false,false},{false,false,false})==8); // shared metadata alone does not create a bond

  Player sameNameDifferentGroups;
  auto sameNameA=unit("bond-a",4); sameNameA.name="Blue Stripes Commando"; sameNameA.bond=true; sameNameA.group="one";
  auto sameNameB=unit("bond-b",4); sameNameB.name="Blue Stripes Commando"; sameNameB.bond=true; sameNameB.group="two";
  sameNameDifferentGroups.board[0]={sameNameA,sameNameB};
  assert(calculateScore(sameNameDifferentGroups,{false,false,false},{false,false,false})==16); // printed name, not catalog group, defines the bond

  Player orderedEffects;
  auto bondedA=referenceCard("Blue Stripes Commando"); bondedA.id="bonded-a";
  auto bondedB=referenceCard("Blue Stripes Commando"); bondedB.id="bonded-b";
  auto rally=unit("rally",1); rally.name="Morale unit"; rally.morale=true;
  orderedEffects.board[0]={bondedA,bondedB,rally};
  assert(calculateScore(orderedEffects,{false,false,false},{false,false,false})==19); // Bond multiplies before Morale is added.
}
static void duplicateWeatherIsDiscarded() {
  Card frost; frost.id="frost-first"; frost.name="Biting Frost"; frost.kind=Kind::Weather; frost.abilities={"frost"};
  Card duplicate=frost; duplicate.id="frost-duplicate";
  Card clear; clear.id="clear-weather"; clear.name="Clear Weather"; clear.kind=Kind::ClearWeather;
  Game game;
  game.players[0].hand={frost,clear,unit("clear-action-remains",1)};
  game.players[1].hand={duplicate,unit("duplicate-action-remains",1)};
  assert(playCard(game,0,"frost-first"));
  assert(game.weather[0] && game.weatherCards.size()==1);
  assert(playCard(game,1,"frost-duplicate"));
  assert(game.weather[0] && game.weatherCards.size()==1);
  assert(game.players[1].graveyard.size()==1 && game.players[1].graveyard[0].id=="frost-duplicate");
  assert(playCard(game,0,"clear-weather"));
  assert(!game.weather[0] && game.weatherCards.empty());
  assert(std::any_of(game.players[0].graveyard.begin(),game.players[0].graveyard.end(),[](const Card& card){return card.id=="frost-first";}));
}
static void turnsAndRounds() {
  Game g; g.players[0].hand={unit("a", 8),unit("a-spare",1)}; g.players[1].hand={unit("b",3),unit("b-spare",1)};
  assert(playCard(g,0,"a")); assert(g.currentPlayer==1);
  assert(pass(g,1)); assert(g.currentPlayer==0 && !g.players[0].passed);
  assert(pass(g,0)); assert(g.phase==Phase::RoundResult);
  assert(g.players[0].roundWins==1 && g.players[1].roundLosses==1);
  assert(startNextRound(g)); assert(g.currentPlayer==0); // the previous round's winner starts
  assert(!startNextRound(g));

  Game tie; tie.players[0].hand={unit("ta",2),unit("ta-spare",1)}; tie.players[1].hand={unit("tb",2),unit("tb-spare",1)};
  assert(playCard(tie,0,"ta")); assert(playCard(tie,1,"tb"));
  assert(pass(tie,0)); assert(pass(tie,1));
  assert(tie.phase==Phase::RoundResult && tie.players[0].roundLosses==1 && tie.players[1].roundLosses==1);

  Game nilfgaard; nilfgaard.players[0].faction=Faction::Nilfgaard;
  nilfgaard.players[0].hand={unit("na",2),unit("na-spare",1)}; nilfgaard.players[1].hand={unit("nb",2),unit("nb-spare",1)};
  assert(playCard(nilfgaard,0,"na")); assert(playCard(nilfgaard,1,"nb"));
  assert(pass(nilfgaard,0)); assert(pass(nilfgaard,1));
  assert(nilfgaard.players[0].roundWins==1 && nilfgaard.players[1].roundLosses==1);
}
static void sourceAutomaticPassing() {
  Game exhausted;
  exhausted.players[0].hand.push_back(unit("last-human-action",4));
  exhausted.players[1].hand.push_back(unit("last-opponent-action",3));
  assert(playCard(exhausted,0,"last-human-action"));
  assert(exhausted.players[0].passed&&exhausted.currentPlayer==1&&exhausted.phase==Phase::OpponentTurn);
  assert(playCard(exhausted,1,"last-opponent-action"));
  assert(exhausted.players[1].passed&&exhausted.phase==Phase::RoundResult);

  Game emptyOpening;
  emptyOpening.phase=Phase::Mulligan;
  assert(replaceMulliganCards(emptyOpening,0,{}));
  assert(replaceMulliganCards(emptyOpening,1,{}));
  assert(emptyOpening.players[0].passed&&emptyOpening.players[1].passed);
  assert(emptyOpening.phase==Phase::RoundResult);

  Game oneSidedOpening;oneSidedOpening.phase=Phase::Mulligan;oneSidedOpening.firstPlayer=0;
  oneSidedOpening.players[1].hand.push_back(unit("only-opponent-action",2));
  assert(replaceMulliganCards(oneSidedOpening,0,{}));
  assert(replaceMulliganCards(oneSidedOpening,1,{}));
  assert(oneSidedOpening.players[0].passed&&!oneSidedOpening.players[1].passed);
  assert(oneSidedOpening.currentPlayer==1&&oneSidedOpening.phase==Phase::OpponentTurn);

  Game oncePerMatch;
  oncePerMatch.players[0].leader=referenceCard("Foltest - The Siegemaster");
  oncePerMatch.players[0].leaderAbilityAvailable=true;
  oncePerMatch.players[0].hand={unit("leader-round-one",1),unit("leader-round-one-spare",1)};
  oncePerMatch.players[1].hand={unit("leader-round-one-opponent",1),unit("leader-round-one-opponent-spare",1)};
  assert(activateLeader(oncePerMatch,0));
  assert(!oncePerMatch.players[0].leaderAbilityAvailable);
  assert(pass(oncePerMatch,1)&&pass(oncePerMatch,0));
  assert(oncePerMatch.phase==Phase::RoundResult&&startNextRound(oncePerMatch));
  assert(!oncePerMatch.players[0].leaderAbilityAvailable);

  Game lastLeaderAction;
  lastLeaderAction.players[0].leader=referenceCard("Foltest - The Siegemaster");
  lastLeaderAction.players[0].leaderAbilityAvailable=true;
  lastLeaderAction.players[1].hand.push_back(unit("after-leader-action",1));
  assert(activateLeader(lastLeaderAction,0));
  assert(lastLeaderAction.players[0].passed&&lastLeaderAction.currentPlayer==1);
}
static void factionStartAndSkelligeRoundThree() {
  std::vector<Card> deck;
  for(int i=0;i<24;++i)deck.push_back(unit("scoia-deck-"+std::to_string(i),1));
  Card scoiaLeader0; scoiaLeader0.id="scoia-leader-0"; scoiaLeader0.kind=Kind::Leader; scoiaLeader0.faction=Faction::Scoiatael;
  Card scoiaLeader1; scoiaLeader1.id="scoia-leader-1"; scoiaLeader1.kind=Kind::Leader; scoiaLeader1.faction=Faction::NorthernRealms;
  Game scoiatael;
  beginMatchWithLeaders(scoiatael,deck,scoiaLeader0,deck,scoiaLeader1,0x12345,1);
  assert(scoiatael.scoiataelChoicePending && scoiatael.firstPlayer==0);
  assert(!replaceMulliganCards(scoiatael,0,{}));
  assert(chooseScoiataelFirstPlayer(scoiatael,1));
  assert(!scoiatael.scoiataelChoicePending && scoiatael.firstPlayer==1);
  assert(!chooseScoiataelFirstPlayer(scoiatael,0));

  Card ordinaryLeader; ordinaryLeader.id="nr-leader"; ordinaryLeader.kind=Kind::Leader; ordinaryLeader.faction=Faction::NorthernRealms;
  Card aiScoiaLeader; aiScoiaLeader.id="ai-scoia-leader"; aiScoiaLeader.kind=Kind::Leader; aiScoiaLeader.faction=Faction::Scoiatael;
  Game aiScoia;
  beginMatchWithLeaders(aiScoia,deck,ordinaryLeader,deck,aiScoiaLeader,0x12345,0);
  assert(!aiScoia.scoiataelChoicePending && (aiScoia.firstPlayer==0||aiScoia.firstPlayer==1));
  Game mirrorScoia;
  beginMatchWithLeaders(mirrorScoia,deck,scoiaLeader0,deck,aiScoiaLeader,0x12345,0);
  assert(!mirrorScoia.scoiataelChoicePending); // The reference skips faction perks when both decks are Scoia'tael.

  Game northern;
  northern.phase=Phase::RoundResult; northern.players[0].faction=Faction::NorthernRealms;
  northern.players[0].score=8; northern.players[1].score=2;
  northern.players[0].deck.push_back(unit("northern-draw",2));
  assert(startNextRound(northern));
  assert(northern.players[0].hand.size()==1 && northern.players[0].deck.empty());

  Game monsters;
  monsters.phase=Phase::RoundResult; monsters.players[0].faction=Faction::Monsters;
  monsters.players[0].score=8; monsters.players[1].score=2;
  monsters.players[0].board[0]={unit("monster-1",2),unit("monster-2",3)};
  monsters.players[0].board[1].push_back(unit("monster-3",4,Row::Ranged));
  assert(startNextRound(monsters));
  int retained=0;
  for(const auto& row:monsters.players[0].board)for(const auto& card:row)if(card.kind==Kind::Unit&&!card.hero)++retained;
  assert(retained==1 && monsters.players[0].graveyard.size()==2);

  Game skellige;
  skellige.phase=Phase::RoundResult; skellige.round=2; skellige.firstPlayer=0;
  skellige.players[0].faction=Faction::Skellige;
  skellige.players[0].hand.push_back(unit("blocked-during-setup",1));
  auto agile=unit("skellige-agile",7,Row::None); agile.agile=true;
  auto agile2=unit("skellige-agile-2",4,Row::None); agile2.agile=true;
  skellige.players[0].graveyard={agile,agile2};
  assert(startNextRound(skellige));
  assert(skellige.round==3 && skellige.phase==Phase::RoundSetup && skellige.skelligeRevivesRemaining==2);
  assert(!playCard(skellige,0,"blocked-during-setup") && !pass(skellige,0));
  assert(!chooseSkelligeRevive(skellige,0,"skellige-agile",Row::None));
  const auto chosenAgile=skellige.skelligeAgileReviveIds.front();
  assert(!chooseSkelligeRevive(skellige,0,"not-a-selected-card",Row::Ranged));
  assert(chooseSkelligeRevive(skellige,0,chosenAgile,Row::Ranged));
  assert(skellige.skelligeRevivesRemaining==1 && skellige.players[0].board[1].size()==1);
  const auto secondAgile=skellige.skelligeAgileReviveIds.front();
  assert(chooseSkelligeRevive(skellige,0,secondAgile,Row::Melee));
  assert(skellige.skelligeRevivesRemaining==0 && skellige.phase==Phase::PlayerTurn);
  assert(skellige.players[0].board[0].size()==1 && skellige.players[0].graveyard.empty());

  Game aiSkellige;
  aiSkellige.phase=Phase::RoundResult; aiSkellige.round=2; aiSkellige.players[1].faction=Faction::Skellige;
  aiSkellige.players[0].hand.push_back(unit("human-spare",1));
  aiSkellige.players[1].hand.push_back(unit("ai-spare",1));
  aiSkellige.players[1].graveyard={unit("ai-skellige-1",3),unit("ai-skellige-2",4,Row::Ranged),unit("ai-skellige-3",5,Row::Siege)};
  assert(startNextRound(aiSkellige));
  assert(aiSkellige.phase==Phase::PlayerTurn && aiSkellige.players[1].graveyard.size()==1);
  int aiRetained=0;
  for(const auto& row:aiSkellige.players[1].board)aiRetained+=static_cast<int>(row.size());
  assert(aiRetained==2);
}
static void saveRoundTripAndCorruptionHandling() {
  Game original;
  original.phase=Phase::RoundSetup; original.currentPlayer=1; original.firstPlayer=1; original.round=3;
  original.weather={true,false,true}; original.horn[1]={false,true,false};
  original.randomRespawn=true; original.scoiataelChoicePending=false;
  original.skelligeRevivesRemaining=1; original.skelligeRevivePlayer=0; original.skelligeAgileReviveIds={"save-agile"}; original.factionAbilitiesInitialized=true;
  original.players[1].mardroeme[2]=true;
  original.players[1].specialOccupied[1]=true;
  original.rngState=0x12345678; original.lastError="persisted diagnostic";
  auto card=unit("save-card",9,Row::None); card.agile=true; card.spy=true; card.group="save-group";
  card.abilities={"spy","agile","morale"}; card.referenceIndex=42; card.deckCopies=3;
  original.players[0].hand={card}; original.players[0].deck={unit("save-deck",4)};
  original.players[0].graveyard={unit("save-grave",7)};auto saveAgile=unit("save-agile",5,Row::None);saveAgile.agile=true;original.players[0].graveyard.push_back(saveAgile);original.players[0].board[1]={unit("save-board",6,Row::Ranged)};
  original.players[0].leader.id="save-leader"; original.players[0].leader.kind=Kind::Leader;
  original.players[0].faction=Faction::Skellige;
  original.players[0].leaderAbilityAvailable=true; original.players[0].roundWins=1; original.players[0].roundLosses=1;
  Card weather; weather.id="save-weather"; weather.name="Skellige Storm"; weather.kind=Kind::Weather; weather.abilities={"storm","rain","fog"};
  original.weatherCards={{weather,1}}; original.revealedCards[0]={card};
  const auto path=(std::filesystem::temp_directory_path()/"gwent-classic-save-test.gws").string();
  std::error_code ignored;std::filesystem::remove(path,ignored);std::filesystem::remove(path+".tmp",ignored);
  std::string error;
  assert(saveGame(path,original,&error));
  Game restored;
  assert(loadGame(path,restored,&error));
  assert(restored.phase==original.phase && restored.currentPlayer==1 && restored.firstPlayer==1 && restored.round==3);
  assert(restored.players[0].hand.size()==1 && restored.players[0].hand[0].id=="save-card");
  assert(restored.players[0].hand[0].abilities==card.abilities && restored.players[0].hand[0].referenceIndex==42);
  assert(restored.players[0].board[1][0].id=="save-board" && restored.players[0].leader.id=="save-leader");
  assert(restored.weather==original.weather && restored.horn==original.horn && restored.weatherCards[0].card.name=="Skellige Storm");
  assert(restored.skelligeRevivesRemaining==1 && restored.skelligeRevivePlayer==0 && restored.skelligeAgileReviveIds==original.skelligeAgileReviveIds && restored.rngState==original.rngState);
  assert(restored.players[1].mardroeme[2]);
  assert(restored.players[1].specialOccupied[1]);
  {
    std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);
    file.seekg(-1,std::ios::end);char byte=0;file.read(&byte,1);byte=static_cast<char>(byte^0x5a);file.seekp(-1,std::ios::end);file.write(&byte,1);
  }
  Game preserved;preserved.round=2;preserved.lastError="must remain intact";
  assert(!loadGame(path,preserved,&error));
  assert(preserved.round==2 && preserved.lastError=="must remain intact");
  std::filesystem::remove(path,ignored);std::filesystem::remove(path+".tmp",ignored);
}
static void userDeckPersistenceAndValidation() {
  const auto& source=referencePremadeDecks().front();
  UserDeck deck{"Geralt - talia testowa",source.faction,source.leaderReferenceIndex,source.cardCounts};
  assert(validateUserDeck(deck).empty());
  std::vector<Card> cards;Card leaderCard;std::string error;
  assert(materializeUserDeck(deck,cards,leaderCard,&error));
  const auto expectedCards=makeReferenceDeck(source.cardCounts);
  assert(cards.size()==expectedCards.size());
  for(std::size_t i=0;i<cards.size();++i)assert(cards[i].id==expectedCards[i].id);
  assert(leaderCard.kind==Kind::Leader&&leaderCard.referenceIndex==source.leaderReferenceIndex);
  const auto path=(std::filesystem::temp_directory_path()/"gwent-classic-user-deck.gwd").string();
  std::error_code ignored;std::filesystem::remove(path,ignored);std::filesystem::remove(path+".tmp",ignored);
  assert(saveUserDeck(path,deck,&error));
  UserDeck restored{"sentinel",Faction::Nilfgaard,0,{}};
  assert(loadUserDeck(path,restored,&error));
  assert(restored.name==deck.name&&restored.faction==deck.faction&&restored.leaderReferenceIndex==deck.leaderReferenceIndex&&restored.cardCounts==deck.cardCounts);
  {
    std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);
    file.seekg(-1,std::ios::end);char byte=0;file.read(&byte,1);byte=static_cast<char>(byte^0x5a);file.seekp(-1,std::ios::end);file.write(&byte,1);
  }
  assert(!loadUserDeck(path,restored,&error));
  assert(restored.name==deck.name&&restored.cardCounts==deck.cardCounts);
  auto invalid=deck;invalid.cardCounts.push_back(invalid.cardCounts.front());
  assert(!validateUserDeck(invalid).empty());
  std::filesystem::remove(path,ignored);std::filesystem::remove(path+".tmp",ignored);
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

  Game agileMedic;auto am=unit("agile-medic",3);am.medic=true;agileMedic.players[0].hand.push_back(am);
  auto agileRevive=unit("agile-revive",7,Row::None);agileRevive.agile=true;agileRevive.abilities={"agile"};
  agileMedic.players[0].graveyard.push_back(agileRevive);
  assert(!playCard(agileMedic,0,"agile-medic",Row::None,"agile-revive"));
  assert(agileMedic.players[0].hand.size()==1&&agileMedic.players[0].graveyard.size()==1);
  assert(playCard(agileMedic,0,"agile-medic",Row::Ranged,"agile-revive"));
  assert(agileMedic.players[0].board[1].size()==1&&agileMedic.players[0].board[1][0].id=="agile-revive");
  assert(std::none_of(agileMedic.players[0].board[0].begin(),agileMedic.players[0].board[0].end(),
                      [](const Card& card){return card.id=="agile-revive";}));

  Game medicHero; auto mh=unit("medic-hero",2); mh.medic=true; medicHero.players[0].hand.push_back(mh);
  auto forbiddenHero=unit("forbidden-hero",10); forbiddenHero.hero=true;
  medicHero.players[0].graveyard.push_back(forbiddenHero);
  assert(!playCard(medicHero,0,"medic-hero",Row::None,"forbidden-hero"));

  Game muster; auto leader=unit("muster1",4); leader.muster=true; leader.group="crone";
  auto copy=unit("muster2",4,Row::Ranged); copy.muster=true; copy.group="crone";
  muster.players[0].hand.push_back(leader); muster.players[0].deck.push_back(copy);
  assert(playCard(muster,0,"muster1"));
  assert(muster.players[0].deck.empty() && muster.players[0].board[0].size()==1 && muster.players[0].board[1].size()==1);

  Game roachMuster;
  Card geralt = referenceCard("Geralt of Rivia");
  Card roach = referenceCard("Roach");
  roachMuster.players[0].hand.push_back(geralt);
  roachMuster.players[0].deck.push_back(roach);
  assert(playCard(roachMuster,0,geralt.id));
  assert(roachMuster.players[0].deck.empty());
  assert(std::any_of(roachMuster.players[0].board[0].begin(),roachMuster.players[0].board[0].end(),
                     [](const Card& card){return card.name=="Roach";}));

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
  std::vector<bool> observed(recipes.size(),false);
  for(std::uint64_t seed=1;seed<=500;++seed) {
    const auto opponent=makeReferenceOpponentDeck(seed);
    assert(opponent.sourceDeckIndex>=0&&opponent.sourceDeckIndex<static_cast<int>(recipes.size()));
    observed[static_cast<std::size_t>(opponent.sourceDeckIndex)]=true;
    const auto& recipe=recipes[static_cast<std::size_t>(opponent.sourceDeckIndex)];
    assert(opponent.leader.kind==Kind::Leader&&opponent.leader.faction==recipe.faction);
    assert(validateDeck(opponent.cards,opponent.leader,recipe.faction).empty());
    const auto repeat=makeReferenceOpponentDeck(seed);
    assert(repeat.sourceDeckIndex==opponent.sourceDeckIndex&&
           repeat.leader.referenceIndex==opponent.leader.referenceIndex&&
           repeat.cards.size()==opponent.cards.size());
  }
  assert(std::all_of(observed.begin(),observed.end(),[](bool value){return value;}));

  Game mardroeme; Card potion; potion.id="potion"; potion.kind=Kind::Mardroeme;
  mardroeme.players[0].hand={potion,unit("mardroeme-spare",1)};
  auto berserker=unit("bear-source",4); berserker.name="Berserker"; berserker.berserker=true;
  mardroeme.players[0].board[0].push_back(berserker);
  assert(playCard(mardroeme,0,"potion",Row::Melee));
  assert(mardroeme.players[0].board[0][0].name=="Transformed Vildkaarl");
  assert(mardroeme.players[0].board[0][0].power==14);
  assert(mardroeme.players[0].mardroeme[0]);
  auto laterBerserker=unit("later-berserker",4);laterBerserker.name="Berserker";laterBerserker.berserker=true;
  mardroeme.players[0].hand.push_back(laterBerserker);
  mardroeme.currentPlayer=0;mardroeme.phase=Phase::PlayerTurn;
  assert(playCard(mardroeme,0,"later-berserker"));
  assert(std::any_of(mardroeme.players[0].board[0].begin(),mardroeme.players[0].board[0].end(),
                     [](const Card& card){return card.id=="later-berserker"&&card.name=="Transformed Vildkaarl";}));
  Card secondPotion=potion;secondPotion.id="potion-duplicate";mardroeme.players[0].hand.push_back(secondPotion);
  mardroeme.currentPlayer=0;mardroeme.phase=Phase::PlayerTurn;
  assert(!playCard(mardroeme,0,"potion-duplicate",Row::Melee));

  Game mardroemeRound; mardroemeRound.phase=Phase::RoundResult;
  mardroemeRound.players[0].mardroeme[0]=true;mardroemeRound.players[0].specialOccupied[0]=true;
  assert(startNextRound(mardroemeRound));
  assert(!mardroemeRound.players[0].mardroeme[0]&&!mardroemeRound.players[0].specialOccupied[0]);

  Game ermion;auto enabler=unit("ermion",8,Row::Ranged);enabler.abilities={"mardroeme"};
  auto young=unit("young-berserker",8,Row::Ranged);young.name="Young Berserker";young.berserker=true;
  ermion.players[0].hand={enabler,unit("ermion-spare",1)};ermion.players[0].board[1].push_back(young);
  assert(playCard(ermion,0,"ermion"));
  assert(ermion.players[0].mardroeme[1]);
  assert(std::any_of(ermion.players[0].board[1].begin(),ermion.players[0].board[1].end(),
                     [](const Card& card){return card.id=="young-berserker"&&card.name=="Transformed Young Vildkaarl";}));
  Card rowMardroeme;rowMardroeme.id="row-mardroeme";rowMardroeme.kind=Kind::Mardroeme;
  ermion.players[0].hand.push_back(rowMardroeme);ermion.currentPlayer=0;ermion.phase=Phase::PlayerTurn;
  assert(playCard(ermion,0,"row-mardroeme",Row::Ranged));
  assert(ermion.players[0].mardroeme[1]&&ermion.players[0].specialOccupied[1]);
  Card rowHorn;rowHorn.id="row-horn";rowHorn.kind=Kind::CommanderHorn;ermion.players[0].hand.push_back(rowHorn);
  ermion.currentPlayer=0;ermion.phase=Phase::PlayerTurn;
  assert(!playCard(ermion,0,"row-horn",Row::Ranged));
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

  Game difficultyChoice;
  difficultyChoice.phase=Phase::OpponentTurn;difficultyChoice.currentPlayer=1;
  difficultyChoice.players[1].hand={unit("weak",1),unit("strong",10)};
  Game preview=difficultyChoice;
  const auto action=chooseAiAction(preview,1);
  assert(action.pass || action.activateLeader ||
         playCard(preview,1,action.cardId,action.target,action.selectedCardId));

  Game roachMulligan;roachMulligan.phase=Phase::Mulligan;
  roachMulligan.players[1].hand={referenceCard("Roach"),referenceCard("Geralt of Rivia"),unit("low-priority",2)};
  roachMulligan.players[1].deck={unit("replacement-a",4),unit("replacement-b",5)};
  const auto roachDiscard=chooseAiMulligan(roachMulligan,1);
  // ControllerAI.redraw() makes two sequential discardOrder() decisions; a
  // low-power Roach is eligible on the second redraw once weaker candidates
  // have moved through the simulated hand/deck swap.
  assert(std::find(roachDiscard.begin(),roachDiscard.end(),roachMulligan.players[1].hand[0].id)!=roachDiscard.end());

  Game leaderChoice;
  leaderChoice.players[0].leader.id="foltest_siegemaster";
  leaderChoice.players[0].leader.kind=Kind::Leader;
  leaderChoice.players[0].leader.abilities={"foltest_siegemaster"};
  leaderChoice.players[0].leaderAbilityAvailable=true;
  leaderChoice.players[0].board[2].push_back(unit("ai-siege",6,Row::Siege));
  leaderChoice.players[0].hand.push_back(unit("ai-small-play",1));
  const auto leaderAction=chooseAiAction(leaderChoice,0);
  assert(leaderAction.activateLeader || (!leaderAction.pass && leaderAction.cardId=="ai-small-play"));

  const auto& recipes=referencePremadeDecks();
  const auto deck0=makeReferenceDeck(recipes[0]);
  const auto deck1=makeReferenceDeck(recipes[2]);
  {
    Game game;
    beginMatch(game,deck0,recipes[0].faction,deck1,recipes[2].faction,0x987654,0);
    const auto aiMulligan=chooseAiMulligan(game,1);
    assert(aiMulligan.size()<=2);
    assert(replaceMulliganCards(game,0,{})); assert(replaceMulliganCards(game,1,aiMulligan));
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
  assert(!activateLeader(draw,0));
  assert(draw.players[0].hand.empty() && draw.players[1].graveyard.size()==1 && draw.players[0].leaderAbilityAvailable);
  assert(activateLeader(draw,0,Row::None,{"stolen-unit"}));
  assert(draw.players[0].hand.size()==1 && draw.players[1].graveyard.empty());

  Game reveal;
  reveal.players[0].leader=leader("emhyr_emperor"); reveal.players[0].leaderAbilityAvailable=true;
  reveal.players[1].hand={unit("known-a",1),unit("known-b",2),unit("known-c",3),unit("known-d",4)};
  assert(activateLeader(reveal,0));
  assert(reveal.revealedCards[0].size()==3 && reveal.players[1].hand.size()==4);
  std::vector<std::string> revealedIds;
  for(const auto& card:reveal.revealedCards[0])revealedIds.push_back(card.id);
  std::sort(revealedIds.begin(),revealedIds.end());
  assert(std::adjacent_find(revealedIds.begin(),revealedIds.end())==revealedIds.end());

  Game heroRestore;
  heroRestore.players[0].leader=leader("emhyr_relentless"); heroRestore.players[0].leaderAbilityAvailable=true;
  Card graveHero=unit("grave-hero",15);graveHero.hero=true;
  heroRestore.players[1].graveyard.push_back(graveHero);
  assert(activateLeader(heroRestore,0,Row::None,{"grave-hero"}));
  assert(heroRestore.players[0].hand.size()==1 && heroRestore.players[0].hand[0].hero);

  Game discard;
  discard.players[0].leader=leader("eredin_destroyer"); discard.players[0].leaderAbilityAvailable=true;
  discard.players[0].hand={unit("discard-a",1),unit("discard-b",2)};
  discard.players[0].deck={unit("draw-a",3),unit("draw-b",4)};
  assert(!activateLeader(discard,0));
  assert(discard.players[0].hand.size()==2 && discard.players[0].deck.size()==2 && discard.players[0].leaderAbilityAvailable);
  assert(activateLeader(discard,0,Row::None,{"discard-a","discard-b","draw-a"}));
  assert(discard.players[0].hand.size()==1 && discard.players[0].hand[0].id=="draw-a");
  assert(discard.players[0].graveyard.size()==2 && discard.players[0].deck.size()==1);

  Game kingWeather;
  kingWeather.players[0].leader=leader("eredin_king"); kingWeather.players[0].leaderAbilityAvailable=true;
  kingWeather.players[0].deck={referenceCard("Biting Frost"),referenceCard("Torrential Rain")};
  assert(!activateLeader(kingWeather,0));
  assert(kingWeather.players[0].deck.size()==2 && kingWeather.weatherCards.empty());
  assert(!activateLeader(kingWeather,0,Row::None,{"not-in-deck"}));
  assert(kingWeather.players[0].deck.size()==2 && kingWeather.weatherCards.empty());
  assert(activateLeader(kingWeather,0,Row::None,{kingWeather.players[0].deck[1].id}));
  assert(kingWeather.weather[2] && kingWeather.weatherCards.size()==1 && kingWeather.weatherCards[0].card.name=="Torrential Rain");

  Game crach;
  crach.players[0].leader=leader("crach_an_craite"); crach.players[0].leaderAbilityAvailable=true;
  crach.players[0].graveyard.push_back(unit("own-grave",1));
  crach.players[1].graveyard.push_back(unit("enemy-grave",1));
  assert(activateLeader(crach,0));
  assert(crach.players[0].deck.size()==1 && crach.players[1].deck.size()==1);
  assert(crach.players[0].graveyard.empty() && crach.players[1].graveyard.empty());
}
int main() { scoreRules(); duplicateWeatherIsDiscarded(); turnsAndRounds(); sourceAutomaticPassing(); factionStartAndSkelligeRoundThree(); saveRoundTripAndCorruptionHandling(); userDeckPersistenceAndValidation(); abilitiesAndValidation(); openingHandAndMulligan(); referenceCatalog(); aiUsesNoHiddenCardsAndCompletesMatches(); avengerCards(); leaderAbilities(); }
