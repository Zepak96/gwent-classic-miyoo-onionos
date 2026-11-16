#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace gwent {
namespace {
struct WeightedAction { AiAction action; double weight{0.0}; bool legal{false}; };
struct BoardData {
  std::vector<const Card*> spies, medics, scorches;
  std::vector<const Card*> ownSpies, ownMedics, ownScorches;
  std::vector<const Card*> decoyFallbackOwnRowMaxima;
  std::array<std::vector<const Card*>,2> graveSpies, graveMedics, graveScorches;
  std::array<std::vector<const Card*>,3> strongestOwn, strongestEnemy;
  int ownMax{-1}, enemyMax{-1};
};

bool has(const Card& card,const std::string& ability) {
  return std::find(card.abilities.begin(),card.abilities.end(),ability)!=card.abilities.end();
}
bool isUnit(const Card& card) { return card.kind==Kind::Unit && !card.hero; }
bool isCombatUnitForAI(const Card& card) {
  return card.kind==Kind::Unit && !card.hero && (card.row!=Row::None || card.agile);
}
std::uint32_t roll(Game& game) {
  auto x=static_cast<std::uint32_t>(game.rngState?game.rngState:1);
  x^=x<<13; x^=x>>17; x^=x<<5; game.rngState=x; return x;
}
bool hasScorch(const Card& card) { return has(card,"scorch_c")||has(card,"scorch_r")||has(card,"scorch_s"); }
int cardPower(const Game& game,int owner,int row,const Card& card,const std::vector<Card>& cards) {
  return calculateCardPower(card,cards,game.weather[row],game.horn[owner][row],
                            game.players[owner].halfWeather,game.players[owner].doubleSpyPower);
}
int scoreRow(const Game& game,int owner,int row,const std::vector<Card>& cards,
             bool weather,bool horn) {
  Player player=game.players[owner];
  player.board={};
  player.board[row]=cards;
  auto weatherState=game.weather;
  auto hornState=game.horn[owner];
  weatherState[row]=weather;
  hornState[row]=horn;
  return calculateScore(player,weatherState,hornState);
}
int currentRowScore(const Game& game,int owner,int row) {
  return scoreRow(game,owner,row,game.players[owner].board[row],
                  game.weather[row],game.horn[owner][row]);
}
int currentBoardCardPower(const Game& game,int owner,const Card* target) {
  for(int row=0;row<3;++row)for(const auto& card:game.players[owner].board[row])
    if(&card==target)return cardPower(game,owner,row,card,game.players[owner].board[row]);
  return target?target->power:0;
}
void collectCards(const std::vector<Card>& cards,int owner,BoardData& data,bool grave=false,bool ownBoard=false) {
  for(const auto& card:cards) {
    if(!isUnit(card)) continue;
    if(card.spy) {
      if(grave)data.graveSpies[owner].push_back(&card);
      else {data.spies.push_back(&card);if(ownBoard)data.ownSpies.push_back(&card);}
    }
    if(card.medic) {
      if(grave)data.graveMedics[owner].push_back(&card);
      else {data.medics.push_back(&card);if(ownBoard)data.ownMedics.push_back(&card);}
    }
    if(hasScorch(card)) {
      if(grave)data.graveScorches[owner].push_back(&card);
      else {data.scorches.push_back(&card);if(ownBoard)data.ownScorches.push_back(&card);}
    }
  }
}
BoardData collectBoardData(const Game& game,int who) {
  BoardData data;
  for(int owner=0;owner<2;++owner) {
    // Keep board-wide counts for helpers that inspect both sides; the source
    // ControllerAI separately scans its own three rows for Decoy choices.
    for(const auto& row:game.players[owner].board)collectCards(row,owner,data,false,owner==who);
    collectCards(game.players[owner].graveyard,owner,data,true);
    int best=-1;
    std::array<int,3> rowMaximum{{-1,-1,-1}};
    for(int row=0;row<3;++row) {
      for(const auto& card:game.players[owner].board[row])
        if(isUnit(card))rowMaximum[row]=std::max(rowMaximum[row],cardPower(game,owner,row,card,game.players[owner].board[row]));
      best=std::max(best,rowMaximum[row]);
    }
    // Source ControllerAI builds max.rmax.filter(...).reduce((a,r) =>
    // r.cards.map(...).concat(a), []), which prepends each row's maxima.
    // That leaves fallback Decoy targets in siege, ranged, melee order.
    if(owner==who)for(int row=2;row>=0;--row)
      if(rowMaximum[row]>=0)
        for(const auto& card:game.players[owner].board[row])
          if(isUnit(card)&&cardPower(game,owner,row,card,game.players[owner].board[row])==rowMaximum[row])data.decoyFallbackOwnRowMaxima.push_back(&card);
    for(int row=0;row<3;++row) for(const auto& card:game.players[owner].board[row])
      if(isUnit(card) && cardPower(game,owner,row,card,game.players[owner].board[row])==best)
        (owner==who?data.strongestOwn[row]:data.strongestEnemy[row]).push_back(&card);
    if(owner==who)data.ownMax=best; else data.enemyMax=best;
  }
  return data;
}
int totalMaxPower(const BoardData& data,int ownerSide) {
  const auto& rows=ownerSide==0?data.strongestOwn:data.strongestEnemy;
  const int power=ownerSide==0?data.ownMax:data.enemyMax;
  if(power<0)return 0;
  int count=0; for(const auto& row:rows)count+=static_cast<int>(row.size());
  return power*count;
}
int weightRowChange(const Game& game,int who,int row,const Card& card) {
  auto cards=game.players[who].board[row];
  const int before=scoreRow(game,who,row,cards,game.weather[row],game.horn[who][row]);
  if(card.kind==Kind::Unit)cards.push_back(card);
  const int after=scoreRow(game,who,row,cards,game.weather[row],game.horn[who][row]);
  int change=after-before;
  if(card.kind==Kind::Unit)
    change-=cardPower(game,who,row,card,cards);
  // The source AI's calcRowPower() excludes Heroes, but weightRowChangeTrue()
  // still subtracts row.calcCardScore(card) for every non-special card. For a
  // Hero with Morale/Bond this leaves its printed strength in the source
  // weight as well as the gain to other units. Preserve that source behavior.
  if(card.kind==Kind::Unit&&card.hero)change+=cardPower(game,who,row,card,cards);
  return std::max(0,change);
}
int weightHornRow(const Game& game,int who,int row) {
  if(game.players[who].specialOccupied[row])return 0;
  return std::max(0,scoreRow(game,who,row,game.players[who].board[row],game.weather[row],true)-
                     currentRowScore(game,who,row));
}
int weatherRow(const std::string& tag) {
  if(tag=="frost")return 0;
  if(tag=="fog")return 1;
  if(tag=="rain")return 2;
  return -1;
}
int weightWeather(const Game& game,int who,const Card& card) {
  std::vector<int> affected;
  if(card.kind==Kind::ClearWeather || card.name=="Clear Weather") {
    for(int row=0;row<3;++row)if(game.weather[row])affected.push_back(row);
  } else if(!card.abilities.empty()) {
    // The reference AI checks the first weather tag, matching its Weather.types lookup.
    const int row=weatherRow(card.abilities.front());
    if(row>=0 && !game.weather[row])affected.push_back(row);
  }
  if(affected.empty())return 1;
  int value=0;
  for(int row:affected) {
    const bool next=card.kind==Kind::ClearWeather || card.name=="Clear Weather"?false:true;
    const int ownBefore=currentRowScore(game,who,row);
    const int enemyBefore=currentRowScore(game,1-who,row);
    const int ownAfter=scoreRow(game,who,row,game.players[who].board[row],next,game.horn[who][row]);
    const int enemyAfter=scoreRow(game,1-who,row,game.players[1-who].board[row],next,game.horn[1-who][row]);
    // ControllerAI.calcRowPower() measures before-minus-after, then returns
    // opponentDiff-minus-ownDiff. That is the AI's own gain minus the
    // opponent's gain; reversing it makes weather help the wrong side.
    value+=(ownAfter-ownBefore)-(enemyAfter-enemyBefore);
  }
  return value;
}
int weightScorchRow(const Game& game,int who,int row) {
  const int enemy=1-who;
  if(currentRowScore(game,enemy,row)<10)return 0;
  int strongest=-1,total=0;
  for(const auto& card:game.players[enemy].board[row]) if(isUnit(card))
    strongest=std::max(strongest,cardPower(game,enemy,row,card,game.players[enemy].board[row]));
  // Source max.rmax uses current Row Card.power values, including row effects.
  for(const auto& card:game.players[enemy].board[row]) if(isUnit(card)&&cardPower(game,enemy,row,card,game.players[enemy].board[row])==strongest)
    total+=cardPower(game,enemy,row,card,game.players[enemy].board[row]);
  return total;
}
int weightGlobalScorch(const BoardData& data) {
  if(data.ownMax>data.enemyMax)return 0;
  if(data.ownMax<data.enemyMax)return totalMaxPower(data,1);
  return std::max(0,totalMaxPower(data,1)-totalMaxPower(data,0));
}
int weightMardroemeRow(const Game& game,int who,const Card& card,int row) {
  if(row!=0&&row!=1)return 0;
  if(card.name=="Mardroeme"&&game.players[who].specialOccupied[row])return 0;
  if(row==1&&card.name!="Ermion"&&std::any_of(game.players[who].hand.begin(),game.players[who].hand.end(),
      [](const Card& item){return item.name=="Ermion";}))return 0;
  const std::string target=row==1?"Young Berserker":"Berserker";
  int count=0;
  for(const auto& card:game.players[who].board[row])if(card.name==target)++count;
  // Source board indexes put close combat at 2 and ranged at 1; native Row
  // uses melee at 0 and ranged at 1. Preserve the source's corresponding
  // weights: close Berserkers use 10*n, ranged Young Berserkers use 8*n²-2*n.
  return std::max(1,row==0?10*count:8*count*count-2*count);
}
bool hasInHand(const Player& player,const std::string& ability) {
  return std::any_of(player.hand.begin(),player.hand.end(),[&](const Card& c){return has(c,ability);});
}
int weightBerserker(const Game& game,int who,int row,const Card& card,int score) {
  if(!hasInHand(game.players[who],"mardroeme") && !game.players[who].mardroeme[row])return score;
  score-=card.power;
  if(row==0)return std::max(1,score+14);
  int count=0;
  if(!game.players[who].mardroeme[row]) {
    for(const auto& unit:game.players[who].board[row])if(unit.name=="Young Berserker")++count;
  } else {
    // Preserve ControllerAI.weightBerserker's source expression exactly:
    // its predicate is a non-empty string literal, so every row card counts.
    count=static_cast<int>(game.players[who].board[row].size());
  }
  score=8*((count+1)*(count+1)-count*count)+count*score;
  return std::max(1,score);
}
int weightMedic(const Game& game,int who,const BoardData& data,int score,int graveOwner) {
  bool hasEligible=false;
  for(const auto& card:game.players[graveOwner].graveyard)
    if(isCombatUnitForAI(card)){hasEligible=true;break;}
  if(!hasEligible)return std::min(1,score);
  int bonus=0;
  const int ownerOpponent=1-graveOwner;
  if(!data.graveSpies[ownerOpponent].empty())bonus=50;
  else if(!data.graveMedics[ownerOpponent].empty())bonus=15;
  else if(!data.graveScorches[ownerOpponent].empty())bonus=10;
  else if(game.players[who].roundLosses==1)bonus=1;
  return score+bonus;
}
int weightDecoy(const BoardData& data) {
  // ControllerAI.getBoardData() scans board.row[0..2], the AI's three rows
  // in the source layout. Opponent-side abilities must not affect this weight.
  if(!data.ownSpies.empty())return 50;
  if(!data.ownMedics.empty())return 15;
  if(!data.ownScorches.empty())return 10;
  return data.ownMax>=0?1:0;
}
std::vector<Card> discardOrder(Game& game,int who) {
  const auto& player=game.players[who];
  std::vector<Card> cards;
  std::vector<Card> musters;
  for(const auto& card:player.hand)if(card.muster||has(card,"muster"))musters.push_back(card);
  std::vector<std::vector<Card>> groups;
  while(!musters.empty()) {
    Card current=musters.back(); musters.pop_back();
    const auto dash=current.name.find('-');
    const std::string prefix=dash==std::string::npos?current.name:current.name.substr(0,dash);
    auto trim=[](std::string value){
      const auto first=value.find_first_not_of(" \t\r\n");
      const auto last=value.find_last_not_of(" \t\r\n");
      return first==std::string::npos?std::string{}:value.substr(first,last-first+1);
    };
    const auto groupName=trim(prefix);
    std::vector<Card> group{current};
    for(int i=static_cast<int>(musters.size())-1;i>=0;--i) if(musters[static_cast<std::size_t>(i)].name.rfind(groupName,0)==0) {
      group.push_back(musters[static_cast<std::size_t>(i)]);
      musters.erase(musters.begin()+i);
    }
    groups.push_back(std::move(group));
  }
  const auto compare=[](const Card& a,const Card& b){
    const auto rank=[](const Card& card){return card.kind==Kind::Weather?-1:
      card.kind==Kind::CommanderHorn||card.kind==Kind::Mardroeme||card.kind==Kind::Scorch||
      card.kind==Kind::ClearWeather||card.kind==Kind::Decoy?-2:0;};
    if(rank(a)!=rank(b))return rank(a)<rank(b);
    if(a.power!=b.power)return a.power<b.power;
    return a.name<b.name;
  };
  for(auto& group:groups) {
    std::stable_sort(group.begin(),group.end(),compare);
    if(!group.empty())group.pop_back();
    cards.insert(cards.end(),group.begin(),group.end());
  }
  std::vector<Card> weather;
  // Clear Weather also has source row "weather" and participates in the
  // same multi-weather redraw selection pool.
  for(const auto& card:player.hand)
    if(card.kind==Kind::Weather||card.kind==Kind::ClearWeather)weather.push_back(card);
  if(weather.size()>1) {
    weather.erase(weather.begin()+static_cast<std::ptrdiff_t>(roll(game)%weather.size()));
    cards.insert(cards.end(),weather.begin(),weather.end());
  }
  std::vector<Card> normal;
  for(const auto& card:player.hand)if(card.abilities.empty())normal.push_back(card);
  std::stable_sort(normal.begin(),normal.end(),compare);
  cards.insert(cards.end(),normal.begin(),normal.end());
  return cards;
}
const Card* chooseMedicTarget(Game& game,const std::vector<Card>& grave) {
  std::vector<const Card*> units;
  for(const auto& card:grave)if(isCombatUnitForAI(card))units.push_back(&card);
  if(units.empty())return nullptr;
  auto minSpy=std::min_element(units.begin(),units.end(),[](const Card* a,const Card* b){
    const int ap=a->spy?a->power:std::numeric_limits<int>::max();
    const int bp=b->spy?b->power:std::numeric_limits<int>::max(); return ap<bp;
  });
  if((*minSpy)->spy) return *minSpy;
  auto maxMedic=std::max_element(units.begin(),units.end(),[](const Card* a,const Card* b){
    const int ap=a->medic?a->power:-1,bp=b->medic?b->power:-1;return ap<bp;
  });
  if((*maxMedic)->medic)return *maxMedic;
  std::vector<const Card*> scorches;
  for(const auto* card:units)if(hasScorch(*card))scorches.push_back(card);
  if(!scorches.empty())return scorches[roll(game)%scorches.size()];
  return *std::max_element(units.begin(),units.end(),[](const Card* a,const Card* b){return a->power<b->power;});
}
int rowForTag(const std::string& tag) {
  if(tag=="scorch_c")return 0;
  if(tag=="scorch_r")return 1;
  if(tag=="scorch_s")return 2;
  return -1;
}
double weightCard(const Game& game,int who,const Card& card,const BoardData& data) {
  if(card.kind==Kind::Decoy || has(card,"decoy"))return weightDecoy(data);
  if(card.kind==Kind::Scorch || has(card,"scorch"))return weightGlobalScorch(data);
  if(card.kind==Kind::CommanderHorn) {
    int best=0;for(int row=0;row<3;++row)best=std::max(best,weightHornRow(game,who,row));
    return best/2.0;
  }
  if(card.kind==Kind::Mardroeme || has(card,"mardroeme"))
    return std::max(weightMardroemeRow(game,who,card,1),weightMardroemeRow(game,who,card,0));
  if(card.kind==Kind::Weather || card.kind==Kind::ClearWeather)return std::max(0,weightWeather(game,who,card));

  int row=card.agile?0:static_cast<int>(card.row);
  if(card.agile) {
    const int close=weightRowChange(game,who,0,card),ranged=weightRowChange(game,who,1,card);
    row=ranged>close?1:0;
  }
  if(row<0||row>2)return 0;
  const int base=cardPower(game,who,row,card,game.players[who].board[row]);
  if(card.spy)return 15+base;
  if(card.medic)return weightMedic(game,who,data,base,who);
  if(card.muster)return base*3;
  if(card.berserker||has(card,"berserker"))return weightBerserker(game,who,row,card,base);
  if(card.morale||card.bond||has(card,"morale")||has(card,"bond")||has(card,"horn"))
    return weightRowChange(game,who,row,card);
  for(const auto& ability:card.abilities) {
    const int scorchRow=rowForTag(ability);
    if(scorchRow>=0)return std::max(1,weightScorchRow(game,who,scorchRow));
  }
  if(has(card,"avenger")||has(card,"avenger_kambi"))return base+50;
  return base;
}
int weightWeatherFromDeck(const Game& game,int who,const std::string& weatherTag) {
  const auto& deck=game.players[who].deck;
  if(std::none_of(deck.begin(),deck.end(),[&](const Card& c){return has(c,weatherTag);}))return 0;
  Card virtualWeather;virtualWeather.kind=Kind::Weather;virtualWeather.name="Weather";virtualWeather.abilities={weatherTag};
  return static_cast<int>(weightCard(game,who,virtualWeather,collectBoardData(game,who)));
}
int weightLeader(Game& game,int who,const BoardData& data,const std::string& id) {
  if(id=="foltest_king")return weightWeatherFromDeck(game,who,"fog");
  if(id=="foltest_lord") { Card clear;clear.kind=Kind::ClearWeather;clear.name="Clear Weather";return weightWeather(game,who,clear); }
  if(id=="foltest_siegemaster")return weightHornRow(game,who,2);
  if(id=="foltest_steelforged")return weightScorchRow(game,who,2);
  if(id=="foltest_son")return weightScorchRow(game,who,1);
  if(id=="emhyr_imperial")return weightWeatherFromDeck(game,who,"rain");
  if(id=="emhyr_emperor") {
    const auto count=game.players[1-who].hand.size();
    return count==0?0:std::max(10,10*(8-static_cast<int>(count)));
  }
  if(id=="emhyr_relentless")return weightMedic(game,who,data,0,1-who);
  if(id=="eredin_commander")return weightHornRow(game,who,0);
  if(id=="eredin_bringer_of_death")return weightMedic(game,who,data,0,who);
  if(id=="eredin_king") {
    int best=0;
    for(const auto& card:game.players[who].deck)if(card.kind==Kind::Weather&&!card.abilities.empty())
      best=std::max(best,weightWeatherFromDeck(game,who,card.abilities.front()));
    return best;
  }
  if(id=="francesca_queen")return weightScorchRow(game,who,0);
  if(id=="francesca_beautiful")return weightHornRow(game,who,1);
  if(id=="francesca_pureblood")return weightWeatherFromDeck(game,who,"frost");
  if(id=="francesca_hope") {
    const bool hasAgile=std::any_of(game.players[who].board[0].begin(),game.players[who].board[0].end(),
                                    [](const Card& card){return card.agile;}) ||
                        std::any_of(game.players[who].board[1].begin(),game.players[who].board[1].end(),
                                    [](const Card& card){return card.agile;});
    if(!hasAgile)return -1; // source helper returns score=-1 when there are no Agile units.
    Game after=game;
    if(!activateLeader(after,who))return 0;
    const int before=calculateScore(game.players[who],game.weather,game.horn[who]);
    const int afterScore=calculateScore(after.players[who],after.weather,after.horn[who]);
    return afterScore-before;
  }
  if(id=="crach_an_craite") {
    if(game.round<2)return 0;
    if(hasInHand(game.players[who],"medic")||hasInHand(game.players[who],"spy"))return 0;
    if(hasInHand(game.players[who],"decoy") &&
       (!data.ownMedics.empty() || (!data.ownSpies.empty() &&
        std::any_of(game.players[who].deck.begin(),game.players[who].deck.end(),[](const Card& c){return c.medic;}))))return 0;
    return 15;
  }
  return 10+(game.round-1)*15;
}
std::vector<std::string> leaderSelection(Game& game,int who,const std::string& id,const BoardData& data) {
  const auto& player=game.players[who];
  if(id=="emhyr_relentless") {
    const auto* target=chooseMedicTarget(game,game.players[1-who].graveyard);
    return target?std::vector<std::string>{target->id}:std::vector<std::string>{};
  }
  if(id=="eredin_bringer_of_death") {
    const auto* target=chooseMedicTarget(game,player.graveyard);
    return target?std::vector<std::string>{target->id}:std::vector<std::string>{};
  }
  if(id=="eredin_destroyer") {
    auto discard=discardOrder(game,who);
    std::vector<std::string> chosen;
    for(const auto& card:discard)if(card.power<7 && chosen.size()<2)chosen.push_back(card.id);
    if(chosen.size()==2&&!player.deck.empty())chosen.push_back(player.deck.front().id);
    return chosen;
  }
  if(id=="eredin_king") {
    const Card* best=nullptr;int score=-1;
    for(const auto& card:player.deck)if(card.kind==Kind::Weather&&!card.abilities.empty()) {
      const int curr=weightWeatherFromDeck(game,who,card.abilities.front());
      if(curr>score){score=curr;best=&card;}
    }
    return best?std::vector<std::string>{best->id}:std::vector<std::string>{};
  }
  (void)data;
  return {};
}
AiAction cardAction(Game& game,int who,const Card& card,const BoardData& data) {
  AiAction action;action.cardId=card.id;
  if(card.kind==Kind::Decoy || has(card,"decoy")) {
    const Card* target=nullptr;
    auto lowest=[&](const std::vector<const Card*>& cards)->const Card* {
      if(cards.empty())return nullptr;
      return *std::min_element(cards.begin(),cards.end(),[&](const Card* a,const Card* b){
        return currentBoardCardPower(game,who,a)<currentBoardCardPower(game,who,b);
      });
    };
    target=lowest(data.ownSpies);
    if(!target && !data.ownMedics.empty())target=data.ownMedics[roll(game)%data.ownMedics.size()];
    if(!target && !data.ownScorches.empty())target=data.ownScorches[roll(game)%data.ownScorches.size()];
    if(!target&&!data.decoyFallbackOwnRowMaxima.empty())
      target=data.decoyFallbackOwnRowMaxima[roll(game)%data.decoyFallbackOwnRowMaxima.size()];
    if(target)action.selectedCardId=target->id;
  } else if(card.kind==Kind::Unit && card.medic) {
    const auto* target=chooseMedicTarget(game,game.players[who].graveyard);
    if(target) {
      action.selectedCardId=target->id;
      if(target->agile) {
        auto difference=[&](int row) {
          auto cards=game.players[who].board[row];cards.push_back(*target);
          return scoreRow(game,who,row,cards,game.weather[row],game.horn[who][row])-
                 currentRowScore(game,who,row);
        };
        const int close=difference(0),ranged=difference(1);
        action.target=close>ranged?Row::Melee:ranged>close?Row::Ranged:
          (roll(game)%2?Row::Melee:Row::Ranged);
      }
    }
  } else if(card.kind==Kind::CommanderHorn) {
    int best=0,bestRow=0;
    // Source ControllerAI.horn() visits close, ranged, then siege and keeps
    // the first row when the score gain ties.
    for(const int row:{0,1,2}) {
      const int score=weightHornRow(game,who,row);
      if(score>best){best=score;bestRow=row;}
    }
    action.target=static_cast<Row>(bestRow);
  } else if(card.kind==Kind::Mardroeme || has(card,"mardroeme")) {
    const int ranged=weightMardroemeRow(game,who,card,1),close=weightMardroemeRow(game,who,card,0);
    action.target=close>ranged?Row::Melee:Row::Ranged;
  } else if(card.agile) {
    // determineAgileRow compares the full gain, including this card's own
    // weather-adjusted power; weightRowChange intentionally excludes it.
    auto gain=[&](int row) {
      auto cards=game.players[who].board[row];cards.push_back(card);
      return scoreRow(game,who,row,cards,game.weather[row],game.horn[who][row])-
             currentRowScore(game,who,row);
    };
    const int close=gain(0),ranged=gain(1);
    action.target=static_cast<Row>(ranged>close?1:close>ranged?0:(roll(game)%2));
  }
  return action;
}
AiAction chooseLeaderAction(Game& game,int who,const BoardData& data) {
  AiAction action;action.activateLeader=true;
  if(!game.players[who].leader.abilities.empty())
    action.leaderSelection=leaderSelection(game,who,game.players[who].leader.abilities.front(),data);
  return action;
}
AiAction resolveSelectedAction(Game& game,int who,const AiAction& intent,const BoardData& data) {
  if(intent.pass)return intent;
  if(intent.activateLeader)return chooseLeaderAction(game,who,data);
  const auto& hand=game.players[who].hand;
  const auto card=std::find_if(hand.begin(),hand.end(),[&](const Card& candidate) {
    return candidate.id==intent.cardId;
  });
  return card==hand.end()?intent:cardAction(game,who,*card,data);
}
}

AiAction chooseAiAction(Game& game,int who) {
  if(who<0||who>1||who!=game.currentPlayer||
     (game.phase!=Phase::PlayerTurn&&game.phase!=Phase::OpponentTurn))return {true,{},{},{}};
  const auto& player=game.players[who];
  const auto& opponent=game.players[1-who];
  if(opponent.passed && (player.score>opponent.score ||
       (player.faction==Faction::Nilfgaard && player.score==opponent.score)))return {true,{},{},{}};

  const BoardData data=collectBoardData(game,who);
  std::vector<WeightedAction> actions;
  actions.reserve(player.hand.size()+2);
  for(const auto& card:player.hand) {
    const double weight=std::max(0.0,weightCard(game,who,card,data));
    // ControllerAI first computes every weight, then rolls once to choose an
    // action, and only then resolves random targets (Decoy/Medic/Leader).
    // Check legality against a throwaway state so candidate evaluation cannot
    // consume the match RNG or preselect a target for an unchosen action.
    Game preview=game;
    const auto previewAction=cardAction(preview,who,card,data);
    const bool legal=playCard(preview,who,previewAction.cardId,previewAction.target,previewAction.selectedCardId);
    AiAction intent;intent.cardId=card.id;
    actions.push_back({intent,static_cast<double>(legal?weight:0),legal});
  }
  if(player.leaderAbilityAvailable && !player.leader.abilities.empty()) {
    int weight=weightLeader(game,who,data,player.leader.abilities.front());
    Game preview=game;
    const auto previewAction=chooseLeaderAction(preview,who,data);
    const bool legal=activateLeader(preview,who,previewAction.target,previewAction.leaderSelection);
    if(!legal)weight=0;
    AiAction intent;intent.activateLeader=true;
    actions.push_back({intent,static_cast<double>(weight),legal});
  }
  double passWeight=0.0;
  if(2-player.roundLosses>1) {
    const int difference=opponent.score-player.score;
    if(difference>30)passWeight=100;
    else if(difference < -30 && static_cast<int>(opponent.hand.size())-static_cast<int>(player.hand.size())>2)passWeight=100;
    else passWeight=std::abs(difference);
  }
  actions.push_back({AiAction{true,{},{},{}},passWeight,true});

  double total=0;
  for(const auto& action:actions)total+=action.weight;
  if(total<=0) {
    for(const auto& card:player.hand)
      // The source fallback checks card.row === "weather", which includes
      // Clear Weather as well as the typed weather cards.
      if(((card.kind==Kind::Weather || card.kind==Kind::ClearWeather) &&
          weightWeather(game,who,card)>-1) ||
         has(card,"avenger") || has(card,"avenger_kambi")) {
        AiAction intent;intent.cardId=card.id;
        return resolveSelectedAction(game,who,intent,data);
      }
    return {true,{},{},{}};
  }
  const double ticket=std::floor((static_cast<double>(roll(game))/4294967296.0)*total);
  double passed=0;
  for(const auto& action:actions) {
    passed+=action.weight;
    if(ticket<passed)return resolveSelectedAction(game,who,action.action,data);
  }
  return resolveSelectedAction(game,who,actions.back().action,data);
}

bool performAiMulligan(Game& game,int who) {
  if(who<0||who>1||game.phase!=Phase::Mulligan||game.scoiataelChoicePending||game.players[who].mulliganDone)
    return false;
  auto& player=game.players[who];
  for(int redraw=0;redraw<2;++redraw) {
    const auto order=discardOrder(game,who);
    if(order.empty()||order.front().power>=15)break;
    const auto handCard=std::find_if(player.hand.begin(),player.hand.end(),[&](const Card& card){return card.id==order.front().id;});
    if(handCard==player.hand.end())break;
    const auto handIndex=static_cast<std::size_t>(handCard-player.hand.begin());
    const Card discarded=*handCard;
    player.hand.erase(handCard);
    // Deck.swap() uses Deck.addCardRandom(), then immediately draws index 0.
    player.deck.push_back(discarded);
    const auto randomIndex=static_cast<std::size_t>(roll(game))%player.deck.size();
    std::swap(player.deck[randomIndex],player.deck.back());
    Card drawn=std::move(player.deck.front());
    player.deck.erase(player.deck.begin());
    insertSourceHandCard(player,std::move(drawn),who,static_cast<int>(handIndex));
  }
  return replaceMulliganCards(game,who,{});
}
}
