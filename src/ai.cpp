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
struct WeightedAction { AiAction action; double weight{0.0}; };
struct BoardData {
  std::vector<const Card*> spies, medics, scorches;
  std::array<std::vector<const Card*>,2> graveSpies, graveMedics, graveScorches;
  std::array<std::vector<const Card*>,3> strongestOwn, strongestEnemy;
  int ownMax{-1}, enemyMax{-1};
};

bool has(const Card& card,const std::string& ability) {
  return std::find(card.abilities.begin(),card.abilities.end(),ability)!=card.abilities.end();
}
bool isUnit(const Card& card) { return card.kind==Kind::Unit && !card.hero; }
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
void collectCards(const std::vector<Card>& cards,int owner,BoardData& data,bool grave=false) {
  for(const auto& card:cards) {
    if(!isUnit(card)) continue;
    if(card.spy) { if(!grave)data.spies.push_back(&card); else data.graveSpies[owner].push_back(&card); }
    if(card.medic) { if(!grave)data.medics.push_back(&card); else data.graveMedics[owner].push_back(&card); }
    if(hasScorch(card)) { if(!grave)data.scorches.push_back(&card); else data.graveScorches[owner].push_back(&card); }
  }
}
BoardData collectBoardData(const Game& game,int who) {
  BoardData data;
  for(int owner=0;owner<2;++owner) {
    for(const auto& row:game.players[owner].board) collectCards(row,owner,data);
    collectCards(game.players[owner].graveyard,owner,data,true);
    int best=-1;
    for(int row=0;row<3;++row) for(const auto& card:game.players[owner].board[row])
      if(isUnit(card)) best=std::max(best,cardPower(game,owner,row,card,game.players[owner].board[row]));
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
std::vector<const Card*> allStrongest(const BoardData& data,int ownerSide) {
  const auto& rows=ownerSide==0?data.strongestOwn:data.strongestEnemy;
  std::vector<const Card*> result;
  for(const auto& row:rows)result.insert(result.end(),row.begin(),row.end());
  return result;
}
int weightRowChange(const Game& game,int who,int row,const Card& card) {
  auto cards=game.players[who].board[row];
  const int before=scoreRow(game,who,row,cards,game.weather[row],game.horn[who][row]);
  if(card.kind==Kind::Unit)cards.push_back(card);
  const int after=scoreRow(game,who,row,cards,game.weather[row],game.horn[who][row]);
  int change=after-before;
  if(card.kind==Kind::Unit)
    change-=cardPower(game,who,row,card,cards);
  return std::max(0,change);
}
int weightHornRow(const Game& game,int who,int row) {
  if(game.horn[who][row])return 0;
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
    value+=(enemyAfter-enemyBefore)-(ownAfter-ownBefore);
  }
  return value;
}
int weightScorchRow(const Game& game,int who,int row) {
  const int enemy=1-who;
  if(currentRowScore(game,enemy,row)<10)return 0;
  int strongest=-1,total=0;
  for(const auto& card:game.players[enemy].board[row]) if(isUnit(card))
    strongest=std::max(strongest,cardPower(game,enemy,row,card,game.players[enemy].board[row]));
  for(const auto& card:game.players[enemy].board[row]) if(isUnit(card) && !card.hero &&
       cardPower(game,enemy,row,card,game.players[enemy].board[row])==strongest)
    total+=cardPower(game,enemy,row,card,game.players[enemy].board[row]);
  return total;
}
int weightGlobalScorch(const BoardData& data) {
  if(data.ownMax>data.enemyMax)return 0;
  if(data.ownMax<data.enemyMax)return totalMaxPower(data,1);
  return std::max(0,totalMaxPower(data,1)-totalMaxPower(data,0));
}
int weightMardroemeRow(const Game& game,int who,int row) {
  if(game.horn[who][row])return 0;
  const std::string target=row==1?"Young Berserker":"Berserker";
  int count=0;
  for(const auto& card:game.players[who].board[row])if(card.name==target)++count;
  return std::max(1,row==2?10*count:8*count*count-2*count);
}
bool hasInHand(const Player& player,const std::string& ability) {
  return std::any_of(player.hand.begin(),player.hand.end(),[&](const Card& c){return has(c,ability);});
}
int weightBerserker(const Game& game,int who,int row,const Card& card,int score) {
  if(!hasInHand(game.players[who],"mardroeme"))return score;
  score-=card.power;
  if(row==0)return std::max(1,score+14);
  int count=0;
  const std::string transformed=card.name=="Young Berserker"?"Transformed Young Vildkaarl":"Transformed Vildkaarl";
  for(const auto& unit:game.players[who].board[row])if(unit.name==transformed)++count;
  score=8*((count+1)*(count+1)-count*count)+count*score;
  return std::max(1,score);
}
int weightMedic(const Game& game,int who,const BoardData& data,int score,int graveOwner) {
  bool hasEligible=false;
  for(const auto& card:game.players[graveOwner].graveyard)if(isUnit(card)){hasEligible=true;break;}
  if(!hasEligible)return std::min(1,score);
  int bonus=0;
  const int opponent=1-who;
  if(!data.graveSpies[opponent].empty())bonus=50;
  else if(!data.graveMedics[opponent].empty())bonus=15;
  else if(!data.graveScorches[opponent].empty())bonus=10;
  else if(game.players[who].roundLosses==1)bonus=1;
  return score+bonus;
}
int weightDecoy(const BoardData& data) {
  if(!data.spies.empty())return 50;
  if(!data.medics.empty())return 15;
  if(!data.scorches.empty())return 10;
  return data.ownMax>=0?1:0;
}
std::vector<Card> discardOrder(const Player& player) {
  std::vector<Card> cards;
  std::map<std::string,std::vector<Card>> groups;
  std::vector<Card> musters;
  for(const auto& card:player.hand)if(card.muster||has(card,"muster"))musters.push_back(card);
  while(!musters.empty()) {
    Card current=musters.back(); musters.pop_back();
    const auto dash=current.name.find('-');
    const std::string prefix=dash==std::string::npos?current.name:current.name.substr(0,dash);
    auto& group=groups[prefix]; group.push_back(current);
    for(int i=static_cast<int>(musters.size())-1;i>=0;--i) if(musters[i].name.rfind(prefix,0)==0) {
      group.push_back(musters[static_cast<std::size_t>(i)]);
      musters.erase(musters.begin()+i);
    }
  }
  for(auto& pair:groups) {
    auto& group=pair.second;
    std::sort(group.begin(),group.end(),[](const Card& a,const Card& b){return a.power<b.power;});
    if(!group.empty())group.pop_back();
    cards.insert(cards.end(),group.begin(),group.end());
  }
  std::vector<Card> weather;
  for(const auto& card:player.hand)if(card.kind==Kind::Weather)weather.push_back(card);
  if(weather.size()>1) {
    weather.erase(weather.begin());
    cards.insert(cards.end(),weather.begin(),weather.end());
  }
  std::vector<Card> normal;
  for(const auto& card:player.hand)if(card.abilities.empty())normal.push_back(card);
  std::sort(normal.begin(),normal.end(),[](const Card& a,const Card& b){return a.power<b.power;});
  cards.insert(cards.end(),normal.begin(),normal.end());
  return cards;
}
const Card* chooseMedicTarget(const std::vector<Card>& grave) {
  std::vector<const Card*> units;
  for(const auto& card:grave)if(isUnit(card))units.push_back(&card);
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
  if(!scorches.empty())return scorches.front();
  return *std::max_element(units.begin(),units.end(),[](const Card* a,const Card* b){return a->power<b->power;});
}
int rowForTag(const std::string& tag) {
  if(tag=="scorch_c")return 0;
  if(tag=="scorch_r")return 1;
  if(tag=="scorch_s")return 2;
  return -1;
}
int weightCard(const Game& game,int who,const Card& card,const BoardData& data) {
  if(card.kind==Kind::Decoy || has(card,"decoy"))return weightDecoy(data);
  if(card.kind==Kind::Scorch || has(card,"scorch"))return weightGlobalScorch(data);
  if(card.kind==Kind::CommanderHorn) {
    int best=0;for(int row=0;row<3;++row)best=std::max(best,weightHornRow(game,who,row));
    return best/2;
  }
  if(card.kind==Kind::Mardroeme || has(card,"mardroeme"))
    return std::max(weightMardroemeRow(game,who,1),weightMardroemeRow(game,who,2));
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
  return weightCard(game,who,virtualWeather,collectBoardData(game,who));
}
int weightLeader(const Game& game,int who,const BoardData& data,const std::string& id) {
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
  if(id=="eredin_destroyer") {
    auto discard=discardOrder(game.players[who]);
    if(discard.size()<2)return 0;
    discard.resize(2);
    discard.erase(std::remove_if(discard.begin(),discard.end(),[](const Card& c){return c.power>=7;}),discard.end());
    if(discard.size()<2)return 0;
    return discard.front().muster||has(discard.front(),"muster")?50:25;
  }
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
    Game after=game;
    if(!activateLeader(after,who))return 0;
    const int before=calculateScore(game.players[who],game.weather,game.horn[who]);
    return std::max(0,after.players[who].score-before);
  }
  if(id=="crach_an_craite") {
    if(game.round<2)return 0;
    if(hasInHand(game.players[who],"medic")||hasInHand(game.players[who],"spy"))return 0;
    if(hasInHand(game.players[who],"decoy") &&
       (!data.graveMedics[who].empty() || (!data.graveSpies[who].empty() &&
        std::any_of(game.players[who].deck.begin(),game.players[who].deck.end(),[](const Card& c){return c.medic;}))))return 0;
    return 15;
  }
  return 10+(game.round-1)*15;
}
std::vector<std::string> leaderSelection(const Game& game,int who,const std::string& id,const BoardData& data) {
  const auto& player=game.players[who];
  if(id=="emhyr_relentless") {
    const auto* target=chooseMedicTarget(game.players[1-who].graveyard);
    return target?std::vector<std::string>{target->id}:std::vector<std::string>{};
  }
  if(id=="eredin_bringer_of_death") {
    const auto* target=chooseMedicTarget(player.graveyard);
    return target?std::vector<std::string>{target->id}:std::vector<std::string>{};
  }
  if(id=="eredin_destroyer") {
    auto discard=discardOrder(player);
    std::vector<std::string> chosen;
    for(const auto& card:discard)if(card.power<7 && chosen.size()<2)chosen.push_back(card.id);
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
    auto lowest=[](const std::vector<const Card*>& cards)->const Card* {
      if(cards.empty())return nullptr;
      return *std::min_element(cards.begin(),cards.end(),[](const Card* a,const Card* b){return a->power<b->power;});
    };
    target=lowest(data.spies);
    if(!target && !data.medics.empty())target=data.medics[roll(game)%data.medics.size()];
    if(!target && !data.scorches.empty())target=data.scorches[roll(game)%data.scorches.size()];
    if(!target) {
      const auto own=allStrongest(data,0);
      if(!own.empty())target=own[roll(game)%own.size()];
    }
    if(target)action.selectedCardId=target->id;
  } else if(card.kind==Kind::Unit && card.medic) {
    const auto* target=chooseMedicTarget(game.players[who].graveyard);
    if(target)action.selectedCardId=target->id;
  } else if(card.kind==Kind::CommanderHorn) {
    int best=-1,bestRow=0;
    for(int row=0;row<3;++row) {
      const int score=weightHornRow(game,who,row);
      if(score>best){best=score;bestRow=row;}
    }
    action.target=static_cast<Row>(bestRow);
  } else if(card.kind==Kind::Mardroeme || has(card,"mardroeme")) {
    const int score1=weightMardroemeRow(game,who,1),score2=weightMardroemeRow(game,who,2);
    action.target=static_cast<Row>(score2>score1?2:1);
  } else if(card.agile) {
    const int close=weightRowChange(game,who,0,card),ranged=weightRowChange(game,who,1,card);
    action.target=static_cast<Row>(ranged>close?1:close>ranged?0:(roll(game)%2));
  }
  return action;
}
AiAction chooseLeaderAction(const Game& game,int who,const BoardData& data) {
  AiAction action;action.activateLeader=true;
  if(!game.players[who].leader.abilities.empty())
    action.leaderSelection=leaderSelection(game,who,game.players[who].leader.abilities.front(),data);
  return action;
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
    const auto choice=cardAction(game,who,card,data);
    actions.push_back({choice,static_cast<double>(std::max(0,weightCard(game,who,card,data)))});
  }
  if(player.leaderAbilityAvailable && !player.leader.abilities.empty()) {
    const auto choice=chooseLeaderAction(game,who,data);
    actions.push_back({choice,static_cast<double>(std::max(0,weightLeader(game,who,data,player.leader.abilities.front())))});
  }
  double passWeight=0.0;
  if(2-player.roundLosses>1) {
    const int difference=opponent.score-player.score;
    if(difference>30)passWeight=100;
    else if(difference < -30 && static_cast<int>(opponent.hand.size())-static_cast<int>(player.hand.size())>2)passWeight=100;
    else passWeight=std::abs(difference);
  }
  actions.push_back({AiAction{true,{},{},{}},passWeight});
  double total=0;
  for(const auto& action:actions)total+=action.weight;
  if(total<=0) {
    for(const auto& card:player.hand)
      if((card.kind==Kind::Weather && weightWeather(game,who,card)>-1) ||
         has(card,"avenger") || has(card,"avenger_kambi"))
        return cardAction(game,who,card,data);
    return {true,{},{},{}};
  }
  const double ticket=(static_cast<double>(roll(game))/4294967296.0)*total;
  double passed=0;
  for(const auto& action:actions) {
    passed+=action.weight;
    if(ticket<passed)return action.action;
  }
  return actions.back().action;
}

std::vector<std::string> chooseAiMulligan(const Game& game,int who) {
  if(who<0||who>1||game.phase!=Phase::Mulligan||game.players[who].mulliganDone)return {};
  Player simulated=game.players[who];
  std::vector<std::string> selected;
  for(int redraw=0;redraw<2;++redraw) {
    const auto order=discardOrder(simulated);
    const auto candidate=std::find_if(order.begin(),order.end(),[](const Card& card){return card.power<15;});
    if(candidate==order.end())break;
    const auto handCard=std::find_if(simulated.hand.begin(),simulated.hand.end(),[&](const Card& card){return card.id==candidate->id;});
    if(handCard==simulated.hand.end()||simulated.deck.empty())break;
    const Card discarded=*handCard;
    selected.push_back(discarded.id);
    simulated.hand.erase(handCard);
    simulated.deck.insert(simulated.deck.begin(),discarded);
    simulated.hand.push_back(simulated.deck.back());
    simulated.deck.pop_back();
  }
  return selected;
}
}
