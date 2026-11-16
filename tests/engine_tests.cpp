#include "gwent/game.hpp"
#include "gwent/catalog.hpp"
#include "gwent/ai.hpp"
#include "gwent/save.hpp"
#include "gwent/deck.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <utility>

using namespace gwent;
static Card unit(std::string id, int power, Row row = Row::Melee) {
  Card c; c.id = std::move(id); c.kind = Kind::Unit; c.power = power; c.row = row; return c;
}
static Card referenceCard(const std::string& name);
static Card leader(std::string id);
static std::uint32_t expectedRandom(std::uint64_t& state) {
  auto x=static_cast<std::uint32_t>(state);
  x^=x<<13;x^=x>>17;x^=x<<5;state=x;return x;
}
static bool expectedSourceCardBefore(const Card& left,const Card& right) {
  const auto rank=[](const Card& card) {
    if(card.kind==Kind::Weather)return -1;
    if(card.kind==Kind::ClearWeather||card.kind==Kind::Decoy||card.kind==Kind::Scorch||card.kind==Kind::CommanderHorn||card.kind==Kind::Mardroeme)return -2;
    return 0;
  };
  if(rank(left)!=rank(right))return rank(left)<rank(right);
  if(left.power!=right.power)return left.power<right.power;
  return left.name<right.name;
}
static void expectedSourceHandAdd(std::vector<Card>& hand,Card card,int owner,std::size_t requestedIndex) {
  if(owner==0&&requestedIndex==0) {
    const auto position=std::upper_bound(hand.begin(),hand.end(),card,expectedSourceCardBefore);
    hand.insert(position,std::move(card));
  } else if(owner==1&&requestedIndex==static_cast<std::size_t>(-1)) {
    hand.insert(hand.begin(),std::move(card));
  } else {
    const auto index=std::min(requestedIndex,hand.size());
    hand.insert(hand.begin()+static_cast<std::ptrdiff_t>(index),std::move(card));
  }
}
static void expectedSourceDeckSwap(std::vector<Card>& hand,std::vector<Card>& deck,
                                   const std::string& id,std::uint64_t& rngState,int owner=0) {
  auto found=std::find_if(hand.begin(),hand.end(),[&](const Card& card){return card.id==id;});
  assert(found!=hand.end());
  const auto index=static_cast<std::size_t>(found-hand.begin());
  Card discarded=*found;hand.erase(found);
  deck.push_back(discarded);
  const auto randomIndex=static_cast<std::size_t>(expectedRandom(rngState))%deck.size();
  std::swap(deck[randomIndex],deck.back());
  Card drawn=deck.front();deck.erase(deck.begin());
  expectedSourceHandAdd(hand,std::move(drawn),owner,index);
}
static bool matchCardIdsAreUnique(const Game& game) {
  std::vector<std::string> ids;
  const auto include=[&](const std::vector<Card>& cards) {
    for(const auto& card:cards) {
      if(card.id.empty()||std::find(ids.begin(),ids.end(),card.id)!=ids.end())return false;
      ids.push_back(card.id);
    }
    return true;
  };
  for(int owner=0;owner<2;++owner) {
    const auto& player=game.players[owner];
    if(!include(player.deck)||!include(player.hand)||!include(player.graveyard))return false;
    for(const auto& row:player.board)if(!include(row))return false;
    for(const auto& special:player.specialCards)
      if(!special.id.empty()&&!include(std::vector<Card>{special}))return false;
  }
  for(const auto& active:game.weatherCards)
    if(!include(std::vector<Card>{active.card}))return false;
  return true;
}
static bool matchContainsOriginalIds(const Game& game,const std::vector<std::string>& expected) {
  std::vector<std::string> actual;
  const auto collect=[&](const std::vector<Card>& cards) {
    for(const auto& card:cards)actual.push_back(card.id);
  };
  for(const auto& player:game.players) {
    collect(player.deck);collect(player.hand);collect(player.graveyard);
    for(const auto& row:player.board)collect(row);
    for(const auto& special:player.specialCards)if(!special.id.empty())actual.push_back(special.id);
  }
  for(const auto& active:game.weatherCards)actual.push_back(active.card.id);
  return std::all_of(expected.begin(),expected.end(),[&](const std::string& id){
    return std::count(actual.begin(),actual.end(),id)==1;
  });
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
  auto a = referenceCard("Blue Stripes Commando"); a.id="bond-a";
  auto b = referenceCard("Blue Stripes Commando"); b.id="bond-b";
  auto c = referenceCard("Blue Stripes Commando"); c.id="bond-c";
  Player singleBond;singleBond.board[0]={a};
  assert(calculateScore(singleBond,{false,false,false},{false,false,false})==4); // one source Bond card has no multiplier
  bond.board[0] = {a,b,c};
  assert(calculateScore(bond, {false,false,false}, {false,false,false}) == 36); // source multiplies each by the three-copy Bond count

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
  Player sourceBondCount;
  auto bondOne=unit("bond-one",4);bondOne.name="Bond Name";bondOne.bond=true;
  auto bondTwo=bondOne;bondTwo.id="bond-two";
  auto sameNameAlly=unit("same-name-ally",3);sameNameAlly.name="Bond Name";sameNameAlly.bond=false;
  sourceBondCount.board[0]={bondOne,bondTwo,sameNameAlly};
  assert(calculateScore(sourceBondCount,{false,false,false},{false,false,false})==22); // source row effect is keyed by name and applies its x2 count to each card with that name

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

  Game avengerSkellige;
  avengerSkellige.phase=Phase::RoundResult;avengerSkellige.round=2;
  avengerSkellige.players[0].faction=Faction::Skellige;
  avengerSkellige.players[0].hand.push_back(unit("avenger-skellige-spare",1));
  avengerSkellige.players[0].board[0]={referenceCard("Olaf"),referenceCard("Kambi")};
  assert(startNextRound(avengerSkellige));
  assert(avengerSkellige.phase==Phase::RoundSetup);
  assert(std::find(avengerSkellige.skelligeAgileReviveIds.begin(),avengerSkellige.skelligeAgileReviveIds.end(),
                   referenceCard("Olaf").id)!=avengerSkellige.skelligeAgileReviveIds.end());
  assert(std::none_of(avengerSkellige.players[0].board[0].begin(),avengerSkellige.players[0].board[0].end(),
                      [](const Card& card){return card.name=="Olaf";}));
  for(const auto& id:avengerSkellige.skelligeAgileReviveIds)
    assert(std::any_of(avengerSkellige.players[0].graveyard.begin(),avengerSkellige.players[0].graveyard.end(),
                       [&](const Card& card){return card.id==id&&card.agile;}));

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
  original.players[1].specialCards[1]=referenceCard("Mardroeme");
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
  assert(restored.players[1].specialCards[1].name=="Mardroeme");
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

  Game chosenDecoy;
  auto decoyCard=referenceCard("Decoy");decoyCard.id="chosen-decoy";
  auto existingHandCard=unit("hand-existing",6);
  chosenDecoy.players[0].hand={existingHandCard,decoyCard};
  auto low=unit("decoy-low",2);low.name="Low Unit";
  auto chosen=unit("decoy-chosen",5);chosen.name="Chosen Unit";
  auto high=unit("decoy-high",10);high.name="High Unit";
  chosenDecoy.players[0].board[0]={low,chosen,high};
  assert(playCard(chosenDecoy,0,"chosen-decoy",Row::None,"decoy-chosen"));
  const auto& decoyRow=chosenDecoy.players[0].board[0];
  assert(decoyRow.size()==3&&decoyRow[0].kind==Kind::Decoy&&
         decoyRow[1].id=="decoy-low"&&decoyRow[2].id=="decoy-high");
  assert(chosenDecoy.players[0].hand.size()==2&&chosenDecoy.players[0].hand[0].id=="decoy-chosen"&&
         chosenDecoy.players[0].hand[1].id=="hand-existing"); // source Hand re-sorts returned cards.

  Game decoyRejectsNonCombat;
  decoyRejectsNonCombat.players[0].hand.push_back(Card{"decoy-noncombat","Decoy",Kind::Decoy});
  auto nonCombat=unit("noncombat-decoy-target",5);nonCombat.row=Row::None;nonCombat.agile=false;
  decoyRejectsNonCombat.players[0].board[0].push_back(nonCombat);
  assert(!playCard(decoyRejectsNonCombat,0,"decoy-noncombat",Row::None,nonCombat.id));
  assert(decoyRejectsNonCombat.players[0].hand.size()==1&&decoyRejectsNonCombat.players[0].board[0].size()==1);

  Game decoyRejectsOpponentCard;
  auto opponentOnly=unit("opponent-decoy-target",7);
  decoyRejectsOpponentCard.players[0].hand.push_back(referenceCard("Decoy"));
  decoyRejectsOpponentCard.players[1].board[1].push_back(opponentOnly);
  assert(!playCard(decoyRejectsOpponentCard,0,decoyRejectsOpponentCard.players[0].hand[0].id,
                   Row::None,opponentOnly.id));
  assert(decoyRejectsOpponentCard.players[0].hand.size()==1&&
         decoyRejectsOpponentCard.players[1].board[1].size()==1&&
         decoyRejectsOpponentCard.players[1].board[1][0].id==opponentOnly.id);

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

  Game randomMedic;randomMedic.randomRespawn=true;randomMedic.rngState=0x2468;
  auto randomMedicCard=unit("random-medic",3);randomMedicCard.medic=true;
  randomMedic.players[0].hand={randomMedicCard};
  auto randomPlain=unit("random-plain",4);auto randomAgile=unit("random-agile",5,Row::None);randomAgile.agile=true;
  randomMedic.players[0].graveyard={randomPlain,randomAgile};
  const auto rngBeforePreview=randomMedic.rngState;
  const auto previewId=previewRandomMedicTargetId(randomMedic,0);
  assert(!previewId.empty()&&randomMedic.rngState==rngBeforePreview);
  const auto preview=std::find_if(randomMedic.players[0].graveyard.begin(),randomMedic.players[0].graveyard.end(),
                                 [&](const Card& card){return card.id==previewId;});
  assert(preview!=randomMedic.players[0].graveyard.end());
  const Row randomAgileRow=preview->agile?Row::Ranged:Row::None;
  const int randomExpectedRow=preview->agile?1:0;
  assert(playCard(randomMedic,0,"random-medic",randomAgileRow,previewId));
  assert(std::any_of(randomMedic.players[0].board[randomExpectedRow].begin(),randomMedic.players[0].board[randomExpectedRow].end(),
                     [&](const Card& card){return card.id==previewId;}));

  Game medicHero; auto mh=unit("medic-hero",2); mh.medic=true; medicHero.players[0].hand.push_back(mh);
  auto forbiddenHero=unit("forbidden-hero",10); forbiddenHero.hero=true;
  medicHero.players[0].graveyard.push_back(forbiddenHero);
  assert(!playCard(medicHero,0,"medic-hero",Row::None,"forbidden-hero"));
  Game medicNonCombat;auto mn=unit("medic-noncombat",2);mn.medic=true;medicNonCombat.players[0].hand.push_back(mn);
  auto excluded=unit("medic-noncombat-target",8);excluded.row=Row::None;excluded.agile=false;
  medicNonCombat.players[0].graveyard.push_back(excluded);
  assert(!playCard(medicNonCombat,0,"medic-noncombat",Row::None,excluded.id));

  Game muster; auto leader=unit("muster1",4); leader.name="Crone - Brewess";leader.muster=true;leader.group="Crone";
  auto copy=unit("muster2",4,Row::Ranged);copy.name="Crone - Whispess";copy.muster=true;copy.group="Crone";
  muster.players[0].hand.push_back(leader); muster.players[0].deck.push_back(copy);
  assert(playCard(muster,0,"muster1"));
  assert(muster.players[0].deck.empty() && muster.players[0].board[0].size()==1 && muster.players[0].board[1].size()==1);

  Game sortedMuster;
  auto brewess=unit("brewess",6);brewess.name="Crone - Brewess";brewess.muster=true;brewess.group="Crone";
  auto whispess=unit("whispess",9);whispess.name="Crone - Whispess";whispess.muster=true;whispess.group="Crone";
  auto weavess=unit("weavess",2);weavess.name="Crone - Weavess";weavess.muster=true;weavess.group="Crone";
  sortedMuster.players[0].hand={brewess,whispess};sortedMuster.players[0].deck={weavess};
  assert(playCard(sortedMuster,0,"brewess"));
  const auto& sortedRow=sortedMuster.players[0].board[0];
  assert(sortedRow.size()==3&&sortedRow[0].id=="weavess"&&sortedRow[1].id=="brewess"&&sortedRow[2].id=="whispess");

  const auto arachasCopies=std::count_if(referenceCardCatalog().begin(),referenceCardCatalog().end(),
      [](const Card& card){return card.name=="Arachas"&&card.muster;});
  assert(arachasCopies==3);
  Game arachasMuster;
  const auto behemoth=referenceCard("Arachas- Behemoth");
  assert(behemoth.group=="Arachas");
  arachasMuster.players[0].hand={behemoth,referenceCard("Arachas")};
  arachasMuster.players[0].deck={referenceCard("Arachas"),referenceCard("Arachas")};
  arachasMuster.players[0].hand[1].id="arachas-hand-copy";
  arachasMuster.players[0].deck[0].id="arachas-deck-copy-a";
  arachasMuster.players[0].deck[1].id="arachas-deck-copy-b";
  assert(playCard(arachasMuster,0,behemoth.id));
  assert(arachasMuster.players[0].hand.empty()&&arachasMuster.players[0].deck.empty());
  assert(arachasMuster.players[0].board[0].size()==3&&arachasMuster.players[0].board[2].size()==1);
  assert(std::all_of(arachasMuster.players[0].board[0].begin(),arachasMuster.players[0].board[0].end(),
                     [](const Card& card){return card.name=="Arachas";}));

  Game roachMuster;
  Card geralt = referenceCard("Geralt of Rivia");
  Card ciri = referenceCard("Cirilla Fiona Elen Riannon");
  Card roach = referenceCard("Roach");
  roachMuster.players[0].hand.push_back(geralt);
  roachMuster.players[0].hand.push_back(ciri);
  roachMuster.players[0].deck.push_back(roach);
  assert(playCard(roachMuster,0,geralt.id));
  assert(roachMuster.players[0].deck.empty() && roachMuster.players[0].hand.size()==1 &&
         roachMuster.players[0].hand.front().name=="Cirilla Fiona Elen Riannon");
  assert(std::any_of(roachMuster.players[0].board[0].begin(),roachMuster.players[0].board[0].end(),
                     [](const Card& card){return card.name=="Roach";}));
  assert(std::none_of(roachMuster.players[0].board[0].begin(),roachMuster.players[0].board[0].end(),
                      [](const Card& card){return card.name=="Cirilla Fiona Elen Riannon";}));

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
  assert(horns.players[0].graveyard.empty()&&horns.players[0].specialCards[0].id=="horn");

  Game specialCrach;auto activeHorn=referenceCard("Commander's Horn");activeHorn.id="active-horn";
  specialCrach.players[0].hand={activeHorn};specialCrach.players[0].graveyard={unit("old-grave-unit",3)};
  Card crachLeader;crachLeader.id="crach_an_craite";crachLeader.kind=Kind::Leader;
  crachLeader.abilities={"crach_an_craite"};
  specialCrach.players[0].leader=crachLeader;specialCrach.players[0].leaderAbilityAvailable=true;
  assert(playCard(specialCrach,0,activeHorn.id,Row::Melee));
  assert(specialCrach.players[0].specialCards[0].id==activeHorn.id&&specialCrach.players[0].graveyard.size()==1);
  specialCrach.currentPlayer=0;specialCrach.phase=Phase::PlayerTurn;
  assert(activateLeader(specialCrach,0));
  assert(specialCrach.players[0].deck.size()==1&&specialCrach.players[0].deck[0].id=="old-grave-unit");
  assert(specialCrach.players[0].specialCards[0].id==activeHorn.id);
  specialCrach.phase=Phase::RoundResult;
  assert(startNextRound(specialCrach));
  assert(specialCrach.players[0].specialCards[0].id.empty()&&
         std::any_of(specialCrach.players[0].graveyard.begin(),specialCrach.players[0].graveyard.end(),
                     [&](const Card& card){return card.id==activeHorn.id;}));

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
static void sourceMulliganSwapSemantics() {
  Game human;human.phase=Phase::Mulligan;human.rngState=0x5a17;
  human.players[0].hand={unit("human-redraw-a",2),unit("human-keep",10),unit("human-redraw-b",3)};
  human.players[0].deck={unit("human-deck-a",4),unit("human-deck-b",5),unit("human-deck-c",6)};
  assert(!replaceMulliganCards(human,0,{"human-redraw-a","human-keep","human-redraw-b"}));
  assert(human.players[0].hand.size()==3&&human.players[0].deck.size()==3&&!human.players[0].mulliganDone);
  auto expectedHand=human.players[0].hand;auto expectedDeck=human.players[0].deck;auto expectedState=human.rngState;
  expectedSourceDeckSwap(expectedHand,expectedDeck,"human-redraw-a",expectedState);
  expectedSourceDeckSwap(expectedHand,expectedDeck,"human-redraw-b",expectedState);
  assert(replaceMulliganCards(human,0,{"human-redraw-a","human-redraw-b"}));
  assert(human.players[0].hand.size()==expectedHand.size()&&human.players[0].deck.size()==expectedDeck.size());
  for(std::size_t i=0;i<expectedHand.size();++i)assert(human.players[0].hand[i].id==expectedHand[i].id);
  for(std::size_t i=0;i<expectedDeck.size();++i)assert(human.players[0].deck[i].id==expectedDeck[i].id);
  assert(human.rngState==expectedState);

  Game ai;ai.phase=Phase::Mulligan;ai.rngState=0x6b28;
  ai.players[1].hand={unit("ai-redraw",1)};
  ai.players[1].deck={unit("ai-deck-a",2),unit("ai-deck-b",3)};
  auto expectedAiHand=ai.players[1].hand;auto expectedAiDeck=ai.players[1].deck;auto expectedAiState=ai.rngState;
  expectedSourceDeckSwap(expectedAiHand,expectedAiDeck,"ai-redraw",expectedAiState,1);
  expectedSourceDeckSwap(expectedAiHand,expectedAiDeck,expectedAiHand[0].id,expectedAiState,1);
  assert(performAiMulligan(ai,1));
  assert(ai.players[1].hand.size()==1&&ai.players[1].deck.size()==2);
  assert(ai.players[1].hand[0].id==expectedAiHand[0].id);
  for(std::size_t i=0;i<expectedAiDeck.size();++i)assert(ai.players[1].deck[i].id==expectedAiDeck[i].id);
  assert(ai.rngState==expectedAiState);
}
static void referenceCatalog() {
  const auto& catalog = referenceCardCatalog();
  assert(catalog.size() == 258);
  assert(std::count_if(catalog.begin(), catalog.end(), [](const Card& c){return c.kind==Kind::Leader;}) >= 20);
  auto geralt = std::find_if(catalog.begin(), catalog.end(), [](const Card& c){return c.name=="Geralt of Rivia";});
  assert(geralt != catalog.end() && geralt->muster && geralt->group=="Roach");
  assert(referenceCardName(geralt->referenceIndex)=="Geralt of Rivia");
  assert(referenceCardName(geralt->referenceIndex,true)=="Geralt z Rivii");
  assert(!referenceCardAbilityTitle(geralt->referenceIndex,true).empty());
  assert(referenceCardDescription(geralt->referenceIndex,true).find("Wyszukuje")!=std::string::npos);
  for(const auto& card:catalog) {
    assert(!referenceCardName(card.referenceIndex).empty());
    assert(!referenceCardName(card.referenceIndex,true).empty());
  }
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
    const UserDeck defaultSelection{"Deck 1",recipe.faction,recipe.leaderReferenceIndex,recipe.cardCounts};
    assert(validateUserDeck(defaultSelection).empty());
    std::vector<Card> selectedCards;Card selectedLeader;
    assert(materializeUserDeck(defaultSelection,selectedCards,selectedLeader));
    assert(selectedCards.size()==premade.size()&&selectedLeader.referenceIndex==chosenLeader.referenceIndex);
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
  mardroeme.players[0].board[0].push_back(unit("after-berserker",9));
  assert(playCard(mardroeme,0,"potion",Row::Melee));
  assert(mardroeme.players[0].board[0][0].id=="after-berserker");
  assert(mardroeme.players[0].board[0][1].name=="Transformed Vildkaarl");
  assert(mardroeme.players[0].board[0][1].power==14);
  assert(std::find(mardroeme.players[0].board[0][1].abilities.begin(),
                   mardroeme.players[0].board[0][1].abilities.end(),"vildkarrl") ==
         mardroeme.players[0].board[0][1].abilities.end());
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
                     [](const Card& card){return card.id=="young-berserker"&&card.name=="Transformed Young Vildkaarl"&&
                       std::find(card.abilities.begin(),card.abilities.end(),"vildkarrl")==card.abilities.end();}));
  Card rowMardroeme;rowMardroeme.id="row-mardroeme";rowMardroeme.kind=Kind::Mardroeme;
  ermion.players[0].hand.push_back(rowMardroeme);ermion.currentPlayer=0;ermion.phase=Phase::PlayerTurn;
  assert(playCard(ermion,0,"row-mardroeme",Row::Ranged));
  assert(ermion.players[0].mardroeme[1]&&ermion.players[0].specialOccupied[1]);
  assert(ermion.players[0].specialCards[1].id=="row-mardroeme"&&ermion.players[0].graveyard.empty());
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

  Game clearWeatherAi;
  clearWeatherAi.phase=Phase::OpponentTurn;clearWeatherAi.currentPlayer=1;
  clearWeatherAi.weather[0]=true;
  clearWeatherAi.players[1].board[0].push_back(unit("weathered-ai-unit",8));
  Card clear;clear.id="ai-clear-weather";clear.name="Clear Weather";clear.kind=Kind::ClearWeather;
  clearWeatherAi.players[1].hand={clear,unit("ai-alternative",1,Row::Siege)};
  int clearWeatherChoices=0;
  for(std::uint64_t seed=1;seed<=512;++seed) {
    auto sample=clearWeatherAi;sample.rngState=seed*0x9e3779b9u;
    const auto choice=chooseAiAction(sample,1);
    if(choice.cardId==clear.id)++clearWeatherChoices;
  }
  // Clear Weather benefits the AI's own weathered unit by seven points and
  // must outweigh the one-point fallback. The old reversed delta chose it
  // zero times because its (incorrect) weight was clipped to zero.
  assert(clearWeatherChoices>350&&clearWeatherChoices<510);

  Game enemyBoardSpyDecoy;
  enemyBoardSpyDecoy.phase=Phase::OpponentTurn;enemyBoardSpyDecoy.currentPlayer=1;
  Card enemySpy=unit("enemy-board-spy",4);enemySpy.spy=true;enemySpy.abilities={"spy"};
  enemyBoardSpyDecoy.players[0].board[0].push_back(enemySpy);
  enemyBoardSpyDecoy.players[1].board[0].push_back(unit("ai-decoy-target",2));
  Card boardSpyDecoy;boardSpyDecoy.id="board-spy-decoy";boardSpyDecoy.name="Decoy";boardSpyDecoy.kind=Kind::Decoy;
  enemyBoardSpyDecoy.players[1].hand={boardSpyDecoy,unit("board-spy-decoy-alternative",1,Row::Siege)};
  int boardSpyDecoyChoices=0;
  for(std::uint64_t seed=1;seed<=256;++seed) {
    auto sample=enemyBoardSpyDecoy;sample.rngState=seed*0xc2b2ae35u;
    if(chooseAiAction(sample,1).cardId==boardSpyDecoy.id)++boardSpyDecoyChoices;
  }
  Game noEnemyBoardSpyDecoy=enemyBoardSpyDecoy;
  noEnemyBoardSpyDecoy.players[0].board[0].clear();
  int noEnemySpyDecoyChoices=0;
  for(std::uint64_t seed=1;seed<=256;++seed) {
    auto sample=noEnemyBoardSpyDecoy;sample.rngState=seed*0xc2b2ae35u;
    if(chooseAiAction(sample,1).cardId==boardSpyDecoy.id)++noEnemySpyDecoyChoices;
  }
  // The pinned source collects AI decision data from board.row[0..2], so a
  // human-side Spy must leave the AI's Decoy weight and selection unchanged.
  assert(boardSpyDecoyChoices==noEnemySpyDecoyChoices);

  Game decoyTargetSide;
  decoyTargetSide.phase=Phase::OpponentTurn;decoyTargetSide.currentPlayer=1;
  Card chooseDecoy;chooseDecoy.id="source-side-decoy";chooseDecoy.name="Decoy";chooseDecoy.kind=Kind::Decoy;
  decoyTargetSide.players[1].hand={chooseDecoy};
  Card aiSpy=unit("ai-own-spy",3);aiSpy.spy=true;aiSpy.abilities={"spy"};
  Card humanSpy=unit("human-side-spy",1);humanSpy.spy=true;humanSpy.abilities={"spy"};
  decoyTargetSide.players[1].board[1].push_back(aiSpy);
  decoyTargetSide.players[0].board[0].push_back(humanSpy);
  const auto decoyTarget=chooseAiAction(decoyTargetSide,1);
  assert(decoyTarget.cardId==chooseDecoy.id&&decoyTarget.selectedCardId==aiSpy.id);
  assert(playCard(decoyTargetSide,1,decoyTarget.cardId,decoyTarget.target,decoyTarget.selectedCardId));
  assert(std::any_of(decoyTargetSide.players[1].hand.begin(),decoyTargetSide.players[1].hand.end(),
                     [&](const Card& card){return card.id==aiSpy.id;}));
  assert(std::any_of(decoyTargetSide.players[0].board[0].begin(),decoyTargetSide.players[0].board[0].end(),
                     [&](const Card& card){return card.id==humanSpy.id;}));

  // Source's zero-total fallback still plays Clear Weather when its raw
  // weather weight is exactly zero. Both sides gain the same amount here, so
  // the ordinary weighted choice has no positive-weight action to select.
  Game zeroWeightClear;
  zeroWeightClear.phase=Phase::OpponentTurn;zeroWeightClear.currentPlayer=1;
  zeroWeightClear.weather[0]=true;
  zeroWeightClear.players[0].board[0].push_back(unit("equal-weathered-opponent",8));
  zeroWeightClear.players[1].board[0].push_back(unit("equal-weathered-ai",8));
  zeroWeightClear.players[1].hand={clear};
  const auto zeroWeightChoice=chooseAiAction(zeroWeightClear,1);
  assert(!zeroWeightChoice.pass&&zeroWeightChoice.cardId==clear.id);

  // Source getMaximums() reads Card.power after Row.updateScore() has applied
  // weather. The AI's printed-10 unit therefore has current power 1 and the
  // opposing printed-8 unit is the Scorch maximum.
  Game rawPowerScorch;
  rawPowerScorch.phase=Phase::OpponentTurn;rawPowerScorch.currentPlayer=1;
  rawPowerScorch.weather[0]=true;
  rawPowerScorch.players[1].board[0].push_back(unit("printed-ten-ai",10));
  rawPowerScorch.players[0].board[1].push_back(unit("printed-eight-opponent",8,Row::Ranged));
  Card aiScorch;aiScorch.id="ai-global-scorch";aiScorch.name="Scorch";aiScorch.kind=Kind::Scorch;
  rawPowerScorch.players[1].hand={aiScorch,unit("ai-one-point",1,Row::Siege)};
  int weatherAdjustedScorchChoices=0;
  for(std::uint64_t seed=1;seed<=256;++seed) {
    auto sample=rawPowerScorch;sample.rngState=seed*0x85ebca6bu;
    if(chooseAiAction(sample,1).cardId==aiScorch.id)++weatherAdjustedScorchChoices;
  }
  assert(weatherAdjustedScorchChoices>200);

  Game difficultyChoice;
  difficultyChoice.phase=Phase::OpponentTurn;difficultyChoice.currentPlayer=1;
  difficultyChoice.players[1].hand={unit("weak",1),unit("strong",10)};
  Game preview=difficultyChoice;
  const auto action=chooseAiAction(preview,1);
  assert(action.pass || action.activateLeader ||
         playCard(preview,1,action.cardId,action.target,action.selectedCardId));

  Game mardroemeAi;
  mardroemeAi.phase=Phase::OpponentTurn;mardroemeAi.currentPlayer=1;
  for(int i=0;i<2;++i) {
    auto close=unit("ai-close-berserker-"+std::to_string(i),5,Row::Melee);
    close.name="Berserker";close.berserker=true;close.abilities={"berserker"};
    auto ranged=unit("ai-ranged-berserker-"+std::to_string(i),5,Row::Ranged);
    ranged.name="Young Berserker";ranged.berserker=true;ranged.abilities={"berserker"};
    mardroemeAi.players[1].board[0].push_back(close);
    mardroemeAi.players[1].board[1].push_back(ranged);
  }
  Card aiMardroeme;aiMardroeme.id="ai-mardroeme";aiMardroeme.name="Mardroeme";
  aiMardroeme.kind=Kind::Mardroeme;aiMardroeme.abilities={"mardroeme"};
  mardroemeAi.players[1].hand.push_back(aiMardroeme);
  const auto mardroemeChoice=chooseAiAction(mardroemeAi,1);
  assert(!mardroemeChoice.pass&&mardroemeChoice.cardId==aiMardroeme.id&&mardroemeChoice.target==Row::Ranged);

  Game decoyAi;
  decoyAi.phase=Phase::OpponentTurn;decoyAi.currentPlayer=1;decoyAi.players[1].roundLosses=1;
  Card aiDecoy;aiDecoy.id="ai-decoy";aiDecoy.name="Decoy";aiDecoy.kind=Kind::Decoy;
  decoyAi.players[1].hand.push_back(aiDecoy);
  decoyAi.players[1].board[0]={unit("ai-decoy-strong-a",10),unit("ai-decoy-weaker-a",5)};
  decoyAi.players[1].board[1]={unit("ai-decoy-strong-b",10),unit("ai-decoy-weaker-b",3)};
  decoyAi.players[1].board[2]={unit("ai-decoy-outside-max-row",9)};
  bool selectedOtherRowMaximum=false;
  for(std::uint64_t seed=1;seed<=64;++seed) {
    auto sample=decoyAi;sample.rngState=seed;
    const auto choice=chooseAiAction(sample,1);
    assert(!choice.pass&&choice.cardId==aiDecoy.id);
    assert(choice.selectedCardId=="ai-decoy-strong-a"||choice.selectedCardId=="ai-decoy-strong-b"||
           choice.selectedCardId=="ai-decoy-outside-max-row");
    selectedOtherRowMaximum=selectedOtherRowMaximum||choice.selectedCardId=="ai-decoy-outside-max-row";
  }
  assert(selectedOtherRowMaximum); // source decoy() targets a row maximum from any own row, not max.me.

  Game roachMulligan;roachMulligan.phase=Phase::Mulligan;
  roachMulligan.players[1].hand={referenceCard("Roach"),referenceCard("Geralt of Rivia"),unit("low-priority",2)};
  roachMulligan.players[1].deck={unit("replacement-a",4),unit("replacement-b",5)};
  // ControllerAI.redraw() makes two sequential discardOrder() decisions; a
  // low-power Roach can become eligible on the second redraw after the first
  // discarded card has been returned to the deck and a replacement drawn.
  const auto roachId=roachMulligan.players[1].hand[0].id;
  assert(performAiMulligan(roachMulligan,1));
  assert(roachMulligan.players[1].hand.size()==3);
  assert(std::any_of(roachMulligan.players[1].hand.begin(),roachMulligan.players[1].hand.end(),[&](const Card& card){return card.id==roachId;}) ||
         std::any_of(roachMulligan.players[1].deck.begin(),roachMulligan.players[1].deck.end(),[&](const Card& card){return card.id==roachId;}));

  Game clearWeatherRedraw;clearWeatherRedraw.phase=Phase::Mulligan;clearWeatherRedraw.rngState=0x6162;
  auto clearInHand=referenceCard("Clear Weather");clearInHand.id="ai-mulligan-clear";
  auto frostInHand=referenceCard("Biting Frost");frostInHand.id="ai-mulligan-frost";
  clearWeatherRedraw.players[1].hand={clearInHand,frostInHand};
  for(int i=0;i<2;++i) {
    auto protectedReplacement=unit("ai-mulligan-protected-"+std::to_string(i),4);
    protectedReplacement.hero=true;protectedReplacement.abilities={"hero"};
    clearWeatherRedraw.players[1].deck.push_back(protectedReplacement);
  }
  assert(performAiMulligan(clearWeatherRedraw,1));
  assert(std::count_if(clearWeatherRedraw.players[1].hand.begin(),clearWeatherRedraw.players[1].hand.end(),
      [](const Card& card){return card.kind==Kind::Weather||card.kind==Kind::ClearWeather;})==1);

  Game stableAiDiscard;stableAiDiscard.phase=Phase::Mulligan;
  for(std::uint64_t seed=1;;++seed) {
    auto state=seed;
    const auto first=expectedRandom(state),second=expectedRandom(state);
    if(first%3==2&&second%3!=0){stableAiDiscard.rngState=seed;break;}
  }
  for(int i=0;i<20;++i) {
    auto equalPriority=unit("equal-priority-"+std::to_string(i),1);
    equalPriority.name="Equal Priority Unit";
    stableAiDiscard.players[1].hand.push_back(equalPriority);
  }
  for(int i=0;i<2;++i) {
    auto protectedReplacement=unit("stable-replacement-"+std::to_string(i),4);
    protectedReplacement.hero=true;protectedReplacement.abilities={"hero"};
    stableAiDiscard.players[1].deck.push_back(protectedReplacement);
  }
  assert(performAiMulligan(stableAiDiscard,1));
  for(int i=0;i<2;++i) {
    const auto id="equal-priority-"+std::to_string(i);
    assert(std::any_of(stableAiDiscard.players[1].deck.begin(),stableAiDiscard.players[1].deck.end(),
        [&](const Card& card){return card.id==id;}));
    assert(std::none_of(stableAiDiscard.players[1].hand.begin(),stableAiDiscard.players[1].hand.end(),
        [&](const Card& card){return card.id==id;}));
  }

  Game leaderChoice;
  leaderChoice.players[0].leader.id="foltest_siegemaster";
  leaderChoice.players[0].leader.kind=Kind::Leader;
  leaderChoice.players[0].leader.abilities={"foltest_siegemaster"};
  leaderChoice.players[0].leaderAbilityAvailable=true;
  leaderChoice.players[0].board[2].push_back(unit("ai-siege",6,Row::Siege));
  leaderChoice.players[0].hand.push_back(unit("ai-small-play",1));
  const auto leaderAction=chooseAiAction(leaderChoice,0);
  assert(leaderAction.activateLeader || (!leaderAction.pass && leaderAction.cardId=="ai-small-play"));

  Game noAgileFrancesca;
  noAgileFrancesca.phase=Phase::OpponentTurn;noAgileFrancesca.currentPlayer=1;
  noAgileFrancesca.players[1].leader.id="francesca_hope";
  noAgileFrancesca.players[1].leader.kind=Kind::Leader;
  noAgileFrancesca.players[1].leader.abilities={"francesca_hope"};
  noAgileFrancesca.players[1].leaderAbilityAvailable=true;
  noAgileFrancesca.players[1].hand={unit("hope-weight-two",2),unit("hope-weight-one",1)};
  for(std::uint64_t seed=1;seed<=128;++seed) {
    auto sample=noAgileFrancesca;sample.rngState=seed;
    const auto choice=chooseAiAction(sample,1);
    // Source gives Francesca's no-op helper weight -1, so the total action
    // weight is 2 and the first card fills that entire interval.
    assert(!choice.pass&&!choice.activateLeader&&choice.cardId=="hope-weight-two");
  }

  const auto& recipes=referencePremadeDecks();
  const auto deck0=makeReferenceDeck(recipes[0]);
  const auto deck1=makeReferenceDeck(recipes[2]);
  {
    Game game;
    beginMatch(game,deck0,recipes[0].faction,deck1,recipes[2].faction,0x987654,0);
    assert(replaceMulliganCards(game,0,{})); assert(performAiMulligan(game,1));
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
  // Exercise every source premade deck as both sides, with its exact source
  // leader, so illegal weighted choices and stalled round transitions surface
  // across many shuffled hands rather than only the two fixtures above.
  for(std::uint64_t seed=0;seed<100;++seed) {
    const auto& playerRecipe=recipes[static_cast<std::size_t>(seed%recipes.size())];
    const auto opponentIndex=static_cast<std::size_t>((seed*7+3)%recipes.size());
    const auto& opponentRecipe=recipes[opponentIndex];
    const auto playerCards=makeReferenceDeck(playerRecipe);
    const auto opponentCards=makeReferenceDeck(opponentRecipe);
    std::vector<std::string> originalIds;
    for(std::size_t i=0;i<playerCards.size();++i)
      originalIds.push_back("p0:"+std::to_string(i)+":"+playerCards[i].id);
    for(std::size_t i=0;i<opponentCards.size();++i)
      originalIds.push_back("p1:"+std::to_string(i)+":"+opponentCards[i].id);
    Card playerLeader,opponentLeader;
    assert(referenceLeader(playerRecipe.leaderReferenceIndex,playerLeader));
    assert(referenceLeader(opponentRecipe.leaderReferenceIndex,opponentLeader));
    Game simulation;
    beginMatchWithLeaders(simulation,playerCards,playerLeader,
      opponentCards,opponentLeader,0xA17E0000ULL+seed,static_cast<int>(seed%2));
    assert(matchCardIdsAreUnique(simulation)&&matchContainsOriginalIds(simulation,originalIds));
    if(simulation.scoiataelChoicePending)
      assert(chooseScoiataelFirstPlayer(simulation,simulation.firstPlayer));
    assert(replaceMulliganCards(simulation,0,{}));
    assert(performAiMulligan(simulation,1));
    int actions=0;
    while(simulation.phase!=Phase::MatchResult&&actions<500) {
      assert(matchCardIdsAreUnique(simulation)&&matchContainsOriginalIds(simulation,originalIds));
      if(simulation.phase==Phase::RoundResult) { assert(startNextRound(simulation));continue; }
      if(simulation.phase==Phase::RoundSetup) {
        const int owner=simulation.skelligeRevivePlayer;
        const auto candidate=std::find_if(simulation.players[owner].graveyard.begin(),simulation.players[owner].graveyard.end(),[&](const Card& card){
          return std::find(simulation.skelligeAgileReviveIds.begin(),simulation.skelligeAgileReviveIds.end(),card.id)!=simulation.skelligeAgileReviveIds.end();
        });
        if(candidate==simulation.players[owner].graveyard.end()) {
          std::cerr<<"Skellige revive candidate missing at seed "<<seed<<" owner "<<owner
                   <<" queued="<<simulation.skelligeRevivesRemaining<<" ids=";
          for(const auto& id:simulation.skelligeAgileReviveIds)std::cerr<<id<<",";
          std::cerr<<" grave=";
          for(const auto& card:simulation.players[owner].graveyard)
            std::cerr<<card.name<<"["<<card.id<<", agile="<<card.agile<<"],";
          for(int zone=0;zone<3;++zone) {
            const auto& cards=zone==0?simulation.players[owner].hand:zone==1?simulation.players[owner].deck:simulation.players[owner].graveyard;
            std::cerr<<" zone"<<zone<<"=";
            for(const auto& card:cards)if(std::find(simulation.skelligeAgileReviveIds.begin(),simulation.skelligeAgileReviveIds.end(),card.id)!=simulation.skelligeAgileReviveIds.end())
              std::cerr<<card.name<<"["<<card.id<<"],";
          }
          for(int row=0;row<3;++row)for(const auto& card:simulation.players[owner].board[row])
            if(std::find(simulation.skelligeAgileReviveIds.begin(),simulation.skelligeAgileReviveIds.end(),card.id)!=simulation.skelligeAgileReviveIds.end())
              std::cerr<<" board"<<row<<"="<<card.name<<"["<<card.id<<"],";
          std::cerr<<"\n";
          assert(false);
        }
        const Row row=candidate->agile?Row::Melee:Row::None;
        assert(chooseSkelligeRevive(simulation,owner,candidate->id,row));
        continue;
      }
      const int actor=simulation.currentPlayer;
      const auto action=chooseAiAction(simulation,actor);
      const bool legal=action.pass?pass(simulation,actor):action.activateLeader?
        activateLeader(simulation,actor,action.target,action.leaderSelection):
        playCard(simulation,actor,action.cardId,action.target,action.selectedCardId);
      if(!legal) {
        std::cerr<<"AI illegal action at seed "<<seed<<" step "<<actions<<" player "<<actor
                 <<" phase "<<static_cast<int>(simulation.phase)<<" action card="<<action.cardId
                 <<" pass="<<action.pass<<" leader="<<action.activateLeader<<" target="<<static_cast<int>(action.target)
                 <<" selected="<<action.selectedCardId<<"\n";
        assert(false);
      }
      ++actions;
    }
    assert(simulation.phase==Phase::MatchResult&&actions<500);
    assert(matchCardIdsAreUnique(simulation)&&matchContainsOriginalIds(simulation,originalIds));
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
static std::vector<Card> expectedSourceDeckInitialization(const std::vector<Card>& source,std::uint64_t& state) {
  std::vector<Card> deck;
  for(const auto& card:source) {
    deck.push_back(card);
    const auto index=static_cast<std::size_t>(expectedRandom(state))%deck.size();
    std::swap(deck[index],deck.back());
  }
  return deck;
}
static void sourceAiPassDecisions() {
  Game passedAhead;
  passedAhead.phase=Phase::OpponentTurn;passedAhead.currentPlayer=1;
  passedAhead.players[0].passed=true;
  passedAhead.players[1].hand={unit("pass-ahead-action",5)};
  passedAhead.players[1].score=6;passedAhead.players[0].score=5;
  const auto aheadRng=passedAhead.rngState;
  const auto ahead=chooseAiAction(passedAhead,1);
  assert(ahead.pass&&!ahead.activateLeader&&passedAhead.rngState==aheadRng);

  Game nilfgaardTie;
  nilfgaardTie.phase=Phase::OpponentTurn;nilfgaardTie.currentPlayer=1;
  nilfgaardTie.players[0].passed=true;nilfgaardTie.players[1].faction=Faction::Nilfgaard;
  nilfgaardTie.players[1].hand={unit("nilfgaard-pass-tie-action",5)};
  nilfgaardTie.players[1].score=nilfgaardTie.players[0].score=7;
  const auto nilfgaardRng=nilfgaardTie.rngState;
  assert(chooseAiAction(nilfgaardTie,1).pass&&nilfgaardTie.rngState==nilfgaardRng);

  Game nonNilfgaardTie;
  nonNilfgaardTie.phase=Phase::OpponentTurn;nonNilfgaardTie.currentPlayer=1;
  nonNilfgaardTie.players[0].passed=true;
  nonNilfgaardTie.players[1].hand={unit("non-nilfgaard-tie-action",5)};
  nonNilfgaardTie.players[1].score=nonNilfgaardTie.players[0].score=7;
  const auto tieAction=chooseAiAction(nonNilfgaardTie,1);
  assert(!tieAction.pass&&tieAction.cardId=="non-nilfgaard-tie-action");

  Game lastGem;
  lastGem.phase=Phase::OpponentTurn;lastGem.currentPlayer=1;
  lastGem.players[1].roundLosses=1;lastGem.players[1].hand={unit("last-gem-action",5)};
  lastGem.players[1].score=0;lastGem.players[0].score=20;
  const auto lastGemAction=chooseAiAction(lastGem,1);
  assert(!lastGemAction.pass&&lastGemAction.cardId=="last-gem-action");
}

static void sourceAiHeroMoraleWeight() {
  constexpr std::uint64_t seed=2688;
  auto state=seed;
  const double ticket=static_cast<double>(expectedRandom(state))/4294967296.0;
  // The source weights Isengrim as 10 strength plus one gained point (11),
  // ahead of a five-point unit. The old native weight was only one.
  assert(ticket>1.0/6.0&&ticket<11.0/16.0);
  Game game;game.phase=Phase::OpponentTurn;game.currentPlayer=1;game.rngState=seed;
  game.players[1].hand={referenceCard("Isengrim Faoiltiarna"),unit("ordinary-five",5)};
  game.players[1].board[0]={unit("ally-five",5)};
  game.players[0].board[0]={unit("opponent-five",5)};
  const auto action=chooseAiAction(game,1);
  assert(!action.pass&&action.cardId==game.players[1].hand.front().id);
}

static void sourceMatchInitializationOrder() {
  std::vector<Card> playerDeck,opponentDeck;
  for(int i=0;i<12;++i) {
    playerDeck.push_back(unit("initial-player-"+std::to_string(i),i+1));
    opponentDeck.push_back(unit("initial-opponent-"+std::to_string(i),i+1));
  }
  playerDeck[0]=referenceCard("Decoy");playerDeck[0].id="initial-player-decoy-a";
  playerDeck[1]=referenceCard("Decoy");playerDeck[1].id="initial-player-decoy-b";
  playerDeck[2]=referenceCard("Biting Frost");playerDeck[2].id="initial-player-weather";
  auto expectedPlayerDeck=playerDeck,expectedOpponentDeck=opponentDeck;
  const auto namespaceInstances=[](std::vector<Card>& cards,int owner) {
    for(std::size_t i=0;i<cards.size();++i)
      cards[i].id="p"+std::to_string(owner)+":"+std::to_string(i)+":"+cards[i].id;
  };
  namespaceInstances(expectedPlayerDeck,0);namespaceInstances(expectedOpponentDeck,1);
  constexpr std::uint64_t seed=0x5a17c3;
  std::uint64_t expectedRng=seed;
  auto expectedPlayer=expectedSourceDeckInitialization(expectedPlayerDeck,expectedRng);
  auto expectedOpponent=expectedSourceDeckInitialization(expectedOpponentDeck,expectedRng);

  Game daisy;
  beginMatchWithLeaders(daisy,playerDeck,leader("francesca_daisy"),opponentDeck,
                        leader("ordinary_leader"),seed,0);
  assert(daisy.players[0].hand.size()==11&&daisy.players[0].deck.size()==1);
  assert(daisy.players[1].hand.size()==10&&daisy.players[1].deck.size()==2);
  std::vector<Card> expectedHumanHand(expectedPlayer.begin(),expectedPlayer.begin()+11);
  std::stable_sort(expectedHumanHand.begin(),expectedHumanHand.end(),expectedSourceCardBefore);
  for(std::size_t i=0;i<11;++i)assert(daisy.players[0].hand[i].id==expectedHumanHand[i].id);
  for(std::size_t i=0;i<10;++i)
    assert(daisy.players[1].hand[i].id==expectedOpponent[9-i].id); // HandAI inserts every draw at index zero.
  assert(daisy.players[0].deck.front().id==expectedPlayer[11].id);
  assert(daisy.players[1].deck[0].id==expectedOpponent[10].id&&
         daisy.players[1].deck[1].id==expectedOpponent[11].id);
  assert(daisy.rngState==expectedRng); // a forced starter skips the coin toss

  expectedRng=seed;
  expectedPlayer=expectedSourceDeckInitialization(expectedPlayerDeck,expectedRng);
  expectedOpponent=expectedSourceDeckInitialization(expectedOpponentDeck,expectedRng);
  const int expectedStarter=static_cast<int>(expectedRandom(expectedRng)%2);
  Game coin;
  beginMatch(coin,playerDeck,Faction::NorthernRealms,opponentDeck,Faction::Monsters,seed);
  assert(coin.firstPlayer==expectedStarter&&coin.rngState==expectedRng);
  expectedHumanHand.assign(expectedPlayer.begin(),expectedPlayer.begin()+10);
  std::stable_sort(expectedHumanHand.begin(),expectedHumanHand.end(),expectedSourceCardBefore);
  for(std::size_t i=0;i<10;++i) {
    assert(coin.players[0].hand[i].id==expectedHumanHand[i].id);
    assert(coin.players[1].hand[i].id==expectedOpponent[9-i].id);
  }
}
static void matchCardInstancesHaveUniqueIds() {
  const auto sharedNeutral=referenceCard("Cirilla Fiona Elen Riannon");
  Game game;
  beginMatch(game,{sharedNeutral},Faction::NorthernRealms,{sharedNeutral},Faction::Monsters,0x1234,0);
  assert(game.players[0].hand.size()==1&&game.players[1].hand.size()==1);
  assert(game.players[0].hand[0].id!=game.players[1].hand[0].id);
  assert(game.players[0].hand[0].id.rfind("p0:0:",0)==0);
  assert(game.players[1].hand[0].id.rfind("p1:0:",0)==0);
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

  Game temporaryAvenger;
  Card cow=referenceCard("Cow");cow.id="temporary-cow";
  temporaryAvenger.players[1].board[0].push_back(cow);
  Card firstScorch;firstScorch.id="temporary-scorch-1";firstScorch.kind=Kind::Scorch;
  Card secondScorch=firstScorch;secondScorch.id="temporary-scorch-2";
  temporaryAvenger.players[0].hand={firstScorch,secondScorch};
  assert(playCard(temporaryAvenger,0,firstScorch.id));
  assert(std::any_of(temporaryAvenger.players[1].board[0].begin(),temporaryAvenger.players[1].board[0].end(),
                     [](const Card& card){return card.name=="Bovine Defense Force";}));
  temporaryAvenger.currentPlayer=0;temporaryAvenger.phase=Phase::PlayerTurn;
  assert(playCard(temporaryAvenger,0,secondScorch.id));
  assert(std::none_of(temporaryAvenger.players[1].graveyard.begin(),temporaryAvenger.players[1].graveyard.end(),
                      [](const Card& card){return card.id.rfind("summoned_",0)==0;}));
}
static void scorchSourceTargets() {
  Game global;
  Card scorch; scorch.id="source-global-scorch"; scorch.kind=Kind::Scorch;
  global.players[0].hand.push_back(scorch);
  auto immuneHero=unit("scorch-hero",20);immuneHero.hero=true;
  global.players[0].board[0]={immuneHero,unit("global-tie-player",9)};
  global.players[1].board[1]={unit("global-tie-opponent",9)};
  global.players[1].board[2]={unit("global-weaker",8)};
  assert(playCard(global,0,scorch.id));
  assert(global.players[0].board[0].size()==1&&global.players[0].board[0][0].hero);
  assert(global.players[1].board[1].empty());
  assert(global.players[1].board[2].size()==1&&global.players[1].board[2][0].id=="global-weaker");
  assert(std::any_of(global.players[0].graveyard.begin(),global.players[0].graveyard.end(),[](const Card& card){return card.id=="global-tie-player";}));
  assert(std::any_of(global.players[1].graveyard.begin(),global.players[1].graveyard.end(),[](const Card& card){return card.id=="global-tie-opponent";}));

  Game row;
  auto rowScorch=unit("source-row-scorch",4);rowScorch.abilities={"scorch_c"};
  row.players[0].hand.push_back(rowScorch);
  row.players[0].board[0].push_back(unit("row-scorch-own-unit",16));
  auto highHero=unit("row-scorch-hero",15);highHero.hero=true;
  row.players[1].board[0]={highHero,unit("row-scorch-target",1)};
  assert(playCard(row,0,rowScorch.id));
  assert(std::any_of(row.players[0].board[0].begin(),row.players[0].board[0].end(),
                     [](const Card& card){return card.id=="row-scorch-own-unit";}));
  assert(row.players[1].board[0].size()==1&&row.players[1].board[0][0].hero);
  assert(row.players[1].graveyard.size()==1&&row.players[1].graveyard[0].id=="row-scorch-target");

  const std::array<const char*,3> rowScorchAbilities={"scorch_c","scorch_r","scorch_s"};
  for(int targetRow=1;targetRow<3;++targetRow) {
    Game rangedOrSiege;
    auto source=unit("scorch-source-"+std::to_string(targetRow),4);
    source.abilities={rowScorchAbilities[static_cast<std::size_t>(targetRow)]};
    rangedOrSiege.players[0].hand.push_back(source);
    rangedOrSiege.players[1].board[targetRow]={unit("high-"+std::to_string(targetRow),6),
                                                unit("low-"+std::to_string(targetRow),4)};
    assert(playCard(rangedOrSiege,0,source.id));
    assert(rangedOrSiege.players[1].board[targetRow].size()==1&&
           rangedOrSiege.players[1].board[targetRow][0].id=="low-"+std::to_string(targetRow));
  }

  Game belowThreshold;
  auto under=unit("under-threshold-scorch",4);under.abilities={"scorch_c"};
  belowThreshold.players[0].hand.push_back(under);
  belowThreshold.players[1].board[0]={unit("row-five",5),unit("row-four",4)};
  assert(playCard(belowThreshold,0,under.id));
  assert(belowThreshold.players[1].board[0].size()==2&&belowThreshold.players[1].graveyard.empty());
}
static void leaderAbilities() {
  std::vector<Card> deck;
  for(int i=0;i<24;++i) deck.push_back(unit("leader-test-"+std::to_string(i),1));
  Game passives;
  beginMatchWithLeaders(passives,deck,leader("francesca_daisy"),deck,leader("king_bran"),0x1234,0);
  assert(passives.players[0].hand.size()==11 && passives.players[0].deck.size()==13);
  passives.players[1].board[0].push_back(unit("bran-weather",9));
  passives.players[1].board[0].push_back(unit("bran-weather-odd",1));
  passives.players[0].board[0].push_back(unit("opponent-weather",9));
  assert(calculateScore(passives.players[1],{true,false,false},{false,false,false})==4);
  // King Bran applies only to the holder's rows; odd power rounds down in
  // weather exactly as the source's Math.floor(total / 2) does (1 -> 0).
  assert(calculateScore(passives.players[0],{true,false,false},{false,false,false})==1);

  const std::vector<std::string> cancellablePassives={"emhyr_invader","eredin_treacherous","francesca_daisy","king_bran"};
  for(int whiteOwner=0;whiteOwner<2;++whiteOwner) {
    Card whiteFlame=leader("emhyr_whiteflame"),activeLeader=leader("foltest_lord");
    Card playerLeader=whiteOwner==0?whiteFlame:activeLeader;
    Card opponentLeader=whiteOwner==1?whiteFlame:activeLeader;
    Game cancellation;
    beginMatchWithLeaders(cancellation,deck,playerLeader,deck,opponentLeader,0x4321+whiteOwner,0);
    assert(!cancellation.players[0].leaderAbilityAvailable&&cancellation.players[0].leaderAbilityCancelled);
    assert(!cancellation.players[1].leaderAbilityAvailable&&cancellation.players[1].leaderAbilityCancelled);
    for(std::size_t passiveIndex=0;passiveIndex<cancellablePassives.size();++passiveIndex) {
      Card passive=leader(cancellablePassives[passiveIndex]);
      playerLeader=whiteOwner==0?whiteFlame:passive;
      opponentLeader=whiteOwner==1?whiteFlame:passive;
      Game suppressedPassive;
      beginMatchWithLeaders(suppressedPassive,deck,playerLeader,deck,opponentLeader,
                            0x432100+passiveIndex*2+static_cast<std::size_t>(whiteOwner),0);
      assert(suppressedPassive.players[0].hand.size()==10&&suppressedPassive.players[1].hand.size()==10);
      assert(!suppressedPassive.randomRespawn&&!suppressedPassive.players[0].doubleSpyPower&&
             !suppressedPassive.players[1].doubleSpyPower&&!suppressedPassive.players[0].halfWeather&&
             !suppressedPassive.players[1].halfWeather);
      assert(suppressedPassive.players[0].leaderAbilityCancelled&&suppressedPassive.players[1].leaderAbilityCancelled);
    }
  }

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

  Game emhyrRain;
  emhyrRain.players[0].leader=leader("emhyr_imperial");emhyrRain.players[0].leaderAbilityAvailable=true;
  emhyrRain.players[0].deck={referenceCard("Torrential Rain")};
  assert(activateLeader(emhyrRain,0));
  assert(emhyrRain.weather[2]&&emhyrRain.players[0].deck.empty()&&emhyrRain.weatherCards[0].card.name=="Torrential Rain");

  Game clearWeather;
  clearWeather.players[0].leader=leader("foltest_lord");clearWeather.players[0].leaderAbilityAvailable=true;
  clearWeather.weather.fill(true);
  clearWeather.weatherCards.push_back({referenceCard("Biting Frost"),1});
  assert(activateLeader(clearWeather,0));
  assert(std::none_of(clearWeather.weather.begin(),clearWeather.weather.end(),[](bool active){return active;}));
  assert(clearWeather.players[1].graveyard.back().name=="Biting Frost");

  Game horn;
  horn.players[0].leader=leader("foltest_siegemaster"); horn.players[0].leaderAbilityAvailable=true;
  horn.players[0].board[2].push_back(unit("siege",5));
  assert(activateLeader(horn,0));
  assert(horn.players[0].score==10 && !horn.players[0].leaderAbilityAvailable);

  Game closeHorn;
  closeHorn.players[0].leader=leader("eredin_commander");closeHorn.players[0].leaderAbilityAvailable=true;
  closeHorn.players[0].board[0].push_back(unit("close-horn",5,Row::Melee));
  assert(activateLeader(closeHorn,0));
  assert(closeHorn.players[0].score==10&&closeHorn.players[0].specialOccupied[0]);
  Game occupiedCloseHorn;
  occupiedCloseHorn.players[0].leader=leader("eredin_commander");occupiedCloseHorn.players[0].leaderAbilityAvailable=true;
  occupiedCloseHorn.players[0].specialOccupied[0]=true;
  assert(!activateLeader(occupiedCloseHorn,0));
  assert(occupiedCloseHorn.players[0].leaderAbilityAvailable&&!occupiedCloseHorn.horn[0][0]);

  Game rowScorch;
  rowScorch.players[0].leader=leader("foltest_steelforged"); rowScorch.players[0].leaderAbilityAvailable=true;
  rowScorch.players[1].board[2]={unit("siege-6",6,Row::Siege),unit("siege-4",4,Row::Siege)};
  assert(activateLeader(rowScorch,0));
  assert(rowScorch.players[1].board[2].size()==1 && rowScorch.players[1].graveyard.size()==1);

  Game rangedScorch;
  rangedScorch.players[0].leader=leader("foltest_son");rangedScorch.players[0].leaderAbilityAvailable=true;
  rangedScorch.players[1].board[1]={unit("ranged-6",6,Row::Ranged),unit("ranged-4",4,Row::Ranged)};
  assert(activateLeader(rangedScorch,0));
  assert(rangedScorch.players[1].board[1].size()==1&&rangedScorch.players[1].graveyard.size()==1);

  Game meleeScorch;
  meleeScorch.players[0].leader=leader("francesca_queen");meleeScorch.players[0].leaderAbilityAvailable=true;
  meleeScorch.players[1].board[0]={unit("melee-6",6,Row::Melee),unit("melee-4",4,Row::Melee)};
  assert(activateLeader(meleeScorch,0));
  assert(meleeScorch.players[1].board[0].size()==1&&meleeScorch.players[1].graveyard.size()==1);

  Game rangedHorn;
  rangedHorn.players[0].leader=leader("francesca_beautiful");rangedHorn.players[0].leaderAbilityAvailable=true;
  rangedHorn.players[0].board[1].push_back(unit("ranged-horn",5,Row::Ranged));
  assert(activateLeader(rangedHorn,0));
  assert(rangedHorn.players[0].score==10&&rangedHorn.players[0].specialOccupied[1]);

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
  assert(!activateLeader(heroRestore,0,Row::None,{"grave-hero"}));
  assert(heroRestore.players[0].hand.empty()&&heroRestore.players[1].graveyard.size()==1&&heroRestore.players[0].leaderAbilityAvailable);
  Game heroRestoreBringer;
  heroRestoreBringer.players[0].leader=leader("eredin_bringer_of_death");heroRestoreBringer.players[0].leaderAbilityAvailable=true;
  heroRestoreBringer.players[0].graveyard.push_back(graveHero);
  assert(!activateLeader(heroRestoreBringer,0,Row::None,{"grave-hero"}));
  assert(heroRestoreBringer.players[0].hand.empty()&&heroRestoreBringer.players[0].graveyard.size()==1&&heroRestoreBringer.players[0].leaderAbilityAvailable);

  Game discard;
  discard.players[0].leader=leader("eredin_destroyer"); discard.players[0].leaderAbilityAvailable=true;
  discard.players[0].hand={unit("discard-a",1),unit("discard-b",2)};
  discard.players[0].deck={unit("draw-a",3),unit("draw-b",4)};
  assert(!activateLeader(discard,0));
  assert(discard.players[0].hand.size()==2 && discard.players[0].deck.size()==2 && discard.players[0].leaderAbilityAvailable);
  assert(activateLeader(discard,0,Row::None,{"discard-a","discard-b","draw-a"}));
  assert(discard.players[0].hand.size()==1 && discard.players[0].hand[0].id=="draw-a");
  assert(discard.players[0].graveyard.size()==2 && discard.players[0].deck.size()==1);

  Game cancelledDestroyerDraw;
  cancelledDestroyerDraw.players[0].leader=leader("eredin_destroyer");
  cancelledDestroyerDraw.players[0].leaderAbilityAvailable=true;
  cancelledDestroyerDraw.players[0].hand={unit("cancel-discard-a",2),unit("cancel-discard-b",3)};
  cancelledDestroyerDraw.players[0].deck={unit("cancel-draw",4)};
  assert(activateLeader(cancelledDestroyerDraw,0,Row::None,{"cancel-discard-a","cancel-discard-b"}));
  assert(cancelledDestroyerDraw.players[0].hand.empty()&&cancelledDestroyerDraw.players[0].graveyard.size()==2&&
         cancelledDestroyerDraw.players[0].deck.size()==1&&!cancelledDestroyerDraw.players[0].leaderAbilityAvailable);

  Game destroyerOneDiscard;
  destroyerOneDiscard.players[0].leader=leader("eredin_destroyer");
  destroyerOneDiscard.players[0].leaderAbilityAvailable=true;
  destroyerOneDiscard.players[0].hand={unit("only-discard",2)};
  assert(activateLeader(destroyerOneDiscard,0,Row::None,{"only-discard"}));
  assert(destroyerOneDiscard.players[0].hand.empty()&&destroyerOneDiscard.players[0].graveyard.size()==1&&
         !destroyerOneDiscard.players[0].leaderAbilityAvailable);

  Game destroyerDrawOnly;
  destroyerDrawOnly.players[0].leader=leader("eredin_destroyer");
  destroyerDrawOnly.players[0].leaderAbilityAvailable=true;
  destroyerDrawOnly.players[0].deck={unit("draw-only-target",4)};
  assert(activateLeader(destroyerDrawOnly,0,Row::None,{"draw-only-target"}));
  assert(destroyerDrawOnly.players[0].hand.size()==1&&destroyerDrawOnly.players[0].graveyard.empty()&&
         destroyerDrawOnly.players[0].deck.empty()&&!destroyerDrawOnly.players[0].leaderAbilityAvailable);

  Game kingWeather;
  kingWeather.players[0].leader=leader("eredin_king"); kingWeather.players[0].leaderAbilityAvailable=true;
  kingWeather.players[0].deck={referenceCard("Biting Frost"),referenceCard("Torrential Rain")};
  assert(!activateLeader(kingWeather,0));
  assert(kingWeather.players[0].deck.size()==2 && kingWeather.weatherCards.empty());
  assert(!activateLeader(kingWeather,0,Row::None,{"not-in-deck"}));
  assert(kingWeather.players[0].deck.size()==2 && kingWeather.weatherCards.empty());
  assert(activateLeader(kingWeather,0,Row::None,{kingWeather.players[0].deck[1].id}));
  assert(kingWeather.weather[2] && kingWeather.weatherCards.size()==1 && kingWeather.weatherCards[0].card.name=="Torrential Rain");

  Game francescaWeather;
  francescaWeather.players[0].leader=leader("francesca_pureblood");francescaWeather.players[0].leaderAbilityAvailable=true;
  francescaWeather.players[0].deck={referenceCard("Biting Frost")};
  assert(activateLeader(francescaWeather,0));
  assert(francescaWeather.weather[0]&&francescaWeather.weatherCards[0].card.name=="Biting Frost");

  Game francescaHope;
  francescaHope.players[0].leader=leader("francesca_hope");francescaHope.players[0].leaderAbilityAvailable=true;
  auto agile=unit("weather-agile",5,Row::Melee);agile.agile=true;
  francescaHope.players[0].board[0].push_back(agile);francescaHope.weather[0]=true;
  assert(activateLeader(francescaHope,0));
  assert(francescaHope.players[0].board[0].empty()&&francescaHope.players[0].board[1].size()==1&&francescaHope.players[0].score==5);

  Game francescaHopeSort;
  francescaHopeSort.players[0].leader=leader("francesca_hope");
  francescaHopeSort.players[0].leaderAbilityAvailable=true;
  auto agileTen=unit("hope-agile-ten",10,Row::Melee);agileTen.agile=true;
  auto agileOne=unit("hope-agile-one",1,Row::Ranged);agileOne.agile=true;
  francescaHopeSort.players[0].board[0]={unit("hope-melee-two",2),agileTen};
  francescaHopeSort.players[0].board[1]={agileOne,unit("hope-ranged-three",3)};
  assert(activateLeader(francescaHopeSort,0));
  for(const auto& row:francescaHopeSort.players[0].board) {
    for(std::size_t i=1;i<row.size();++i)
      assert(row[i-1].power<row[i].power ||
             (row[i-1].power==row[i].power&&row[i-1].name<=row[i].name));
  }

  Game randomRespawn;
  beginMatchWithLeaders(randomRespawn,deck,leader("emhyr_invader"),deck,leader("foltest_lord"),0x9812,0);
  assert(randomRespawn.randomRespawn);

  Game crach;
  crach.players[0].leader=leader("crach_an_craite"); crach.players[0].leaderAbilityAvailable=true;
  crach.rngState=0x12345678;
  for(int owner=0;owner<2;++owner) {
    auto& player=crach.players[owner];
    for(int i=0;i<5;++i)player.deck.push_back(unit("crach-deck-"+std::to_string(owner)+"-"+std::to_string(i),i+1));
    for(int i=0;i<3;++i)player.graveyard.push_back(unit("crach-grave-"+std::to_string(owner)+"-"+std::to_string(i),i+1));
  }
  auto expectedRandomState=crach.rngState;
  auto nextExpected=[&]() {
    auto x=static_cast<std::uint32_t>(expectedRandomState);
    x^=x<<13;x^=x>>17;x^=x<<5;expectedRandomState=x;return x;
  };
  std::array<std::vector<std::string>,2> expectedDecks;
  for(int owner=0;owner<2;++owner) {
    auto& expected=expectedDecks[owner];
    for(const auto& card:crach.players[owner].deck)expected.push_back(card.id);
    for(const auto& card:crach.players[owner].graveyard) {
      expected.push_back(card.id);
      const auto index=nextExpected()%expected.size();
      std::swap(expected[index],expected.back());
    }
  }
  assert(activateLeader(crach,0));
  for(int owner=0;owner<2;++owner) {
    assert(crach.players[owner].deck.size()==expectedDecks[owner].size());
    for(std::size_t i=0;i<expectedDecks[owner].size();++i)assert(crach.players[owner].deck[i].id==expectedDecks[owner][i]);
  }
  assert(crach.players[0].graveyard.empty() && crach.players[1].graveyard.empty());

  Game emptyCrach;
  emptyCrach.players[0].leader=leader("crach_an_craite");emptyCrach.players[0].leaderAbilityAvailable=true;
  for(int owner=0;owner<2;++owner)
    for(int i=0;i<5;++i)emptyCrach.players[owner].deck.push_back(unit("empty-crach-"+std::to_string(owner)+"-"+std::to_string(i),i+1));
  std::array<std::vector<std::string>,2> untouchedDecks;
  for(int owner=0;owner<2;++owner)for(const auto& card:emptyCrach.players[owner].deck)untouchedDecks[owner].push_back(card.id);
  assert(activateLeader(emptyCrach,0));
  for(int owner=0;owner<2;++owner)
    for(std::size_t i=0;i<untouchedDecks[owner].size();++i)assert(emptyCrach.players[owner].deck[i].id==untouchedDecks[owner][i]);
}
int main() { scoreRules(); duplicateWeatherIsDiscarded(); turnsAndRounds(); sourceAutomaticPassing(); factionStartAndSkelligeRoundThree(); saveRoundTripAndCorruptionHandling(); userDeckPersistenceAndValidation(); abilitiesAndValidation(); openingHandAndMulligan(); sourceMulliganSwapSemantics(); referenceCatalog(); aiUsesNoHiddenCardsAndCompletesMatches(); sourceAiPassDecisions(); sourceAiHeroMoraleWeight(); sourceMatchInitializationOrder(); matchCardInstancesHaveUniqueIds(); avengerCards(); scorchSourceTargets(); leaderAbilities(); }
