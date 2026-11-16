#include <SDL.h>
#include "gwent/audio.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"
#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include "gwent/deck.hpp"
#include "gwent/game.hpp"
#include "gwent/save.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr int W=640,H=480,STAGE_H=360;
constexpr int MATCH_RULES_PANEL_Y=423,MATCH_RULES_PANEL_H=51;
constexpr int DECK_RULES_PANEL_Y=368,DECK_RULES_PANEL_H=104;
struct Color { Uint8 r,g,b,a{255}; };
constexpr Color ink{13,19,20}, panel{25,34,34}, panelLight{37,48,46}, gold{196,156,78},
                paper{229,218,186}, muted{145,148,132}, green{94,137,102}, blue{87,125,143};
using Glyph=std::array<Uint8,7>;
const std::unordered_map<char,Glyph>& font() {
  static const std::unordered_map<char,Glyph> glyphs={
    {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},{'C',{14,17,16,16,16,17,14}},
    {'D',{30,17,17,17,17,17,30}},{'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,15}},{'H',{17,17,17,31,17,17,17}},{'I',{14,4,4,4,4,4,14}},
    {'J',{7,2,2,2,18,18,12}},{'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},{'O',{14,17,17,17,17,17,14}},
    {'P',{30,17,17,30,16,16,16}},{'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},{'U',{17,17,17,17,17,17,14}},
    {'V',{17,17,17,17,17,10,4}},{'W',{17,17,17,21,21,21,10}},{'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},{'2',{14,17,1,2,4,8,31}},
    {'3',{30,1,1,14,1,1,30}},{'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},{'8',{14,17,17,14,17,17,14}},
    {'9',{14,17,17,15,1,1,14}},{':',{0,4,4,0,4,4,0}},{'.',{0,0,0,0,0,6,6}},
    {',',{0,0,0,0,4,4,8}},{'!',{4,4,4,4,4,0,4}},{'?',{14,17,1,2,4,0,4}},
    {'-',{0,0,0,31,0,0,0}},{'+',{0,4,4,31,4,4,0}},{'/',{1,2,2,4,8,8,16}},
    {'(',{2,4,8,8,8,4,2}},{')',{8,4,2,2,2,4,8}},{'\'',{4,4,8,0,0,0,0}},
    {'"',{10,10,0,0,0,0,0}},{'=',{0,31,0,31,0,0,0}},{'*',{0,21,14,31,14,21,0}},
    {'<',{2,4,8,16,8,4,2}},{'>',{8,4,2,1,2,4,8}},{'_',{0,0,0,0,0,0,31}},
    {'\x01',{14,17,17,31,17,19,21}},{'\x02',{2,14,17,16,16,17,14}},
    {'\x03',{14,17,16,30,16,17,15}},{'\x04',{16,16,16,21,16,16,31}},
    {'\x05',{2,17,25,21,19,17,17}},{'\x06',{2,14,17,17,17,17,14}},
    {'\x07',{2,15,16,14,1,1,30}},{'\x08',{2,31,1,2,4,8,31}},
    {'\x09',{4,31,1,2,4,8,31}},
    {'[',{14,8,8,8,8,8,14}},{']',{14,2,2,2,2,2,14}},{' ',{0,0,0,0,0,0,0}}
  };
  return glyphs;
}
std::string ascii(const std::string& value) {
  std::string out;
  for(std::size_t i=0;i<value.size();) {
    const unsigned char a=static_cast<unsigned char>(value[i]);
    if(a<128) { out.push_back(static_cast<char>(std::toupper(a))); ++i; continue; }
    if(i+1<value.size()) {
      const unsigned char b=static_cast<unsigned char>(value[i+1]);
      char base='?';
      if(a==0xC4) {
        if(b==0x85||b==0x84)base='\x01';
        else if(b==0x87||b==0x86)base='\x02';
        else if(b==0x99||b==0x98)base='\x03';
      }
      if(a==0xC5) {
        if(b==0x82||b==0x81)base='\x04';
        else if(b==0x84||b==0x83)base='\x05';
        else if(b==0x9B||b==0x9A)base='\x07';
        else if(b==0xBA||b==0xB9)base='\x08';
        else if(b==0xBC||b==0xBB)base='\x09';
      }
      if(a==0xC3 && (b==0xB3||b==0x93))base='\x06';
      out.push_back(base); i+=2;
    } else { out.push_back('?'); ++i; }
  }
  return out;
}
void color(SDL_Renderer* r,Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
void box(SDL_Renderer* r,int x,int y,int w,int h,Color c) { color(r,c); SDL_Rect q{x,y,w,h}; SDL_RenderFillRect(r,&q); }
void frame(SDL_Renderer* r,int x,int y,int w,int h,Color c) { color(r,c); SDL_Rect q{x,y,w,h}; SDL_RenderDrawRect(r,&q); }
void polishMark(SDL_Renderer* r,char ch,int x,int y,int scale,Color c) {
  if(ch<'\x01'||ch>'\x09')return;
  color(r,c);
  if(ch=='\x01'||ch=='\x03') {
    SDL_Rect lower{x+4*scale,y+6*scale,scale,scale};SDL_RenderFillRect(r,&lower);
  } else if(ch=='\x09') {
    SDL_Rect dot{x+2*scale,y-2*scale,scale,scale};SDL_RenderFillRect(r,&dot);
  } else {
    SDL_Rect first{x+3*scale,y-scale,scale,scale};SDL_Rect second{x+2*scale,y-2*scale,scale,scale};
    SDL_RenderFillRect(r,&first);SDL_RenderFillRect(r,&second);
  }
}
void text(SDL_Renderer* r,int x,int y,const std::string& value,Color c,int scale=1) {
  color(r,c); const std::string s=ascii(value); int cx=x;
  for(char ch:s) {
    const auto it=font().find(ch); const Glyph g=it==font().end()?font().at('?'):it->second;
    for(int row=0;row<7;++row) for(int col=0;col<5;++col) if(g[row]&(1u<<(4-col))) {
      SDL_Rect p{cx+col*scale,y+row*scale,scale,scale}; SDL_RenderFillRect(r,&p);
    }
    polishMark(r,ch,cx,y,scale,c);
    cx+=(6*scale);
  }
}
void narrowText(SDL_Renderer* r,int x,int y,const std::string& value,Color c,int advance=4) {
  color(r,c);const std::string s=ascii(value);int cx=x;
  for(char ch:s) {
    const auto it=font().find(ch);const Glyph g=it==font().end()?font().at('?'):it->second;
    for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(g[row]&(1u<<(4-col))) {
      SDL_Rect p{cx+(col*advance)/5,y+row,1,1};SDL_RenderFillRect(r,&p);
    }
    polishMark(r,ch,cx,y,1,c);
    cx+=advance;
  }
}
int textWidth(const std::string& s,int scale=1) { return static_cast<int>(ascii(s).size())*6*scale; }
std::string shortName(std::string s,int n) { if(static_cast<int>(ascii(s).size())>n) s=ascii(s).substr(0,n); return ascii(s); }
void centeredTextInBox(SDL_Renderer* r,int x,int y,int width,int height,const std::string& value,Color c,int scale=1) {
  const int visibleWidth=std::max(0,(static_cast<int>(ascii(value).size())*6-1)*scale);
  const int visibleHeight=7*scale;
  text(r,x+std::max(0,(width-visibleWidth)/2),y+std::max(0,(height-visibleHeight)/2),value,c,scale);
}
void tallTextInBox(SDL_Renderer* r,int x,int y,int width,int height,const std::string& value,Color c,bool shadow=false) {
  const std::string s=ascii(value);
  if(s.empty())return;
  const int advance=std::max(1,std::min(6,width/static_cast<int>(s.size())));
  const int textWidthPx=advance*static_cast<int>(s.size());
  const int startX=x+(width-textWidthPx)/2,startY=y+std::max(0,(height-14)/2);
  const auto draw=[&](int offsetX,int offsetY,Color c) {
    color(r,c);int cx=startX+offsetX;
    for(char ch:s) {
      const auto it=font().find(ch);const Glyph g=it==font().end()?font().at('?'):it->second;
      for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(g[row]&(1u<<(4-col))) {
        SDL_Rect p{cx+(col*advance)/5,startY+row*2+offsetY,1,2};SDL_RenderFillRect(r,&p);
      }
      cx+=advance;
    }
  };
  if(shadow&&height>14)draw(1,1,Color{229,218,186,210});
  draw(0,0,c);
}
void rowScoreText(SDL_Renderer* r,int x,int y,const std::string& value) {
  tallTextInBox(r,x,y,17,44,value,ink,true);
}

enum class Screen { DeckSelect, DeckBuilder, Match };
enum class Language { Polish, English };
constexpr Uint32 SOURCE_CARD_PREVIEW_MS=1000;
constexpr Uint32 SOURCE_CARD_FLIGHT_MS=499;
constexpr Uint32 SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS=1000;
constexpr Uint32 SOURCE_ABILITY_ANIMATION_MS=1650;
struct App {
  enum class CardZone { Board, Weather, Hand, Deck, Grave };
  struct CardFlight {
    bool active{false};
    gwent::Card card;
    CardZone destination{CardZone::Board};
    int owner{-1};
    int row{-1};
    int fromX{0},fromY{0},toX{0},toY{0};
    Uint32 startedAt{0};
  };
  struct CardEffect {
    bool active{false};
    std::string icon;
    int x{0},y{0};
    Uint32 startedAt{0};
    Uint32 duration{1650};
  };
  struct ScheduledSound { std::string key;Uint32 startedAt{0}; };
  struct RowLayout {
    bool initialized{false};
    std::size_t cardCount{0};
    double fromX{0},fromPitch{0},toX{0},toPitch{0};
    Uint32 startedAt{0};
  };
  Screen screen{Screen::DeckBuilder};
  Language language{Language::Polish};
  std::size_t deckIndex{0};
  int deckSlot{1};
  bool userDeckSelected{false};
  bool editingLeader{false};
  bool cardDetailsOpen{false};
  bool miyooControls{false};
  bool leaderSelectionMode{false};
  bool leaderRevealOpen{false};
  bool passConfirmationOpen{false};
  bool pauseOpen{false};
  int pauseIndex{0};
  bool sfxEnabled{true};
  bool musicEnabled{true};
  bool notificationsEnabled{true};
  std::string activeNotification;
  Uint32 notificationStartedAt{0};
  Uint32 endScreenStartedAt{0};
  std::vector<std::string> notificationQueue;
  std::vector<std::string> leaderSelections;
  CardFlight cardFlight;
  std::vector<CardFlight> triggeredCardFlights;
  std::vector<CardEffect> cardEffects;
  std::vector<ScheduledSound> scheduledSounds;
  std::array<std::array<bool,3>,2> weatherFrom{};
  std::array<std::array<bool,3>,2> weatherTo{};
  std::array<std::array<Uint32,3>,2> weatherChangedAt{};
  std::array<std::array<bool,3>,2> weatherTransition{};
  std::array<std::array<std::string,3>,2> weatherOverlay{};
  std::array<std::array<RowLayout,3>,2> rowLayouts{};
  bool weatherInitialized{false};
  Uint32 selectDownAt{0};
  bool selectLongPressed{false};
  int editorPane{0};
  int editorCardIndex{0};
  gwent::UserDeck editDeck;
  gwent::Game game;
  int handIndex{0};
  int rowIndex{0};
  int reviveRowIndex{0};
  int targetIndex{0};
  Uint32 carouselMotionStartedAt{0};
  int carouselMotionDirection{0};
  bool targetMode{false};
  std::vector<std::string> selectedTargets;
  std::vector<std::string> mulligan;
  std::string savePath;
  std::string dataDirectory;
  std::string assetDirectory;
  mutable std::unordered_map<std::string,SDL_Texture*> textures;
  mutable std::vector<std::string> textureOrder;
  gwent_port::AudioSystem audio;
  std::string status{"PRESS ENTER"};
  Uint32 aiAt{0};
  Uint32 actionAnimationLockUntil{0};
  bool actionAnimationLockActive{false};
};
void animatePlayedCard(App& a,const gwent::Card& card,int player,int handIndex,int handSizeBefore);
void queueTriggeredEffects(App& a,const gwent::Game& before,const gwent::Card& played,int player);
void queueLeaderTransitionVisuals(App& a,const gwent::Game& before,const std::string& ability,int player);
double sourceEase(double progress) {
  progress=std::clamp(progress,0.0,1.0);
  double low=0.0,high=1.0,t=progress;
  for(int i=0;i<12;++i) {
    t=(low+high)*0.5;
    const double inverse=1.0-t;
    const double x=3.0*inverse*inverse*t*0.25+3.0*inverse*t*t*0.25+t*t*t;
    if(x<progress)low=t;else high=t;
  }
  const double inverse=1.0-t;
  return 3.0*inverse*inverse*t*0.1+3.0*inverse*t*t+t*t*t;
}
double sourceEaseIn(double progress) {
  progress=std::clamp(progress,0.0,1.0);
  double low=0.0,high=1.0,t=progress;
  for(int i=0;i<12;++i) {
    t=(low+high)*0.5;
    const double inverse=1.0-t;
    const double x=3.0*inverse*inverse*t*0.42+3.0*inverse*t*t+t*t*t;
    if(x<progress)low=t;else high=t;
  }
  return t*t*t;
}
struct SourceRowGeometry { double firstX{0},pitch{0}; };
SourceRowGeometry sourceRowGeometry(std::size_t cardCount) {
  constexpr double rowLeft=170.0,rowWidth=336.0,rowCardsLeft=0.20,rowCardsWidth=0.797;
  constexpr double cardWidth=4.45*W/100.0,smallMargin=0.075*W/100.0,overlapCoefficient=0.00325*W/100.0;
  const double areaX=rowLeft+rowWidth*rowCardsLeft;
  const double areaWidth=rowWidth*rowCardsWidth;
  if(cardCount==0)return {areaX+areaWidth/2.0,cardWidth+2*smallMargin};
  const double n=static_cast<double>(cardCount);
  const double sideMargin=cardCount<10?smallMargin:(areaWidth-cardWidth*n)/(2*n)-overlapCoefficient*n;
  const double pitch=cardWidth+2*sideMargin;
  const double total=n*pitch;
  return {areaX+(areaWidth-total)/2.0+sideMargin,pitch};
}
struct CardLocation {
  gwent::Card card;
  App::CardZone zone{App::CardZone::Board};
  int owner{-1},row{-1},index{0};
  std::size_t count{0};
};
std::vector<CardLocation> cardLocations(const gwent::Game& game) {
  std::vector<CardLocation> locations;
  for(int owner=0;owner<2;++owner) {
    const auto add=[&](const std::vector<gwent::Card>& cards,App::CardZone zone,int row=-1) {
      for(int i=0;i<static_cast<int>(cards.size());++i)
        locations.push_back({cards[static_cast<std::size_t>(i)],zone,owner,row,i,cards.size()});
    };
    add(game.players[owner].deck,App::CardZone::Deck);
    add(game.players[owner].hand,App::CardZone::Hand);
    add(game.players[owner].graveyard,App::CardZone::Grave);
    for(int row=0;row<3;++row)add(game.players[owner].board[static_cast<std::size_t>(row)],App::CardZone::Board,row);
  }
  for(int i=0;i<static_cast<int>(game.weatherCards.size());++i) {
    const auto& weather=game.weatherCards[static_cast<std::size_t>(i)];
    locations.push_back({weather.card,App::CardZone::Weather,weather.owner,-1,i,game.weatherCards.size()});
  }
  return locations;
}
std::pair<int,int> cardLocationPosition(const CardLocation& location) {
  switch(location.zone) {
    case App::CardZone::Board: {
      const auto geometry=sourceRowGeometry(location.count);
      return {static_cast<int>(std::lround(geometry.firstX+location.index*geometry.pitch)),
              location.owner==0?143+location.row*45:5+(2-location.row)*45};
    }
    case App::CardZone::Weather:return {47+location.index*29,149};
    case App::CardZone::Hand:
      if(location.owner!=0)return {292,20};
      return {194+(location.index-std::clamp(location.index-5,0,std::max(0,static_cast<int>(location.count)-11)))*29,277};
    case App::CardZone::Deck:
    case App::CardZone::Grave:return {location.zone==App::CardZone::Grave?514:572,location.owner==0?275:23};
  }
  return {0,0};
}
bool cardFlightHides(const App::CardFlight& flight,const gwent::Card& card,App::CardZone zone,int owner,int row=-1) {
  return flight.active&&flight.card.id==card.id&&flight.destination==zone&&flight.owner==owner&&
         (zone!=App::CardZone::Board||flight.row==row);
}
bool cardFlightHides(const App& app,const gwent::Card& card,App::CardZone zone,int owner,int row=-1) {
  if(cardFlightHides(app.cardFlight,card,zone,owner,row))return true;
  return std::any_of(app.triggeredCardFlights.begin(),app.triggeredCardFlights.end(),
    [&](const App::CardFlight& flight){return cardFlightHides(flight,card,zone,owner,row);});
}
bool sourceWaitsForRowPlacementAudio(const gwent::Card& card,const gwent::Game& before,int player,gwent::Row targetRow) {
  const auto hasAbility=[&](const char* key) {
    return std::find(card.abilities.begin(),card.abilities.end(),key)!=card.abilities.end();
  };
  const int row=static_cast<int>(targetRow);
  const bool rowHasMardroeme=row>=0&&row<3&&before.players[player].mardroeme[static_cast<std::size_t>(row)];
  return card.kind==gwent::Kind::Unit&&
    !card.spy&&!hasAbility("spy")&&!hasAbility("vildkarrl")&&
    !(card.berserker&&rowHasMardroeme)&&!hasAbility("decoy");
}
bool actionAnimationLocked(const App& app,Uint32 now) {
  return app.actionAnimationLockActive&&!SDL_TICKS_PASSED(now,app.actionAnimationLockUntil);
}
void extendAnimationDeadline(Uint32& deadline,Uint32 candidate) {
  if(static_cast<Sint32>(candidate-deadline)>0)deadline=candidate;
}
void lockInputUntilQueuedAnimationsFinish(App& app,Uint32 minimumUntil) {
  Uint32 deadline=minimumUntil;
  const auto includeFlight=[&](const App::CardFlight& flight) {
    if(!flight.active)return;
    Uint32 end=flight.startedAt+SOURCE_CARD_FLIGHT_MS;
    if(flight.destination==App::CardZone::Board&&flight.row>=0&&flight.row<3&&flight.card.kind==gwent::Kind::Unit) {
      const auto row=static_cast<gwent::Row>(flight.row);
      if(sourceWaitsForRowPlacementAudio(flight.card,app.game,flight.owner,row))end+=SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS;
      end+=600;
    }
    extendAnimationDeadline(deadline,end);
  };
  includeFlight(app.cardFlight);
  for(const auto& flight:app.triggeredCardFlights)includeFlight(flight);
  for(const auto& effect:app.cardEffects)if(effect.active)
    extendAnimationDeadline(deadline,effect.startedAt+effect.duration+600);
  for(const auto& sound:app.scheduledSounds)
    extendAnimationDeadline(deadline,sound.startedAt+SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS);
  app.actionAnimationLockUntil=deadline;
  app.actionAnimationLockActive=true;
}
void lockCardActionUntilAnimationsFinish(App& app,const gwent::Card& played,const gwent::Game& before,
                                         int player,gwent::Row targetRow) {
  const Uint32 now=SDL_GetTicks();
  Uint32 minimumUntil=now+SOURCE_CARD_PREVIEW_MS;
  if(app.cardFlight.active) {
    minimumUntil=app.cardFlight.startedAt+SOURCE_CARD_FLIGHT_MS;
    if(app.cardFlight.destination==App::CardZone::Board&&
       sourceWaitsForRowPlacementAudio(played,before,player,targetRow))
      minimumUntil+=SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS;
    minimumUntil+=600;
  }
  lockInputUntilQueuedAnimationsFinish(app,minimumUntil);
}
void queueCardTransitionFlights(App& app,const gwent::Game& before,const gwent::Card& played,int player) {
  const auto oldLocations=cardLocations(before),newLocations=cardLocations(app.game);
  const Uint32 now=SDL_GetTicks();
  const Uint32 previewEnd=now+SOURCE_CARD_PREVIEW_MS;
  const Uint32 cardArrival=previewEnd+SOURCE_CARD_FLIGHT_MS;
  const Uint32 placedEffectsStart=cardArrival+
    (sourceWaitsForRowPlacementAudio(played,before,player,played.row)?SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS:0);
  for(const auto& oldLocation:oldLocations) {
    const auto current=std::find_if(newLocations.begin(),newLocations.end(),[&](const CardLocation& item){return item.card.id==oldLocation.card.id;});
    if(current==newLocations.end()||
       (oldLocation.zone==current->zone&&oldLocation.owner==current->owner&&oldLocation.row==current->row))continue;
    if(current->zone==App::CardZone::Hand&&current->owner!=0)continue;
    if(app.cardFlight.active&&app.cardFlight.card.id==oldLocation.card.id)continue;
    Uint32 startsAt=now;
    if(played.kind==gwent::Kind::Scorch&&oldLocation.card.id==played.id&&current->zone==App::CardZone::Grave)
      startsAt=previewEnd+SOURCE_ABILITY_ANIMATION_MS;
    else if(played.medic&&oldLocation.zone==App::CardZone::Grave&&current->zone==App::CardZone::Board)
      startsAt=placedEffectsStart+SOURCE_ABILITY_ANIMATION_MS;
    else if(played.muster&&oldLocation.owner==player&&current->zone==App::CardZone::Board&&oldLocation.card.id!=played.id)
      startsAt=placedEffectsStart+SOURCE_ABILITY_ANIMATION_MS;
    else if(played.spy&&oldLocation.zone==App::CardZone::Deck&&current->zone==App::CardZone::Hand)
      startsAt=cardArrival+SOURCE_ABILITY_ANIMATION_MS;
    else if((played.kind==gwent::Kind::Scorch||
             std::find(played.abilities.begin(),played.abilities.end(),"scorch_c")!=played.abilities.end()||
             std::find(played.abilities.begin(),played.abilities.end(),"scorch_r")!=played.abilities.end()||
             std::find(played.abilities.begin(),played.abilities.end(),"scorch_s")!=played.abilities.end())&&
            oldLocation.zone==App::CardZone::Board&&current->zone==App::CardZone::Grave)
      startsAt=previewEnd+SOURCE_ABILITY_ANIMATION_MS;
    const auto [fromX,fromY]=cardLocationPosition(oldLocation);
    const auto [toX,toY]=cardLocationPosition(*current);
    App::CardFlight flight;flight.active=true;flight.card=current->card;flight.destination=current->zone;
    flight.owner=current->owner;flight.row=current->row;flight.fromX=fromX;flight.fromY=fromY;
    flight.toX=toX;flight.toY=toY;flight.startedAt=startsAt;
    app.triggeredCardFlights.push_back(std::move(flight));
  }
}
SourceRowGeometry currentRowGeometry(const App::RowLayout& layout,std::size_t cardCount,Uint32 now) {
  if(!layout.initialized)return sourceRowGeometry(cardCount);
  const Uint32 elapsed=std::min<Uint32>(now-layout.startedAt,250);
  const double progress=sourceEase(static_cast<double>(elapsed)/250.0);
  return {layout.fromX+(layout.toX-layout.fromX)*progress,
          layout.fromPitch+(layout.toPitch-layout.fromPitch)*progress};
}
void updateRowLayouts(App& a,Uint32 now) {
  for(int owner=0;owner<2;++owner)for(int row=0;row<3;++row) {
    auto& layout=a.rowLayouts[owner][row];
    const std::size_t count=a.game.players[owner].board[row].size();
    if(!layout.initialized) {
      const auto geometry=sourceRowGeometry(count);
      layout={true,count,geometry.firstX,geometry.pitch,geometry.firstX,geometry.pitch,now};
    } else if(layout.cardCount!=count) {
      const auto current=currentRowGeometry(layout,layout.cardCount,now);
      const auto target=sourceRowGeometry(count);
      layout.cardCount=count;layout.fromX=current.firstX;layout.fromPitch=current.pitch;
      layout.toX=target.firstX;layout.toPitch=target.pitch;layout.startedAt=now;
    }
  }
}
int sourceAbilityScalePercent(Uint32 elapsed) {
  const auto interpolate=[&](int from,int to,Uint32 time) {
    const double progress=sourceEase(static_cast<double>(time)/250.0);
    return static_cast<int>(std::lround(from+(to-from)*progress));
  };
  if(elapsed<50)return 40;
  if(elapsed<300)return interpolate(40,100,elapsed-50);
  if(elapsed<350)return 100;
  if(elapsed<600)return interpolate(100,80,elapsed-350);
  if(elapsed<1350)return 80;
  if(elapsed<1600)return interpolate(80,40,elapsed-1350);
  return 40;
}
const char* sourceWeatherOverlayForRow(int row) {
  static constexpr std::array<const char*,3> overlays{"frost","fog","rain"};
  return overlays[static_cast<std::size_t>(std::clamp(row,0,2))];
}
SDL_Texture* textureFor(SDL_Renderer* renderer,const App& app,const std::string& relative) {
  if(relative.empty()||app.assetDirectory.empty())return nullptr;
  const auto found=app.textures.find(relative);
  if(found!=app.textures.end()) {
    auto& order=app.textureOrder;
    order.erase(std::remove(order.begin(),order.end(),relative),order.end());
    order.push_back(relative);
    return found->second;
  }
  const auto path=std::filesystem::path(app.assetDirectory)/relative;
  int width=0,height=0,channels=0;
  unsigned char* pixels=stbi_load(path.string().c_str(),&width,&height,&channels,STBI_rgb_alpha);
  if(!pixels)return nullptr;
  SDL_Texture* loaded=nullptr;
  const int maxWidth=relative.rfind("img/lg/",0)==0?246:W;
  if(width>maxWidth||height>H) {
    const float scale=std::min({1.0f,static_cast<float>(maxWidth)/width,static_cast<float>(H)/height});
    const int scaledWidth=std::max(1,static_cast<int>(std::lround(width*scale)));
    const int scaledHeight=std::max(1,static_cast<int>(std::lround(height*scale)));
    SDL_Surface* source=SDL_CreateRGBSurfaceWithFormatFrom(pixels,width,height,32,width*4,SDL_PIXELFORMAT_RGBA32);
    SDL_Surface* scaled=SDL_CreateRGBSurfaceWithFormat(0,scaledWidth,scaledHeight,32,SDL_PIXELFORMAT_RGBA32);
    if(source&&scaled&&SDL_BlitScaled(source,nullptr,scaled,nullptr)==0)loaded=SDL_CreateTextureFromSurface(renderer,scaled);
    if(source)SDL_FreeSurface(source);
    if(scaled)SDL_FreeSurface(scaled);
  } else {
    loaded=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,width,height);
    if(loaded)SDL_UpdateTexture(loaded,nullptr,pixels,width*4);
  }
  if(loaded)SDL_SetTextureBlendMode(loaded,SDL_BLENDMODE_BLEND);
  stbi_image_free(pixels);
  app.textures.emplace(relative,loaded);
  app.textureOrder.push_back(relative);
  constexpr std::size_t maxCachedTextures=48;
  while(app.textureOrder.size()>maxCachedTextures) {
    const std::string oldest=app.textureOrder.front();
    app.textureOrder.erase(app.textureOrder.begin());
    const auto stale=app.textures.find(oldest);
    if(stale!=app.textures.end()) {
      SDL_RenderFlush(renderer);
      if(stale->second)SDL_DestroyTexture(stale->second);
      app.textures.erase(stale);
    }
  }
  return loaded;
}
void drawImage(SDL_Renderer* renderer,const App& app,const std::string& relative,int x,int y,int w,int h) {
  if(auto* image=textureFor(renderer,app,relative)) { SDL_Rect destination{x,y,w,h};SDL_RenderCopy(renderer,image,nullptr,&destination); }
}
int cardPortraitHeight(int width) {
  // The source card's painted portrait ends at the gold divider around 74.5%.
  // Size the crop at its natural aspect ratio so faces are never vertically stretched.
  return std::max(1,static_cast<int>(std::lround(width*0.745*775.0/410.0)));
}
void drawCardPortrait(SDL_Renderer* renderer,const App& app,const std::string& relative,int x,int y,int w,int h) {
  if(auto* image=textureFor(renderer,app,relative)) {
    int textureWidth=0,textureHeight=0;
    if(SDL_QueryTexture(image,nullptr,nullptr,&textureWidth,&textureHeight)!=0||textureWidth<=0||textureHeight<=0)return;
    SDL_Rect source{0,0,textureWidth,std::clamp(static_cast<int>(std::lround(textureHeight*0.745)),1,textureHeight)};
    SDL_Rect destination{x,y,w,h};
    SDL_RenderCopy(renderer,image,&source,&destination);
  }
}
void drawImageAlpha(SDL_Renderer* renderer,const App& app,const std::string& relative,int x,int y,int w,int h,Uint8 alpha) {
  if(auto* image=textureFor(renderer,app,relative)) {
    SDL_SetTextureAlphaMod(image,alpha);
    SDL_Rect destination{x,y,w,h};SDL_RenderCopy(renderer,image,nullptr,&destination);
    SDL_SetTextureAlphaMod(image,255);
  }
}
const std::vector<gwent::ReferenceDeck>& recipes() { return gwent::referencePremadeDecks(); }
std::string tr(const App& a,const std::string& key) {
  const bool pl=a.language==Language::Polish;
  if(key=="title")return pl?"GWINT KLASYCZNY":"GWENT CLASSIC";
  if(key=="start")return pl?"ENTER - ROZPOCZNIJ":"ENTER - START";
  if(key=="choose")return pl?"WYBIERZ TALIĘ":"CHOOSE A DECK";
  if(key=="opponent")return pl?"PRZECIWNIK":"OPPONENT";
  if(key=="opponent_short")return pl?"RYWAL":"OPPONENT";
  if(key=="player_short")return pl?"TY":"YOU";
  if(key=="notif_coin_me")return pl?"ZACZYNASZ TY":"YOU WILL GO FIRST";
  if(key=="notif_coin_op")return pl?"RYWAL ZACZYNA":"YOUR OPPONENT WILL GO FIRST";
  if(key=="notif_round_start")return pl?"POCZĄTEK RUNDY":"ROUND START";
  if(key=="notif_me_pass")return pl?"SPASOWAŁEŚ RUNDĘ":"ROUND PASSED";
  if(key=="notif_op_pass")return pl?"RYWAL SPASOWAŁ":"YOUR OPPONENT HAS PASSED";
  if(key=="notif_win_round")return pl?"WYGRYWASZ RUNDĘ!":"YOU WON THE ROUND!";
  if(key=="notif_lose_round")return pl?"RYWAL WYGRYWA RUNDĘ":"YOUR OPPONENT WON THE ROUND";
  if(key=="notif_draw_round")return pl?"REMIS W TEJ RUNDZIE":"THE ROUND ENDED IN A DRAW";
  if(key=="notif_me_turn")return pl?"TWÓJ RUCH!":"YOUR TURN!";
  if(key=="notif_op_turn")return pl?"RUCH RYWALA":"OPPONENT'S TURN";
  if(key=="ai")return pl?"PRZECIWNIK AI":"AI OPPONENT";
  if(key=="language")return pl?"JĘZYK":"LANGUAGE";
  if(key=="instructions")return pl?"LEWO/PRAWO - TALIA":"LEFT/RIGHT - DECK";
  if(key=="confirm")return pl?"ENTER - GRA   Z - JEZYK":"ENTER - PLAY   Z - LANGUAGE";
  if(key=="mulligan")return pl?"WYBIERZ DO 2 KART DO WYMIANY":"CHOOSE UP TO 2 CARDS TO MULLIGAN";
  if(key=="mulligan_controls")return pl?"A: WYBIERZ   START: ZATWIERDŹ / POMIŃ":"A: MARK CARD   START: CONFIRM / SKIP";
  if(key=="mulligan_details")return pl?"X: POWIĘKSZ KARTĘ   LEWO/PRAWO: WYBÓR":"X: ENLARGE CARD   LEFT/RIGHT: SELECT";
  if(key=="choose_first")return pl?"SCOIA'TAEL WYBIERA KOLEJKE: LEWO/PRAWO, ENTER":"SCOIA'TAEL CHOOSES FIRST: LEFT/RIGHT, ENTER";
  if(key=="human_first")return pl?"ZACZYNASZ TY":"YOU START";
  if(key=="ai_first")return pl?"ZACZYNA PRZECIWNIK":"OPPONENT STARTS";
  if(key=="skellige_revival")return pl?"SKELLIGE: WYBIERZ JEDNOSTKE Z CMENTARZA":"SKELLIGE: CHOOSE A UNIT FROM THE GRAVEYARD";
  if(key=="skellige_agile")return pl?"ZWINNA: GORA/DOL WYBIERA RZAD":"AGILE: UP/DOWN SELECTS ROW";
  if(key=="melee")return pl?"WALKA WRĘCZ":"MELEE";
  if(key=="ranged")return pl?"DYSTANSOWY":"RANGED";
  if(key=="controls")return pl?"A GRAJ  B WROC  X PODGLAD  Y LIDER":"A PLAY  B BACK  X INSPECT  Y LEADER";
  if(key=="pass")return pl?"SELECT - PASUJ (POTWIERDZ)":"SELECT - PASS (CONFIRM)";
  if(key=="round")return pl?"RUNDA":"ROUND";
  if(key=="round_complete")return pl?"RUNDA ZAKOŃCZONA":"ROUND COMPLETE";
  if(key=="passed")return pl?"SPASOWANO":"PASSED";
  if(key=="pass_button")return pl?"PASUJ":"PASS";
  if(key=="your_turn")return pl?"TWOJ RUCH":"YOUR TURN";
  if(key=="ai_turn")return pl?"RUCH PRZECIWNIKA":"OPPONENT TURN";
  if(key=="continue")return pl?"ENTER - DALEJ":"ENTER - CONTINUE";
  if(key=="defeat")return pl?"PRZEGRANA":"DEFEAT";
  if(key=="victory")return pl?"ZWYCIESTWO":"VICTORY";
  if(key=="target")return pl?"WYBIERZ KARTĘ":"CHOOSE A CARD";
  if(key=="resume")return pl?"R - WCZYTAJ ZAPIS   ESC - WSTECZ":"R - RESUME SAVE   ESC - BACK";
  if(key=="save_controls")return pl?"SELECT/S - ZAPISZ   START/O - WCZYTAJ":"SELECT/S - SAVE   START/O - LOAD";
  if(key=="deck_builder")return pl?"BUDOWNICZY Talii":"DECK BUILDER";
  if(key=="deck_mode")return pl?"KARTY":"CARDS";
  if(key=="leader_mode")return pl?"LIDER":"LEADER";
  if(key=="deck_count")return pl?"KARTY W TALII":"CARDS IN DECK";
  if(key=="deck_valid")return pl?"TALIA POPRAWNA":"DECK IS VALID";
  if(key=="deck_invalid")return pl?"TALIA NIEPOPRAWNA":"DECK IS INVALID";
  if(key=="deck_saved")return pl?"ZAPISANO TALIE":"DECK SAVED";
  if(key=="deck_save_error")return pl?"NIE UDAŁO SIĘ ZAPISAĆ TALII":"FAILED TO SAVE DECK";
  if(key=="deck_slot")return pl?"SLOT TALII":"DECK SLOT";
  if(key=="deck_controls")return pl?"GÓRA/DÓŁ: KARTA  A: DODAJ  B: USUŃ":"UP/DOWN: CARD  A: ADD  B: REMOVE";
  if(key=="deck_controls2")return pl?"L: LIDER  F: FRAKCJA  SELECT: ZAPISZ":"L: LEADER  F: FACTION  SELECT: SAVE";
  if(key=="deck_need_cards")return pl?"MIN. 22 JEDNOSTKI / MAX. 10 SPECJALNYCH":"MIN 22 UNITS / MAX 10 SPECIALS";
  if(key=="deck_custom")return pl?"WLASNA TALIA":"CUSTOM DECK";
  if(key=="card_details")return pl?"SZCZEGÓŁY KARTY":"CARD DETAILS";
  if(key=="leader_reveal")return pl?"KARTY Z REKI PRZECIWNIKA":"OPPONENT'S REVEALED CARDS";
  if(key=="leader_continue")return pl?"A / ENTER - KONTYNUUJ":"A / ENTER - CONTINUE";
  if(key=="leader_unavailable")return pl?"ZDOLNOSC LIDERA NIEDOSTEPNA":"LEADER ABILITY UNAVAILABLE";
  if(key=="leader_choose_target")return pl?"WYBIERZ KARTE":"CHOOSE A CARD";
  if(key=="leader_choose_discard")return pl?"WYBIERZ KARTE DO ODRZUCENIA":"CHOOSE A CARD TO DISCARD";
  if(key=="leader_choose_draw")return pl?"WYBIERZ KARTE DO DOBRANIA":"CHOOSE A CARD TO DRAW";
  if(key=="leader_choose_weather")return pl?"WYBIERZ KARTE POGODOWA":"CHOOSE A WEATHER CARD";
  if(key=="leader_choose_grave")return pl?"WYBIERZ JEDNOSTKE Z CMENTARZA":"CHOOSE A UNIT FROM THE GRAVEYARD";
  if(key=="leader_selected")return pl?"ODRZUCONE":"DISCARD SELECTION";
  if(key=="leader_controls")return pl?"LEWO/PRAWO: KARTA  A: WYBIERZ  B: WROC":"LEFT/RIGHT: CARD  A: SELECT  B: BACK";
  if(key=="leader_cancel")return pl?"B / ESC - ANULUJ":"B / ESC - CANCEL";
  if(key=="pause")return pl?"PAUZA / USTAWIENIA":"PAUSE / SETTINGS";
  if(key=="pause_resume")return pl?"WROC DO GRY":"RESUME GAME";
  if(key=="pause_save")return pl?"ZAPISZ MECZ":"SAVE MATCH";
  if(key=="pause_load")return pl?"WCZYTAJ MECZ":"LOAD MATCH";
  if(key=="pause_language")return pl?"JEZYK":"LANGUAGE";
  if(key=="pause_sfx")return pl?"EFEKTY DZWIEKOWE":"SOUND EFFECTS";
  if(key=="pause_music")return pl?"MUZYKA":"MUSIC";
  if(key=="pause_notifications")return pl?"POWIADOMIENIA":"NOTIFICATIONS";
  if(key=="pause_exit")return pl?"WRÓĆ DO MENU TALII":"RETURN TO DECK MENU";
  if(key=="pause_controls")return pl?"GÓRA/DÓŁ: WYBÓR  A: ZMIEŃ  B: WRÓĆ":"UP/DOWN: SELECT  A: CHANGE  B: BACK";
  if(key=="pass_confirm")return pl?"SPASOWAĆ TĘ RUNDĘ?":"PASS THIS ROUND?";
  if(key=="pass_confirm_controls")return pl?"A: POTWIERDZ  B: ANULUJ":"A: CONFIRM  B: CANCEL";
  if(key=="on")return "ON";
  if(key=="off")return "OFF";
  if(key=="power")return pl?"SIŁA":"POWER";
  if(key=="row")return pl?"RZĄD":"ROW";
  if(key=="special")return pl?"SPECJALNA":"SPECIAL";
  if(key=="siege")return pl?"OBLEZENIE":"SIEGE";
  if(key=="close_card")return pl?"B / ESC - ZAMKNIJ":"B / ESC - CLOSE";
  if(key=="no_ability")return pl?"BRAK ZDOLNOSCI SPECJALNEJ":"NO SPECIAL ABILITY";
  if(key=="saved")return pl?"ZAPISANO":"GAME SAVED";
  if(key=="loaded")return pl?"WCZYTANO ZAPIS":"SAVE LOADED";
  if(key=="no_save")return pl?"BRAK POPRAWNEGO ZAPISU":"NO VALID SAVE FOUND";
  if(key=="save_error")return pl?"BŁĄD ZAPISU MECZU":"MATCH SAVE ERROR";
  if(key=="reference_decks_missing")return pl?"BRAK TALI REFERENCYJNYCH":"REFERENCE DECKS MISSING";
  if(key=="reference_deck_invalid")return pl?"TALIA PRZECIWNIKA JEST NIEPOPRAWNA":"REFERENCE OPPONENT DECK INVALID";
  if(key=="leader_used")return pl?"UŻYTO ZDOLNOŚCI LIDERA":"LEADER ABILITY USED";
  if(key=="leader_cannot_use")return pl?"NIE MOŻNA UŻYĆ ZDOLNOŚCI LIDERA":"LEADER CANNOT BE USED";
  if(key=="leader_no_target")return pl?"BRAK POPRAWNEGO CELU":"LEADER HAS NO VALID TARGET";
  if(key=="no_card_selected")return pl?"NIE WYBRANO KARTY":"NO CARD SELECTED";
  if(key=="decoy_needs_unit")return pl?"MANEKIN WYMAGA JEDNOSTKI NA PLANSZY":"DECOY NEEDS A UNIT ON THE BOARD";
  if(key=="illegal_play")return pl?"NIEPRAWIDŁOWE ZAGRANIE":"ILLEGAL PLAY";
  if(key=="opponent_used_leader")return pl?"RYWAL UŻYŁ ZDOLNOŚCI LIDERA":"OPPONENT USED LEADER ABILITY";
  if(key=="ai_pass")return pl?"RUCH PRZECIWNIKA - PAS":"OPPONENT TURN - PASS";
  if(key=="you_passed")return pl?"PASOWAŁEŚ":"YOU PASSED";
  if(key=="skellige_revived")return pl?"SKELLIGE: WSKRZESZONO":"SKELLIGE REVIVED";
  if(key=="illegal_target")return pl?"NIEPRAWIDŁOWY CEL":"ILLEGAL TARGET";
  if(key=="mulligan_error")return pl?"NIE MOŻNA WYMIENIĆ TYCH KART":"MULLIGAN ERROR";
  if(key=="ai_action_error")return pl?"BŁĄD RUCHU PRZECIWNIKA - ZGŁOŚ TEN PRZYPADEK":"AI ACTION ERROR - REPORT THIS CASE";
  return key;
}
std::string displayCardName(const App& a,const gwent::Card& card) {
  if(card.referenceIndex>=0) {
    const auto& localized=gwent::referenceCardName(card.referenceIndex,a.language==Language::Polish);
    if(!localized.empty())return localized;
  }
  return card.name;
}
constexpr Uint32 notificationFadeMs=150;
constexpr Uint32 notificationHoldMs=900;
constexpr Uint32 notificationDurationMs=notificationFadeMs*2+notificationHoldMs;
constexpr Uint32 endScreenFadeMs=300;
Uint8 endScreenAlpha(Uint32 elapsed) {
  return static_cast<Uint8>(std::min<Uint32>(elapsed,endScreenFadeMs)*255/endScreenFadeMs);
}
Uint8 notificationAlpha(Uint32 elapsed) {
  if(elapsed<notificationFadeMs)return static_cast<Uint8>(elapsed*255/notificationFadeMs);
  if(elapsed<notificationFadeMs+notificationHoldMs)return 255;
  if(elapsed<notificationDurationMs)return static_cast<Uint8>((notificationDurationMs-elapsed)*255/notificationFadeMs);
  return 0;
}
void startNextNotification(App& a) {
  if(!a.notificationsEnabled||a.notificationQueue.empty()) { a.activeNotification.clear();return; }
  a.activeNotification=std::move(a.notificationQueue.front());
  a.notificationQueue.erase(a.notificationQueue.begin());
  a.notificationStartedAt=SDL_GetTicks();
}
void queueNotification(App& a,const std::string& key) {
  if(!a.notificationsEnabled)return;
  if(a.activeNotification.empty()) { a.activeNotification=key;a.notificationStartedAt=SDL_GetTicks(); }
  else a.notificationQueue.push_back(key);
}
void updateNotification(App& a,Uint32 now) {
  if(!a.notificationsEnabled) { a.activeNotification.clear();a.notificationQueue.clear();return; }
  if(!a.activeNotification.empty()&&SDL_TICKS_PASSED(now,a.notificationStartedAt+notificationDurationMs))
    startNextNotification(a);
}
void drawNotification(SDL_Renderer* r,const App& a) {
  if(a.activeNotification.empty())return;
  const Uint32 elapsed=SDL_GetTicks()-a.notificationStartedAt;
  const Uint8 alpha=notificationAlpha(elapsed);
  if(!alpha)return;
  std::string image,label;
  if(a.activeNotification=="me-coin") { image="notif_me_coin";label="notif_coin_me"; }
  else if(a.activeNotification=="op-coin") { image="notif_op_coin";label="notif_coin_op"; }
  else if(a.activeNotification=="round-start") { image="notif_round_start";label="notif_round_start"; }
  else if(a.activeNotification=="me-pass") { image="notif_round_passed";label="notif_me_pass"; }
  else if(a.activeNotification=="op-pass") { image="notif_round_passed";label="notif_op_pass"; }
  else if(a.activeNotification=="win-round") { image="notif_win_round";label="notif_win_round"; }
  else if(a.activeNotification=="lose-round") { image="notif_lose_round";label="notif_lose_round"; }
  else if(a.activeNotification=="draw-round") { image="notif_draw_round";label="notif_draw_round"; }
  else if(a.activeNotification=="me-turn") { image="notif_me_turn";label="notif_me_turn"; }
  else if(a.activeNotification=="op-turn") { image="notif_op_turn";label="notif_op_turn"; }
  else return;
  const auto trAlpha=[alpha](Uint8 value){return static_cast<Uint8>(static_cast<unsigned>(value)*alpha/255);};
  box(r,0,154,640,51,Color{10,10,10,trAlpha(242)});
  drawImageAlpha(r,a,"img/icons/"+image+".png",144,121,141,104,alpha);
  text(r,287,171,tr(a,label),Color{218,170,78,alpha},1);
}
void queueGameplayNotifications(App& a,gwent::Phase previousPhase,int previousRound,int previousPlayer,
                               const std::array<bool,2>& previousPassed,
                               const std::array<int,2>& previousRoundWins) {
  if(a.screen!=Screen::Match)return;
  const bool wasTurn=previousPhase==gwent::Phase::PlayerTurn||previousPhase==gwent::Phase::OpponentTurn;
  if(wasTurn)for(int player=0;player<2;++player)
    if(!previousPassed[player]&&a.game.players[player].passed)
      queueNotification(a,player==0?"me-pass":"op-pass");
  const bool enteredResult=(a.game.phase==gwent::Phase::RoundResult||a.game.phase==gwent::Phase::MatchResult)&&
                           previousPhase!=a.game.phase;
  if(enteredResult) {
    if(a.game.players[0].roundWins>previousRoundWins[0])queueNotification(a,"win-round");
    else if(a.game.players[1].roundWins>previousRoundWins[1])queueNotification(a,"lose-round");
    else queueNotification(a,"draw-round");
    return;
  }
  const bool roundStarted=a.game.round!=previousRound||
    (previousPhase==gwent::Phase::Mulligan&&
     (a.game.phase==gwent::Phase::PlayerTurn||a.game.phase==gwent::Phase::OpponentTurn));
  if(roundStarted)queueNotification(a,"round-start");
  const bool enteredTurn=a.game.phase==gwent::Phase::PlayerTurn||a.game.phase==gwent::Phase::OpponentTurn;
  if(enteredTurn&&(roundStarted||a.game.currentPlayer!=previousPlayer))
    queueNotification(a,a.game.currentPlayer==0?"me-turn":"op-turn");
}
bool loadSavedMatch(App& a) {
  gwent::Game restored;std::string error;
  if(a.savePath.empty()||!gwent::loadGame(a.savePath,restored,&error)) { a.status=tr(a,"no_save");return false; }
  a.game=std::move(restored);a.screen=Screen::Match;a.handIndex=0;a.rowIndex=0;a.targetIndex=0;
  a.endScreenStartedAt=SDL_GetTicks();
  a.targetMode=false;a.selectedTargets.clear();a.mulligan.clear();a.status=tr(a,"loaded");
  if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
  return true;
}
void saveCurrentMatch(App& a) {
  std::string error;
  if(a.screen!=Screen::Match||a.savePath.empty()||!gwent::saveGame(a.savePath,a.game,&error))a.status=tr(a,"save_error");
  else a.status=tr(a,"saved");
}
std::string factionName(gwent::Faction faction,bool pl) {
  switch(faction) {
    case gwent::Faction::NorthernRealms:return pl?"KROLESTWA POLNOCY":"NORTHERN REALMS";
    case gwent::Faction::Nilfgaard:return "NILFGAARD";
    case gwent::Faction::Scoiatael:return "SCOIA'TAEL";
    case gwent::Faction::Monsters:return pl?"POTWORY":"MONSTERS";
    case gwent::Faction::Skellige:return "SKELLIGE";
    default:return pl?"NEUTRALNA":"NEUTRAL";
  }
}
std::string userDeckPath(const App& a,int slot) {
  return a.dataDirectory+"deck-"+std::to_string(slot)+".gwd";
}
void saveSettings(const App& a) {
  if(a.dataDirectory.empty())return;
  std::ofstream settings(a.dataDirectory+"settings-v1.cfg",std::ios::trunc);
  if(settings)settings<<"GWENT_SETTINGS_V3 "<<static_cast<int>(a.language)<<' '
                     <<static_cast<int>(a.sfxEnabled)<<' '<<static_cast<int>(a.notificationsEnabled)<<' '
                     <<static_cast<int>(a.musicEnabled)<<'\n';
}
void loadSettings(App& a) {
  if(a.dataDirectory.empty())return;
  std::ifstream settings(a.dataDirectory+"settings-v1.cfg");
  std::string first;
  if(!(settings>>first))return;
  int language=0,sfx=1,notifications=1,music=1;
  if(first=="GWENT_SETTINGS_V3") {
    if(!(settings>>language>>sfx>>notifications>>music))return;
  } else if(first=="GWENT_SETTINGS_V2") {
    if(!(settings>>language>>sfx>>notifications))return;
  } else {
    try { language=std::stoi(first); } catch(...) { return; }
    int second=1,legacyThird=0;
    if(!(settings>>second))return;
    sfx=settings>>legacyThird?legacyThird:second;
  }
  if(language>=0&&language<=1)a.language=static_cast<Language>(language);
  a.sfxEnabled=sfx!=0;
  a.notificationsEnabled=notifications!=0;
  a.musicEnabled=music!=0;
}
void playCardSound(App& a,const gwent::Card& card,const gwent::Game& before,int player,gwent::Row targetRow) {
  const auto hasAbility=[&](const char* key) {
    return std::find(card.abilities.begin(),card.abilities.end(),key)!=card.abilities.end();
  };
  // Source Weather.addCard() plays Card.audio (the final ability tag) for
  // weather cards, while ordinary unit cards use Row.playPlacementAudio().
  if(card.kind==gwent::Kind::Weather||card.kind==gwent::Kind::ClearWeather) {
    if(!card.abilities.empty())a.scheduledSounds.push_back({card.abilities.back(),SDL_GetTicks()+SOURCE_CARD_PREVIEW_MS});
    return;
  }

  const int row=static_cast<int>(targetRow);
  const bool rowHasMardroeme=row>=0&&row<3&&before.players[player].mardroeme[static_cast<std::size_t>(row)];
  const int transformedCount=row>=0&&row<3?static_cast<int>(std::count_if(
    before.players[player].board[static_cast<std::size_t>(row)].begin(),
    before.players[player].board[static_cast<std::size_t>(row)].end(),
    [](const gwent::Card& unit){return unit.berserker||std::find(unit.abilities.begin(),unit.abilities.end(),"berserker")!=unit.abilities.end();})):0;
  const bool berserker=card.berserker||hasAbility("berserker");
  const Uint32 arrivalAt=SDL_GetTicks()+SOURCE_CARD_PREVIEW_MS+SOURCE_CARD_FLIGHT_MS;
  if(card.kind==gwent::Kind::Mardroeme) {
    // Mardroeme occupies the row's special slot and has no placement sound;
    // each transformed Vildkaarl plays the Mardroeme SFX in its source handler.
    for(int i=0;i<transformedCount;++i)a.scheduledSounds.push_back({"mardroeme",SDL_GetTicks()+SOURCE_CARD_PREVIEW_MS});
    return;
  }
  if(card.kind==gwent::Kind::Decoy) { a.scheduledSounds.push_back({"decoy",arrivalAt});return; }
  if(card.kind==gwent::Kind::Scorch||card.kind==gwent::Kind::CommanderHorn) return;
  if(card.spy||hasAbility("spy")||hasAbility("vildkarrl")||(berserker&&rowHasMardroeme)) {
    if(berserker&&rowHasMardroeme)a.scheduledSounds.push_back({"mardroeme",arrivalAt});
    return;
  }
  const char* placement=card.hero?"hero":targetRow==gwent::Row::Ranged?"common_ranged":
    targetRow==gwent::Row::Siege?"common_siege":"common_close";
  a.scheduledSounds.push_back({placement,arrivalAt});

  // A unit card carrying Mardroeme (Ermion) still gets its ordinary row
  // placement sound; its placed callback then transforms existing Berserkers.
  if(hasAbility("mardroeme"))for(int i=0;i<transformedCount;++i)
    a.scheduledSounds.push_back({"mardroeme",arrivalAt+SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS});
}
std::vector<const gwent::Card*> editorCards(const App& a) {
  std::vector<const gwent::Card*> cards;std::vector<int> seen;
  for(const auto& card:gwent::referenceCardCatalog()) {
    if(card.kind==gwent::Kind::Leader||card.deckCopies<=0||
       (card.faction!=gwent::Faction::Neutral&&card.faction!=a.editDeck.faction)||
       std::find(seen.begin(),seen.end(),card.referenceIndex)!=seen.end())continue;
    seen.push_back(card.referenceIndex);cards.push_back(&card);
  }
  const auto factionRank=[](gwent::Kind kind) {
    if(kind==gwent::Kind::Weather||kind==gwent::Kind::ClearWeather)return -1;
    if(kind==gwent::Kind::Decoy||kind==gwent::Kind::Scorch||kind==gwent::Kind::CommanderHorn||kind==gwent::Kind::Mardroeme)return -2;
    return 0;
  };
  std::stable_sort(cards.begin(),cards.end(),[&](const auto* left,const auto* right) {
    const int leftRank=factionRank(left->kind),rightRank=factionRank(right->kind);
    if(leftRank!=rightRank)return leftRank<rightRank;
    if(left->power!=right->power)return left->power>right->power;
    return left->name<right->name;
  });
  return cards;
}
int deckCopies(const gwent::UserDeck& deck,int referenceIndex) {
  const auto it=std::find_if(deck.cardCounts.begin(),deck.cardCounts.end(),[&](const auto& entry){return entry.first==referenceIndex;});
  return it==deck.cardCounts.end()?0:it->second;
}
void openDeckEditor(App& a) {
  a.editingLeader=false;a.editorCardIndex=0;
  std::string error;
  if(!gwent::loadUserDeck(userDeckPath(a,a.deckSlot),a.editDeck,&error)) {
    const auto& recipe=recipes()[a.deckIndex%recipes().size()];
    a.editDeck={"Deck "+std::to_string(a.deckSlot),recipe.faction,recipe.leaderReferenceIndex,recipe.cardCounts};
  }
  a.screen=Screen::DeckBuilder;
}
void changeDeckFaction(App& a) {
  const int next=(static_cast<int>(a.editDeck.faction)+1)%5;
  a.editDeck.faction=static_cast<gwent::Faction>(next);
  const auto& catalog=gwent::referenceCardCatalog();
  a.editDeck.cardCounts.erase(std::remove_if(a.editDeck.cardCounts.begin(),a.editDeck.cardCounts.end(),[&](const auto& entry){
    const auto it=std::find_if(catalog.begin(),catalog.end(),[&](const auto& card){return card.referenceIndex==entry.first&&card.kind!=gwent::Kind::Leader;});
    return it==catalog.end()||(it->faction!=gwent::Faction::Neutral&&it->faction!=a.editDeck.faction);
  }),a.editDeck.cardCounts.end());
  const auto leader=std::find_if(catalog.begin(),catalog.end(),[&](const auto& card){return card.kind==gwent::Kind::Leader&&card.faction==a.editDeck.faction;});
  if(leader!=catalog.end())a.editDeck.leaderReferenceIndex=leader->referenceIndex;
  a.editorCardIndex=0;
}
gwent::Card recipeLeader(std::size_t index) {
  gwent::Card leader;
  if(!recipes().empty()) gwent::referenceLeader(recipes()[index%recipes().size()].leaderReferenceIndex,leader);
  return leader;
}
void startMatch(App& a) {
  if(recipes().size()<2) { a.status=tr(a,"reference_decks_missing"); return; }
  gwent::Card humanLeader=recipeLeader(a.deckIndex);std::vector<gwent::Card> humanDeck;
  if(a.userDeckSelected) {
    if(!gwent::materializeUserDeck(a.editDeck,humanDeck,humanLeader)) { a.status=tr(a,"deck_invalid");return; }
  } else humanDeck=gwent::makeReferenceDeck(recipes()[a.deckIndex]);
  const auto seed=static_cast<std::uint64_t>(SDL_GetTicks())^SDL_GetPerformanceCounter();
  auto opponent=gwent::makeReferenceOpponentDeck(seed);
  if(opponent.sourceDeckIndex<0||opponent.cards.empty()||opponent.leader.kind!=gwent::Kind::Leader) {
    a.status=tr(a,"reference_deck_invalid");return;
  }
  gwent::beginMatchWithLeaders(a.game,std::move(humanDeck),humanLeader,
      std::move(opponent.cards),opponent.leader,seed,-1);
  a.mulligan.clear(); a.handIndex=0; a.rowIndex=0; a.screen=Screen::Match;a.endScreenStartedAt=0;
  queueNotification(a,a.game.firstPlayer==0?"me-coin":"op-coin");
  a.audio.play("game_opening");
  a.status=tr(a,"mulligan");
}
std::vector<std::string> legalCardTargets(const App& a,const gwent::Card& card) {
  std::vector<std::string> ids;
  const auto& p=a.game.players[0];
  if(card.kind==gwent::Kind::Decoy) {
    for(const auto& row:p.board) for(const auto& target:row)
      if(target.kind==gwent::Kind::Unit && !target.hero && (target.row!=gwent::Row::None||target.agile)) ids.push_back(target.id);
  } else if(card.kind==gwent::Kind::Unit && card.medic) {
    for(const auto& target:p.graveyard) if(target.kind==gwent::Kind::Unit && !target.hero && (target.row!=gwent::Row::None||target.agile)) ids.push_back(target.id);
    const auto graveCard=[&](const std::string& id)->const gwent::Card* {
      const auto it=std::find_if(p.graveyard.begin(),p.graveyard.end(),[&](const gwent::Card& item){return item.id==id;});
      return it==p.graveyard.end()?nullptr:&*it;
    };
    const auto sourceBasePower=[](const gwent::Card& card) {
      const auto& catalog=gwent::referenceCardCatalog();
      const auto original=std::find_if(catalog.begin(),catalog.end(),[&](const gwent::Card& item){return item.referenceIndex==card.referenceIndex;});
      return original==catalog.end()?card.power:original->power;
    };
    std::stable_sort(ids.begin(),ids.end(),[&](const std::string& left,const std::string& right) {
      const auto* aCard=graveCard(left);const auto* bCard=graveCard(right);
      if(!aCard||!bCard)return left<right;
      const int aBasePower=sourceBasePower(*aCard),bBasePower=sourceBasePower(*bCard);
      if(aBasePower!=bBasePower)return aBasePower<bBasePower;
      return aCard->name<bCard->name;
    });
  }
  return ids;
}
std::string activeLeaderAbility(const gwent::Card& leader) {
  const auto it=std::find_if(leader.abilities.begin(),leader.abilities.end(),[](const std::string& id){
    return id.rfind("foltest_",0)==0||id.rfind("emhyr_",0)==0||id.rfind("eredin_",0)==0||
           id.rfind("francesca_",0)==0||id=="crach_an_craite"||id=="king_bran";
  });
  return it==leader.abilities.end()?std::string{}:*it;
}
std::vector<gwent::Card> leaderChoiceCards(const App& a) {
  const auto& player=a.game.players[0];
  const std::string ability=activeLeaderAbility(player.leader);
  std::vector<gwent::Card> cards;
  if(ability=="emhyr_relentless"||ability=="eredin_bringer_of_death") {
    const auto& grave=ability=="emhyr_relentless"?a.game.players[1].graveyard:player.graveyard;
    for(const auto& card:grave)if(card.kind==gwent::Kind::Unit&&!card.hero&&(card.row!=gwent::Row::None||card.agile))cards.push_back(card);
  } else if(ability=="eredin_destroyer"&&a.leaderSelections.size()<2) {
    for(const auto& card:player.hand)
      if(std::find(a.leaderSelections.begin(),a.leaderSelections.end(),card.id)==a.leaderSelections.end())cards.push_back(card);
  } else if(ability=="eredin_destroyer") {
    for(const auto& card:player.deck)cards.push_back(card);
  } else if(ability=="eredin_king") {
    for(const auto& card:player.deck)if(card.kind==gwent::Kind::Weather)cards.push_back(card);
  }
  return cards;
}
std::string leaderChoicePrompt(const App& a) {
  const std::string ability=activeLeaderAbility(a.game.players[0].leader);
  if(ability=="eredin_destroyer")return tr(a,a.leaderSelections.size()<2?"leader_choose_discard":"leader_choose_draw");
  if(ability=="eredin_king")return tr(a,"leader_choose_weather");
  return tr(a,"leader_choose_grave");
}
void finishLeaderChoice(App& a) {
  const auto before=a.game;
  const std::string ability=activeLeaderAbility(a.game.players[0].leader);
  const bool okay=gwent::activateLeader(a.game,0,gwent::Row::None,a.leaderSelections);
  a.leaderSelectionMode=false;
  a.targetIndex=0;
  if(okay) {
    queueLeaderTransitionVisuals(a,before,ability,0);
    lockInputUntilQueuedAnimationsFinish(a,SDL_GetTicks());
    a.status=tr(a,"leader_used");
    if(!a.game.revealedCards[0].empty())a.leaderRevealOpen=true;
    if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
  } else a.status=tr(a,"leader_cannot_use");
  a.leaderSelections.clear();
}
void beginLeaderChoice(App& a) {
  a.leaderSelections.clear();
  if(!a.game.players[0].leaderAbilityAvailable||a.game.players[0].leaderAbilityCancelled) {
    a.status=tr(a,"leader_unavailable");return;
  }
  const auto ability=activeLeaderAbility(a.game.players[0].leader);
  const bool needsChoice=ability=="emhyr_relentless"||ability=="eredin_bringer_of_death"||
                         ability=="eredin_destroyer"||ability=="eredin_king";
  if(!needsChoice) {
    a.audio.play("open");
    const auto before=a.game;
    if(gwent::activateLeader(a.game,0)) {
      queueLeaderTransitionVisuals(a,before,ability,0);
      lockInputUntilQueuedAnimationsFinish(a,SDL_GetTicks());
      a.status=tr(a,"leader_used");
      if(!a.game.revealedCards[0].empty())a.leaderRevealOpen=true;
      if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
    } else a.status=tr(a,"leader_cannot_use");
    return;
  }
  if((ability=="eredin_destroyer"&&(a.game.players[0].hand.size()<2||a.game.players[0].deck.empty()))||
     leaderChoiceCards(a).empty()) { a.status=tr(a,"leader_no_target");return; }
  a.leaderSelectionMode=true;
  a.targetIndex=0;
  a.audio.play("open");
  a.status=leaderChoicePrompt(a);
}
gwent::Card findById(const std::vector<gwent::Card>& cards,const std::string& id) {
  const auto it=std::find_if(cards.begin(),cards.end(),[&](const gwent::Card& c){return c.id==id;});
  return it==cards.end()?gwent::Card{}:*it;
}
const gwent::Card* findTargetCard(const gwent::Game& game,const std::string& id) {
  for(const auto& player:game.players) {
    for(const auto& row:player.board)for(const auto& card:row)if(card.id==id)return &card;
    for(const auto& card:player.graveyard)if(card.id==id)return &card;
    for(const auto& card:player.hand)if(card.id==id)return &card;
  }
  return nullptr;
}
void doHumanPlay(App& a) {
  auto& hand=a.game.players[0].hand;
  if(a.handIndex<0 || a.handIndex>=static_cast<int>(hand.size())) { a.status=tr(a,"no_card_selected"); return; }
  const auto card=hand[a.handIndex];
  const auto targets=legalCardTargets(a,card);
  if(card.kind==gwent::Kind::Decoy && targets.empty()) { a.status=tr(a,"decoy_needs_unit"); return; }
  if(!targets.empty()) {
    a.targetMode=true; a.selectedTargets=targets; a.targetIndex=0; a.reviveRowIndex=0; a.audio.play("open");a.status=tr(a,"target"); return;
  }
  const int handSizeBefore=static_cast<int>(hand.size());
  const auto before=a.game;
  if(gwent::playCard(a.game,0,card.id,static_cast<gwent::Row>(a.rowIndex))) {
    animatePlayedCard(a,card,0,a.handIndex,handSizeBefore);
    queueTriggeredEffects(a,before,card,0);
    playCardSound(a,card,before,0,static_cast<gwent::Row>(a.rowIndex));
    lockCardActionUntilAnimationsFinish(a,card,before,0,static_cast<gwent::Row>(a.rowIndex));
    a.status=displayCardName(a,card)+(a.language==Language::Polish?" - ZAGRANO":" - PLAYED"); a.handIndex=std::min(a.handIndex,static_cast<int>(a.game.players[0].hand.size())-1);
  } else a.status=tr(a,"illegal_play");
}
void performAi(App& a) {
  if(a.game.phase!=gwent::Phase::OpponentTurn) return;
  const auto action=gwent::chooseAiAction(a.game,1);
  bool success=false;
  if(action.pass) { success=gwent::pass(a.game,1);a.audio.play("pass");a.status=tr(a,"ai_pass"); }
  else if(action.activateLeader) {
    const auto before=a.game;
    const auto ability=activeLeaderAbility(a.game.players[1].leader);
    success=gwent::activateLeader(a.game,1,action.target,action.leaderSelection);
    if(success) { queueLeaderTransitionVisuals(a,before,ability,1);lockInputUntilQueuedAnimationsFinish(a,SDL_GetTicks()); }
    a.status=tr(a,"opponent_used_leader");
  } else {
    const auto& aiHand=a.game.players[1].hand;
    const auto it=std::find_if(aiHand.begin(),aiHand.end(),[&](const gwent::Card& item){return item.id==action.cardId;});
    const int cardIndex=static_cast<int>(it-aiHand.begin());
    const int handSizeBefore=static_cast<int>(aiHand.size());
    const auto card=it==aiHand.end()?gwent::Card{}:*it;
    const auto before=a.game;
    success=gwent::playCard(a.game,1,action.cardId,action.target,action.selectedCardId);
    if(success) {
      animatePlayedCard(a,card,1,cardIndex,handSizeBefore);
      queueTriggeredEffects(a,before,card,1);
      playCardSound(a,card,before,1,action.target);
      lockCardActionUntilAnimationsFinish(a,card,before,1,action.target);
    }
    a.status=(a.language==Language::Polish?"RYWAL ZAGRYWA ":"OPPONENT PLAYED ")+displayCardName(a,card);
  }
  if(!success) {
    // Do not disguise an invalid ported AI action as a voluntary source-game
    // pass. Keep the turn state intact and make the failed action visible so
    // the mismatched rule can be corrected instead of silently changing play.
    a.status=tr(a,"ai_action_error");
  }
  a.aiAt=SDL_GetTicks()+360;
}
int rowScore(const gwent::Game& game,int owner,int row) {
  gwent::Player p=game.players[owner];
  for(int i=0;i<3;++i) if(i!=row) p.board[i].clear();
  return gwent::calculateScore(p,game.weather,game.horn[owner]);
}
void drawCardTile(SDL_Renderer* r,const App& app,int x,int y,int w,int h,const gwent::Card& card,bool selected,bool dim=false) {
  if(w>=45&&h>=60)drawImage(r,app,"img/sm/"+gwent::referenceCardImage(card.referenceIndex),x,y,w,h);
  const Color base=card.hero?Color{85,96,107}:card.kind==gwent::Kind::Unit?Color{53,69,63}:Color{78,63,43};
  if(w<45||h<60)box(r,x,y,w,h,selected?Color{101,79,44}:base);
  frame(r,x,y,w,h,selected?paper:gold);
  if(dim) box(r,x+2,y+2,w-4,h-4,Color{20,24,24,130});
  const std::string name=shortName(displayCardName(app,card),std::max(3,(w-6)/6));
  if(w<45||h<60) {
    text(r,x+3,y+5,name,card.hero?paper:paper,1);
    if(card.kind==gwent::Kind::Unit) text(r,x+3,y+h-12,std::to_string(card.power)+(card.hero?" H":""),gold,1);
    else text(r,x+3,y+h-12,card.kind==gwent::Kind::Weather?"WEA":"SPC",gold,1);
  }
  if(card.spy) text(r,x+w-10,y+4,"S",blue,1);
  if(card.medic) text(r,x+w-10,y+4,"M",green,1);
}
void drawSourceCard(SDL_Renderer* r,const App& a,int x,int y,const gwent::Card& card,bool selected=false) {
  constexpr int w=28,h=43;
  drawImage(r,a,"img/sm/"+gwent::referenceCardImage(card.referenceIndex),x,y,w,h);
  std::string powerIcon="normal";
  if(card.hero)powerIcon="hero";
  else if(!card.abilities.empty()&&(card.kind==gwent::Kind::Weather||card.kind==gwent::Kind::ClearWeather||card.kind==gwent::Kind::Scorch||card.kind==gwent::Kind::CommanderHorn||card.kind==gwent::Kind::Mardroeme||card.kind==gwent::Kind::Decoy))powerIcon=card.abilities.front();
  if(powerIcon.rfind("scorch",0)==0)powerIcon="scorch";
  drawImage(r,a,"img/icons/power_"+powerIcon+".png",x-1,y-2,24,25);
  if(card.kind==gwent::Kind::Unit||card.kind==gwent::Kind::Leader) {
    const std::string power=std::to_string(card.power);
    if(card.hero) {
      const int scale=power.size()>1?1:2;
      centeredTextInBox(r,x-1,y-2,24,25,power,paper,scale);
    } else {
      // The badge's inner disc is 12x12 px at the 640x480 render size.
      // Use the regular bitmap glyphs so two-digit strengths remain legible.
      const int numberWidth=static_cast<int>(power.size())*6-1;
      const int numberX=x+6-numberWidth/2;
      text(r,numberX+1,y+4,power,Color{190,145,70,255},1);
      text(r,numberX,y+3,power,ink,1);
    }
  }
  if(card.row!=gwent::Row::None) {
    const char* rowIcon=card.agile?"agile":card.row==gwent::Row::Ranged?"ranged":card.row==gwent::Row::Siege?"siege":"close";
    drawImage(r,a,"img/icons/card_row_"+std::string(rowIcon)+".png",x+18,y+32,9,9);
  }
  std::string ability=card.abilities.empty()?std::string{}:card.abilities.back();
  if(ability=="cerys")ability="muster";
  if(ability.rfind("avenger",0)==0)ability="avenger";
  if(ability.rfind("scorch",0)==0)ability="scorch";
  if(card.abilities.empty()&&card.agile)ability="agile";
  if(card.kind==gwent::Kind::Weather||card.kind==gwent::Kind::ClearWeather||card.kind==gwent::Kind::Decoy||card.kind==gwent::Kind::Scorch||card.kind==gwent::Kind::CommanderHorn||card.kind==gwent::Kind::Mardroeme)ability.clear();
  if(!ability.empty())drawImage(r,a,"img/icons/card_ability_"+ability+".png",x+2,y+32,9,9);
  if(selected)frame(r,x-2,y-2,w+4,h+4,gold);
}
void animatePlayedCard(App& a,const gwent::Card& card,int player,int handIndex,int handSizeBefore) {
  App::CardFlight flight;
  flight.card=card;
  if(player==0) {
    const int first=std::clamp(handIndex-5,0,std::max(0,handSizeBefore-11));
    flight.fromX=194+(handIndex-first)*29;flight.fromY=277;
  } else { flight.fromX=292;flight.fromY=20; }
  bool found=false;
  for(int owner=0;owner<2&&!found;++owner)for(int row=0;row<3&&!found;++row) {
    const auto& cards=a.game.players[owner].board[row];
    const auto it=std::find_if(cards.begin(),cards.end(),[&](const gwent::Card& item){return item.id==card.id;});
    if(it==cards.end())continue;
    const int index=static_cast<int>(it-cards.begin());
    const auto geometry=sourceRowGeometry(cards.size());
    flight.owner=owner;flight.row=row;flight.toX=static_cast<int>(std::lround(geometry.firstX+index*geometry.pitch));
    flight.toY=owner==0?143+row*45:5+(2-row)*45;found=true;
  }
  if(!found) {
    const auto it=std::find_if(a.game.weatherCards.begin(),a.game.weatherCards.end(),[&](const gwent::ActiveWeather& item){return item.card.id==card.id;});
    if(it!=a.game.weatherCards.end()) {
      const int index=static_cast<int>(it-a.game.weatherCards.begin());
      flight.owner=it->owner;flight.row=-1;flight.toX=47+index*29;flight.toY=149;found=true;
    } else if(card.kind==gwent::Kind::CommanderHorn) {
      // Source places Commander's Horn in its row's special slot. In this
      // renderer that slot is represented by the horn marker at row's left.
      int targetRow=player==0?a.rowIndex:0;
      if(player==1)for(int row=0;row<3;++row)if(a.game.horn[player][row]){targetRow=row;break;}
      flight.owner=player;flight.row=targetRow;flight.toX=190;flight.toY=player==0?143+targetRow*45:5+(2-targetRow)*45;found=true;
    }
  }
  if(found) {
    flight.destination=flight.row==-1?App::CardZone::Weather:App::CardZone::Board;
    flight.active=true;flight.startedAt=SDL_GetTicks()+SOURCE_CARD_PREVIEW_MS;a.cardFlight=std::move(flight);
    // Source Card.animate() attaches the matching anim_<ability> artwork to
    // the card: 50 ms setup, 300 ms fade in, 1000 ms hold, 300 ms fade out.
    // Scorch has its own 50 + 1300 + 300 ms sequence and cover sizing.
    std::vector<std::string> effects;
    for(const auto* ability:{"horn","spy","morale"})
      if(std::find(card.abilities.begin(),card.abilities.end(),ability)!=card.abilities.end())
        effects.emplace_back(ability);
    if(card.kind==gwent::Kind::CommanderHorn&&std::find(effects.begin(),effects.end(),"horn")==effects.end())effects.emplace_back("horn");
    if(card.spy&&std::find(effects.begin(),effects.end(),"spy")==effects.end())effects.emplace_back("spy");
    const bool rowAudioWait=sourceWaitsForRowPlacementAudio(card,a.game,player,card.row);
    Uint32 effectStart=flight.startedAt+SOURCE_CARD_FLIGHT_MS+
      (rowAudioWait?SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS:0);
    for(const auto& effect:effects) {
      App::CardEffect visual;visual.active=true;visual.icon=effect;visual.x=flight.toX;visual.y=flight.toY;
      visual.startedAt=effectStart;visual.duration=1650;a.cardEffects.push_back(std::move(visual));
      a.scheduledSounds.push_back({effect,effectStart});
      effectStart+=1650;
    }
  }
}
void queueTargetEffect(App& a,const std::string& icon,int x,int y,Uint32 delay=0) {
  App::CardEffect visual;visual.active=true;visual.icon=icon;visual.x=x;visual.y=y;
  visual.startedAt=SDL_GetTicks()+delay;visual.duration=1650;
  a.cardEffects.push_back(std::move(visual));
  a.scheduledSounds.push_back({icon,SDL_GetTicks()+delay});
}
void queueTriggeredEffects(App& a,const gwent::Game& before,const gwent::Card& played,int player) {
  queueCardTransitionFlights(a,before,played,player);
  const Uint32 now=SDL_GetTicks();
  const Uint32 previewEnd=now+SOURCE_CARD_PREVIEW_MS;
  const Uint32 cardArrival=previewEnd+SOURCE_CARD_FLIGHT_MS;
  const Uint32 placedEffectsStart=cardArrival+
    (sourceWaitsForRowPlacementAudio(played,before,player,played.row)?SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS:0);
  const bool scorchAction=played.kind==gwent::Kind::Scorch||
    std::find(played.abilities.begin(),played.abilities.end(),"scorch_c")!=played.abilities.end()||
    std::find(played.abilities.begin(),played.abilities.end(),"scorch_r")!=played.abilities.end()||
    std::find(played.abilities.begin(),played.abilities.end(),"scorch_s")!=played.abilities.end();
  const auto boardPosition=[](int owner,int row,int index,std::size_t cardCount) {
    const auto geometry=sourceRowGeometry(cardCount);
    return std::pair<int,int>{static_cast<int>(std::lround(geometry.firstX+index*geometry.pitch)),
                              owner==0?143+row*45:5+(2-row)*45};
  };
  const auto containsId=[](const std::vector<gwent::Card>& cards,const std::string& id) {
    return std::any_of(cards.begin(),cards.end(),[&](const gwent::Card& c){return c.id==id;});
  };
  // Scorch resolves its targets simultaneously in the reference game. Keep
  // the old row coordinates because the engine has already moved them to the grave.
  if(scorchAction) {
    for(int owner=0;owner<2;++owner)for(int row=0;row<3;++row) {
      const auto& oldRow=before.players[owner].board[row];
      for(int index=0;index<static_cast<int>(oldRow.size());++index) {
        const auto& target=oldRow[index];
        if(containsId(a.game.players[owner].graveyard,target.id)) {
          const auto [x,y]=boardPosition(owner,row,index,oldRow.size());
          // Source Scorch animates the affected unit cards directly; the
          // Scorch special itself has no Card.animate() call.
          queueTargetEffect(a,"scorch",x,y,SOURCE_CARD_PREVIEW_MS);
        }
      }
    }
  }
  for(int owner=0;owner<2;++owner)for(int row=0;row<3;++row) {
    const auto& current=a.game.players[owner].board[row];
    const int bondCount=static_cast<int>(std::count_if(current.begin(),current.end(),[&](const gwent::Card& c){return c.name==played.name;}));
    for(int index=0;index<static_cast<int>(current.size());++index) {
      const auto& target=current[index];
      const bool existed=std::any_of(before.players[0].board.begin(),before.players[0].board.end(),[&](const auto& r){return containsId(r,target.id);})||
                         std::any_of(before.players[1].board.begin(),before.players[1].board.end(),[&](const auto& r){return containsId(r,target.id);});
      const auto [x,y]=boardPosition(owner,row,index,current.size());
      if(played.medic&&!existed&&containsId(before.players[player].graveyard,target.id))
        queueTargetEffect(a,"medic",x,y,placedEffectsStart-now);
      if(played.bond&&owner==player&&containsId(current,played.id)&&target.id!=played.id&&bondCount>1&&target.name==played.name)
        queueTargetEffect(a,"bond",x,y,placedEffectsStart-now);
      if(target.id==played.id&&owner==player) {
        if(played.bond&&bondCount>1)queueTargetEffect(a,"bond",x,y,placedEffectsStart-now);
        const bool revived=played.medic&&std::any_of(current.begin(),current.end(),[&](const gwent::Card& item) {
          return !std::any_of(before.players[0].board.begin(),before.players[0].board.end(),[&](const auto& oldRow){return containsId(oldRow,item.id);})&&
                 !std::any_of(before.players[1].board.begin(),before.players[1].board.end(),[&](const auto& oldRow){return containsId(oldRow,item.id);})&&
                 containsId(before.players[player].graveyard,item.id);
        });
        if(revived)queueTargetEffect(a,"medic",x,y,placedEffectsStart-now);
        const auto isMusterGroup=[&](const gwent::Card& item) {
          return !played.group.empty()&&(item.group==played.group||item.name==played.group||
                 item.name.rfind(played.group+" - ",0)==0);
        };
        const bool mustered=played.muster&&std::any_of(current.begin(),current.end(),[&](const gwent::Card& item) {
          if(item.id==played.id||!isMusterGroup(item))return false;
          const bool previouslyOnBoard=std::any_of(before.players[0].board.begin(),before.players[0].board.end(),[&](const auto& oldRow){return containsId(oldRow,item.id);})||
                                       std::any_of(before.players[1].board.begin(),before.players[1].board.end(),[&](const auto& oldRow){return containsId(oldRow,item.id);});
          return !previouslyOnBoard;
        });
        if(mustered)queueTargetEffect(a,"muster",x,y,placedEffectsStart-now);
      }
    }
  }
}
void queueLeaderTransitionVisuals(App& a,const gwent::Game& before,const std::string& ability,int player) {
  gwent::Card action;action.id="leader-action";
  if(ability.rfind("foltest_steelforged",0)==0||ability=="foltest_son"||ability=="francesca_queen")
    action.abilities={ability=="foltest_steelforged"?"scorch_s":ability=="foltest_son"?"scorch_r":"scorch_c"};
  queueCardTransitionFlights(a,before,action,player);
  if(!action.abilities.empty()) {
    const int row=action.abilities.front()=="scorch_s"?2:action.abilities.front()=="scorch_r"?1:0;
    const int owner=1-player;
    const auto& oldRow=before.players[owner].board[static_cast<std::size_t>(row)];
    const auto& grave=a.game.players[owner].graveyard;
    for(int i=0;i<static_cast<int>(oldRow.size());++i) {
      const auto& card=oldRow[static_cast<std::size_t>(i)];
      if(std::any_of(grave.begin(),grave.end(),[&](const gwent::Card& item){return item.id==card.id;})) {
        const auto geometry=sourceRowGeometry(oldRow.size());
        queueTargetEffect(a,"scorch",static_cast<int>(std::lround(geometry.firstX+i*geometry.pitch)),
                          owner==0?143+row*45:5+(2-row)*45);
      }
    }
  }
  if(ability=="foltest_siegemaster"||ability=="eredin_commander"||ability=="francesca_beautiful") {
    const int row=ability=="foltest_siegemaster"?2:ability=="eredin_commander"?0:1;
    App::CardEffect effect;effect.active=true;effect.icon="horn";effect.x=190;
    effect.y=player==0?143+row*45:5+(2-row)*45;effect.startedAt=SDL_GetTicks();effect.duration=1650;
    a.cardEffects.push_back(std::move(effect));
  }
  for(const auto& active:a.game.weatherCards) {
    const bool existed=std::any_of(before.weatherCards.begin(),before.weatherCards.end(),[&](const gwent::ActiveWeather& old){return old.card.id==active.card.id;});
    if(!existed&&!active.card.abilities.empty())a.audio.play(active.card.abilities.back());
  }
  if(ability=="crach_an_craite")a.audio.play("redraw");
  else if(ability=="eredin_destroyer") { a.audio.play("discard");a.audio.play("discard");a.audio.play("draw"); }
}
void drawCardFlight(SDL_Renderer* r,const App& a,const App::CardFlight& flight) {
  if(!flight.active)return;
  if(!SDL_TICKS_PASSED(SDL_GetTicks(),flight.startedAt)) {
    drawSourceCard(r,a,flight.fromX,flight.fromY,flight.card,true);
    return;
  }
  const float t=std::clamp(static_cast<float>(SDL_GetTicks()-flight.startedAt)/499.0f,0.0f,1.0f);
  const float eased=static_cast<float>(sourceEase(t));
  const int x=static_cast<int>(std::lround(flight.fromX+(flight.toX-flight.fromX)*eased));
  const int y=static_cast<int>(std::lround(flight.fromY+(flight.toY-flight.fromY)*eased));
  drawSourceCard(r,a,x,y,flight.card,true);
}
void drawCardEffect(SDL_Renderer* r,const App& a) {
  const Uint32 now=SDL_GetTicks();
  for(const auto& effect:a.cardEffects) {
    if(!effect.active||!SDL_TICKS_PASSED(now,effect.startedAt))continue;
    if(SDL_TICKS_PASSED(now,effect.startedAt+effect.duration))continue;
    const Uint32 elapsed=now-effect.startedAt;
    Uint8 alpha=255;
    if(elapsed<50)alpha=0;
    else if(elapsed<350)alpha=static_cast<Uint8>(std::clamp((elapsed-50)*255/300,0u,255u));
    else if(elapsed>=effect.duration-300)alpha=static_cast<Uint8>(std::clamp((effect.duration-elapsed)*255/300,0u,255u));
    const int scale=effect.icon=="scorch"?100:sourceAbilityScalePercent(elapsed);
    const int width=std::max(1,static_cast<int>(std::lround(28*scale/100.0)));
    const int height=std::max(1,static_cast<int>(std::lround(43*scale/100.0)));
    drawImageAlpha(r,a,"img/icons/anim_"+effect.icon+".png",effect.x+(28-width)/2,effect.y+(43-height)/2,width,height,alpha);
  }
}
std::vector<std::string> wrapDetailText(const std::string& value,std::size_t limit);
void drawCardPreview(SDL_Renderer* r,const App& a,const gwent::Card& card) {
  constexpr int x=489,y=15,width=127;
  const int portraitHeight=cardPortraitHeight(width);
  drawCardPortrait(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),x,y,width,portraitHeight);
  box(r,x,y+portraitHeight,width,241-portraitHeight,Color{8,8,8,245});
  frame(r,x,y,width,241,gold);
  const auto nameLines=wrapDetailText(displayCardName(a,card),20);
  for(std::size_t i=0;i<nameLines.size()&&i<2;++i)
    centeredTextInBox(r,x+3,y+portraitHeight+3+static_cast<int>(i)*8,width-6,8,nameLines[i],paper,1);
  const int statY=y+portraitHeight+22;
  centeredTextInBox(r,x+3,statY,width-6,8,tr(a,"power")+" "+std::to_string(card.power)+"  "+tr(a,"row")+" "+
    (card.row==gwent::Row::Melee?tr(a,"melee"):card.row==gwent::Row::Ranged?tr(a,"ranged"):card.row==gwent::Row::Siege?tr(a,"siege"):tr(a,"special")),gold,1);
}
std::vector<std::string> wrapDetailText(const std::string& value,std::size_t limit) {
  std::vector<std::string> lines;
  std::string line,word;
  const auto flush=[&]() {
    if(word.empty())return;
    if(!line.empty()&&line.size()+word.size()+1>limit) { lines.push_back(line);line.clear(); }
    if(!line.empty())line+=' ';
    line+=word;word.clear();
  };
  for(char ch:ascii(value)) {
    if(ch==' '||ch=='\n'||ch=='\t')flush();
    else word.push_back(ch);
  }
  flush();if(!line.empty())lines.push_back(line);
  return lines;
}
void drawCardRulesPanel(SDL_Renderer* r,const App& a,const gwent::Card& card,int x,int y,int w,int h) {
  const bool polish=a.language==Language::Polish;
  const std::string& abilityTitle=gwent::referenceCardAbilityTitle(card.referenceIndex,polish);
  const std::string& description=gwent::referenceCardDescription(card.referenceIndex,polish);
  if(abilityTitle.empty()&&description.empty())return;
  box(r,x,y,w,h,Color{9,8,7,246});
  box(r,x,y,w,2,Color{151,111,49});
  frame(r,x,y,w,h,Color{110,80,39});
  const std::string heading=abilityTitle.empty()?displayCardName(a,card):abilityTitle;
  const int innerWidth=w-20;
  const int headingScale=textWidth(heading,2)<=innerWidth?2:1;
  text(r,x+10,y+6,heading,Color{245,211,151},headingScale);
  if(description.empty())return;
  const int bodyY=y+(headingScale==2?24:17);
  const int bodyHeight=y+h-5-bodyY;
  int scale=2;
  std::size_t columns=static_cast<std::size_t>(std::max(1,innerWidth/(6*scale)));
  auto lines=wrapDetailText(description,columns);
  int lineHeight=15;
  if(lines.size()*static_cast<std::size_t>(lineHeight)>static_cast<std::size_t>(std::max(0,bodyHeight))) {
    scale=1;lineHeight=9;
    columns=static_cast<std::size_t>(std::max(1,innerWidth/(6*scale)));
    lines=wrapDetailText(description,columns);
  }
  const std::size_t visible=std::min(lines.size(),static_cast<std::size_t>(std::max(0,bodyHeight)/lineHeight));
  for(std::size_t i=0;i<visible;++i)
    text(r,x+10,bodyY+static_cast<int>(i)*lineHeight,lines[i],Color{246,238,220},scale);
}
void drawCardDetails(SDL_Renderer* r,const App& a,const gwent::Card& card) {
  box(r,0,0,W,H,Color{0,0,0,255});
  constexpr int portraitX=8,portraitY=8,portraitW=246;
  const int portraitH=cardPortraitHeight(portraitW);
  drawCardPortrait(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),portraitX,portraitY,portraitW,portraitH);
  box(r,8,portraitY+portraitH,246,H-(portraitY+portraitH)-8,Color{12,11,9,255});
  frame(r,7,7,248,H-14,gold);
  const auto portraitName=wrapDetailText(displayCardName(a,card),38);
  int portraitTextY=portraitY+portraitH+8;
  for(std::size_t i=0;i<portraitName.size()&&i<2;++i) {
    centeredTextInBox(r,14,portraitTextY,234,9,portraitName[i],Color{255,220,153},1);
    portraitTextY+=11;
  }
  const auto& portraitAbility=gwent::referenceCardAbilityTitle(card.referenceIndex,a.language==Language::Polish);
  const auto& portraitDescription=gwent::referenceCardDescription(card.referenceIndex,a.language==Language::Polish);
  const std::string portraitCopy=portraitAbility.empty()?(portraitDescription.empty()?tr(a,"no_ability"):portraitDescription):
    (portraitDescription.empty()?portraitAbility:portraitAbility+": "+portraitDescription);
  const auto portraitLines=wrapDetailText(portraitCopy,38);
  for(const auto& line:portraitLines) {
    if(portraitTextY+9>H-10)break;
    centeredTextInBox(r,13,portraitTextY,236,9,line,Color{240,233,214},1);
    portraitTextY+=10;
  }
  box(r,264,23,354,434,Color{20,20,20,250});
  frame(r,264,23,354,434,Color{143,108,54});
  text(r,280,40,tr(a,"card_details"),gold,2);
  int y=72;
  std::string row=card.row==gwent::Row::Ranged?tr(a,"ranged"):card.row==gwent::Row::Siege?tr(a,"siege"):card.row==gwent::Row::Melee?tr(a,"melee"):tr(a,"special");
  text(r,280,y,tr(a,"power")+" "+std::to_string(card.power)+"  "+tr(a,"row")+" "+row,Color{235,207,164},1);
  y+=18;
  const bool polish=a.language==Language::Polish;
  const char* kindLabel=card.hero?(polish?"BOHATER":"HERO"):
    card.kind==gwent::Kind::Unit?(polish?"JEDNOSTKA":"UNIT"):
    card.kind==gwent::Kind::Leader?(polish?"DOWODCA":"LEADER"):
    card.kind==gwent::Kind::Weather?(polish?"POGODA":"WEATHER"):
    card.kind==gwent::Kind::ClearWeather?(polish?"CZYSTE NIEBO":"CLEAR WEATHER"):
    card.kind==gwent::Kind::CommanderHorn?(polish?"RÓG DOWÓDCY":"COMMANDER'S HORN"):
    card.kind==gwent::Kind::Scorch?(polish?"SPALENIE":"SCORCH"):
    card.kind==gwent::Kind::Mardroeme?(polish?"MARDROEME":"MARDROEME"):(polish?"MANEKIN":"DECOY");
  text(r,280,y,kindLabel,Color{242,234,218},1);
  y+=18;
  text(r,280,y,factionName(card.faction,polish),Color{195,172,129},1);
  text(r,280,433,tr(a,"close_card"),Color{185,185,185},1);
}
void drawChoiceCarousel(SDL_Renderer* r,const App& a,const std::vector<const gwent::Card*>& cards,
                        int selected,const std::string& title,const std::string& controls) {
  if(cards.empty())return;
  const int index=std::clamp(selected,0,static_cast<int>(cards.size())-1);
  centeredTextInBox(r,20,34,W-40,18,title,gold,1);
  constexpr std::array<int,5> xs{{90,171,269,381,478}};
  constexpr std::array<int,5> widths{{72,88,103,88,72}};
  constexpr std::array<int,5> heights{{136,166,195,166,136}};
  constexpr int top=73;
  for(int slot=0;slot<5;++slot) {
    const int cardIndex=index+slot-2;
    if(cardIndex<0||cardIndex>=static_cast<int>(cards.size()))continue;
    const auto& card=*cards[static_cast<std::size_t>(cardIndex)];
    int x=xs[static_cast<std::size_t>(slot)];
    const int width=widths[static_cast<std::size_t>(slot)],
              height=heights[static_cast<std::size_t>(slot)];
    // Port the source carousel's eased nudge and per-position transition
    // staggering (outer 450 ms, adjacent 300 ms, center 200 ms).
    const Uint32 elapsed=SDL_GetTicks()-a.carouselMotionStartedAt;
    const Uint32 duration=slot==2?200u:((slot==1||slot==3)?300u:450u);
    if(a.carouselMotionStartedAt&&elapsed<duration) {
      const double t=static_cast<double>(elapsed)/duration;
      const double eased=sourceEaseIn(t); // CSS cubic-bezier(.42, 0, 1, 1)
      const double offset=static_cast<double>(a.carouselMotionDirection)*-0.6*6.4*(1.0-eased);
      x+=static_cast<int>(std::lround(offset));
    }
    const int portraitHeight=std::min(height,cardPortraitHeight(width));
    drawCardPortrait(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),x,top,width,portraitHeight);
    if(slot==2) {
      const Uint32 elapsed=SDL_GetTicks()-a.carouselMotionStartedAt;
      const int pulse=a.carouselMotionStartedAt&&elapsed<100?static_cast<int>((100-elapsed)*0.30):0;
      box(r,x-4,top-4,width+8,height+8,Color{218,168,71,static_cast<Uint8>(44+pulse)});
      frame(r,x-2,top-2,width+4,height+4,gold);
      frame(r,x-1,top-1,width+2,height+2,Color{245,215,139,220});
    }
    const int nameY=top+portraitHeight;
    box(r,x,nameY,width,height-portraitHeight,Color{8,8,8,244});
    const auto nameLines=wrapDetailText(displayCardName(a,card),std::max<std::size_t>(7,static_cast<std::size_t>(width/6)));
    const int nameStart=nameY+std::max(1,(height-portraitHeight-static_cast<int>(std::min<std::size_t>(nameLines.size(),2)*8))/2);
    for(std::size_t line=0;line<nameLines.size()&&line<2;++line)
      centeredTextInBox(r,x+1,nameStart+static_cast<int>(line)*8,width-2,8,nameLines[line],paper,1);
  }
  const auto& selectedCard=*cards[static_cast<std::size_t>(index)];
  box(r,226,277,188,51,Color{20,20,20,242});
  box(r,226,277,188,1,Color{255,255,255,110});
  box(r,226,327,188,1,Color{255,255,255,87});
  std::string ability;
  if(!selectedCard.abilities.empty()) {
    ability=selectedCard.abilities.back();
    if(ability=="cerys")ability="muster";
    if(ability.rfind("avenger",0)==0)ability="avenger";
    if(ability.rfind("scorch",0)==0)ability="scorch";
  } else if(selectedCard.agile)ability="agile";
  if(!ability.empty())drawImage(r,a,"img/icons/card_ability_"+ability+".png",231,281,13,13);
  const auto& abilityTitle=gwent::referenceCardAbilityTitle(selectedCard.referenceIndex,a.language==Language::Polish);
  const auto& description=gwent::referenceCardDescription(selectedCard.referenceIndex,a.language==Language::Polish);
  if(!abilityTitle.empty())text(r,248,282,shortName(abilityTitle,27),Color{255,220,153,255},1);
  else text(r,248,282,tr(a,"power")+" "+std::to_string(selectedCard.power),Color{255,220,153,255},1);
  const auto descriptionLines=wrapDetailText(description.empty()?tr(a,"no_ability"):description,30);
  for(std::size_t line=0;line<descriptionLines.size()&&line<3;++line)
    text(r,231,296+static_cast<int>(line)*8,descriptionLines[line],Color{242,234,218,255},1);
  centeredTextInBox(r,10,STAGE_H-20,W-20,13,controls,muted,1);
}
void noteCarouselMove(App& a,int direction) {
  a.carouselMotionStartedAt=SDL_GetTicks();
  a.carouselMotionDirection=direction;
}
void drawRow(SDL_Renderer* r,const App& a,int owner,int row,int y) {
  constexpr int left=170,width=336,rowHeight=44;
  const bool player=owner==0;
  const bool chosen=player&&a.game.phase==gwent::Phase::PlayerTurn&&a.rowIndex==row;
  if(chosen)box(r,left,y,width,rowHeight,Color{255,255,0,20});
  const int total=rowScore(a.game,owner,row);
  // The source row score occupies a centered 5.1% column; high-score laurels
  // belong to the overall score medallion, not to every populated row.
  rowScoreText(r,left,y,std::to_string(total));
  if(a.game.horn[owner][row])drawImage(r,a,"img/icons/power_horn.png",left+20,y+8,27,28);
  const auto& cards=a.game.players[owner].board[row];
  const auto geometry=currentRowGeometry(a.rowLayouts[owner][row],cards.size(),SDL_GetTicks());
  for(std::size_t i=0;i<cards.size();++i) {
    const auto& card=cards[i];
    if(cardFlightHides(a,card,App::CardZone::Board,owner,row))continue;
    drawSourceCard(r,a,static_cast<int>(std::lround(geometry.firstX+i*geometry.pitch)),y,card);
  }
  if(a.weatherTransition[owner][row]||a.game.weather[row]) {
    const std::string overlay=a.weatherOverlay[owner][row].empty()?sourceWeatherOverlayForRow(row):a.weatherOverlay[owner][row];
    Uint8 alpha=255;
    if(a.weatherTransition[owner][row]) {
      const Uint32 elapsed=SDL_GetTicks()-a.weatherChangedAt[owner][row];
      const Uint32 step=std::min(elapsed,500u);
      const Uint8 amount=static_cast<Uint8>(step*255/500);
      alpha=a.weatherTo[owner][row]?amount:static_cast<Uint8>(255-amount);
    }
    if(alpha)drawImageAlpha(r,a,"img/icons/overlay_"+overlay+".png",left+19,y,width-20,rowHeight,alpha);
  }
}
void updateWeatherTransitions(App& a) {
  const Uint32 now=SDL_GetTicks();
  for(int owner=0;owner<2;++owner)for(int row=0;row<3;++row) {
    const bool active=a.game.weather[row];
    // The source adds a weather class to each affected row: frost on melee,
    // fog on ranged, rain on siege. Skellige Storm carries fog + rain, so its
    // two row visuals naturally use the same source overlays. Do not pick the
    // last weather card globally; that made one card's art leak onto every row.
    if(active)a.weatherOverlay[owner][row]=sourceWeatherOverlayForRow(row);
    if(!a.weatherInitialized) {
      a.weatherFrom[owner][row]=active;a.weatherTo[owner][row]=active;
      a.weatherChangedAt[owner][row]=now;a.weatherTransition[owner][row]=false;
    } else if(a.weatherTo[owner][row]!=active) {
      a.weatherFrom[owner][row]=!active;a.weatherTo[owner][row]=active;
      a.weatherChangedAt[owner][row]=now;a.weatherTransition[owner][row]=true;
    }
    if(a.weatherTransition[owner][row]&&SDL_TICKS_PASSED(now,a.weatherChangedAt[owner][row]+500))
      a.weatherTransition[owner][row]=false;
  }
  a.weatherInitialized=true;
}
void drawDeckSelect(SDL_Renderer* r,const App& a) {
  box(r,0,0,W,H,ink); box(r,0,0,W,5,gold); box(r,0,H-5,W,5,gold);
  text(r,22,18,tr(a,"choose"),gold,2);
  box(r,30,58,580,251,panel); frame(r,30,58,580,251,gold);
  const auto& selected=recipes()[a.deckIndex%recipes().size()];
  const auto leader=recipeLeader(a.deckIndex);
  const auto opponent=recipes()[(a.deckIndex+1)%recipes().size()];
  const auto opponentLeader=recipeLeader((a.deckIndex+1)%recipes().size());
  text(r,57,83,"<",gold,2); text(r,565,83,">",gold,2);
  const bool custom=a.userDeckSelected;
  text(r,104,87,custom?tr(a,"deck_custom"):factionName(selected.faction,a.language==Language::Polish),paper,2);
  gwent::Card deckLeader=leader;
  if(custom)gwent::referenceLeader(a.editDeck.leaderReferenceIndex,deckLeader);
  text(r,104,115,(a.language==Language::Polish?"DOWODCA: ":"LEADER: ")+displayCardName(a,deckLeader),muted,1);
  text(r,104,139,(a.language==Language::Polish?"KARTY: ":"CARDS: ")+std::to_string(custom?gwent::makeReferenceDeck(a.editDeck.cardCounts).size():gwent::makeReferenceDeck(selected).size()),muted,1);
  box(r,73,174,493,1,Color{65,72,67});
  text(r,74,193,tr(a,"opponent"),muted,1);
  text(r,74,215,factionName(opponent.faction,a.language==Language::Polish)+" / "+displayCardName(a,opponentLeader),paper,1);
  text(r,74,251,tr(a,"language"),muted,1); text(r,226,251,a.language==Language::Polish?"POLSKI":"ENGLISH",gold,1);
  text(r,25,332,custom?tr(a,"deck_slot")+" "+std::to_string(a.deckSlot)+"  B: EDIT  P: PREMADE":tr(a,"instructions"),paper,1);
  text(r,119,370,tr(a,"confirm"),gold,1);
  text(r,170,414,tr(a,"resume"),muted,1);
  text(r,25,434,"B - "+tr(a,"deck_builder")+"   [ / ] - SLOT  P - PREMADE",muted,1);
  if(!a.status.empty())text(r,470,434,shortName(a.status,25),gold,1);
}
std::vector<const gwent::Card*> editorLeaders(const App& a) {
  std::vector<const gwent::Card*> leaders;
  for(const auto& card:gwent::referenceCardCatalog())if(card.kind==gwent::Kind::Leader&&card.faction==a.editDeck.faction)leaders.push_back(&card);
  return leaders;
}
void drawDeckBuilder(SDL_Renderer* r,const App& a) {
  const bool pl=a.language==Language::Polish;
  drawImage(r,a,"img/board.jpg",0,0,W,STAGE_H);
  box(r,0,0,W,STAGE_H,Color{10,10,10,242});
  const std::string factionKey=[&](){switch(a.editDeck.faction){case gwent::Faction::NorthernRealms:return std::string("realms");case gwent::Faction::Nilfgaard:return std::string("nilfgaard");case gwent::Faction::Monsters:return std::string("monsters");case gwent::Faction::Scoiatael:return std::string("scoiatael");case gwent::Faction::Skellige:return std::string("skellige");default:return std::string("realms");}}();
  const std::string descriptions[]={
    pl?"Dobierz kartę ze swojej talii po wygranej rundzie.":"Draw a card from your deck whenever you win a round.",
    pl?"Wygrywa każdą rundę zakończoną remisem.":"Wins any round that ends in a draw.",
    pl?"Decyduje, kto wykonuje pierwszy ruch.":"Decides who takes first turn.",
    pl?"Po rundzie pozostawia losową jednostkę na polu bitwy.":"Keeps a random Unit Card out after each round.",
    pl?"Na początku trzeciej rundy przywraca 2 losowe karty z cmentarza na pole bitwy.":"2 random cards from the graveyard are placed on the battlefield at the start of the third round."
  };
  const int factionIndex=std::clamp(static_cast<int>(a.editDeck.faction),0,4);
  drawImage(r,a,"img/icons/deck_shield_"+factionKey+".png",18,9,21,21);
  narrowText(r,47,12,factionName(a.editDeck.faction,pl),Color{222,222,222});
  narrowText(r,130,34,descriptions[factionIndex],Color{171,151,124});
  narrowText(r,256,54,pl?"WCZYTAJ TALIE":"UPLOAD DECK",paper);
  narrowText(r,300,54,pl?"ZMIEN FRAKCJE":"CHANGE FACTION",paper);
  narrowText(r,355,54,pl?"ZAPISZ TALIE":"DOWNLOAD DECK",paper);
  narrowText(r,54,27,pl?"KOLEKCJA KART":"CARD COLLECTION",Color{149,149,149});
  narrowText(r,540,28,pl?"TALIA":"DECK",Color{149,149,149});

  const auto cards=editorCards(a);
  std::vector<const gwent::Card*> bankCards,deckCards;
  for(const auto* card:cards) {
    const int count=deckCopies(a.editDeck,card->referenceIndex);
    if(count>0)deckCards.push_back(card);
    if(count<card->deckCopies)bankCards.push_back(card);
  }
  const auto& shownCards=a.editorPane==0?bankCards:deckCards;
  const int cursor=shownCards.empty()?0:std::clamp(a.editorCardIndex,0,static_cast<int>(shownCards.size())-1);
  const int first=std::clamp(cursor-5,0,std::max(0,static_cast<int>(shownCards.size())-6));
  const auto drawArray=[&](const std::vector<const gwent::Card*>& source,int x,int pane) {
    const int start=(pane==a.editorPane)?first:0;
    for(int i=start;i<std::min(static_cast<int>(source.size()),start+6);++i) {
      const int local=i-start,col=local%3,row=local/3;
      const int cx=x+7+col*71,cy=81+row*123;
      const auto* card=source[static_cast<std::size_t>(i)];
      const int deckCount=deckCopies(a.editDeck,card->referenceIndex);
      const int count=pane==0?card->deckCopies-deckCount:deckCount;
      const int portraitHeight=std::min(118,cardPortraitHeight(63));
      drawCardPortrait(r,a,"img/lg/"+gwent::referenceCardImage(card->referenceIndex),cx,cy,63,portraitHeight);
      box(r,cx,cy+portraitHeight,63,118-portraitHeight,Color{9,8,7,246});
      drawImage(r,a,"img/icons/preview_count.png",cx+45,cy+3,16,16);
      const auto countLabel=std::to_string(count);
      centeredTextInBox(r,cx+45,cy+3,16,16,countLabel,Color{95,73,35},1);
      const auto cardNameLines=wrapDetailText(displayCardName(a,*card),10);
      const int titleY=cy+portraitHeight+std::max(0,(118-portraitHeight-static_cast<int>(std::min<std::size_t>(cardNameLines.size(),2)*8))/2);
      for(std::size_t line=0;line<cardNameLines.size()&&line<2;++line)
        centeredTextInBox(r,cx+1,titleY+static_cast<int>(line)*8,61,8,cardNameLines[line],paper,1);
      if(pane==a.editorPane&&i==cursor)frame(r,cx-2,cy-2,67,122,paper);
    }
    const int trackX=x+210;
    box(r,trackX,81,7,245,Color{19,19,19,180});
    const int thumbHeight=std::max(18,245*6/std::max(6,static_cast<int>(source.size())));
    const int thumbY=81+(source.size()<=6?0:(245-thumbHeight)*start/std::max(1,static_cast<int>(source.size())-6));
    box(r,trackX+2,thumbY+2,3,thumbHeight-4,Color{85,85,85,170});
  };
  drawArray(bankCards,53,0);drawArray(deckCards,374,1);

  auto leaders=editorLeaders(a);
  int leaderIndex=0;
  if(!leaders.empty()) {
    const auto selected=std::find_if(leaders.begin(),leaders.end(),[&](const auto* card){return card->referenceIndex==a.editDeck.leaderReferenceIndex;});
    if(selected!=leaders.end())leaderIndex=static_cast<int>(selected-leaders.begin());
    narrowText(r,291,76,pl?"LIDER":"LEADER",Color{143,108,54});
    drawImage(r,a,"img/lg/"+gwent::referenceCardImage(leaders[static_cast<std::size_t>(leaderIndex)]->referenceIndex),298,89,44,82);
  }

  int total=0,units=0,special=0,strength=0,heroes=0;
  for(const auto* card:cards) {
    const int count=deckCopies(a.editDeck,card->referenceIndex);total+=count;
    if(card->kind==gwent::Kind::Weather||card->kind==gwent::Kind::ClearWeather||card->kind==gwent::Kind::Decoy||card->kind==gwent::Kind::Scorch||card->kind==gwent::Kind::CommanderHorn||card->kind==gwent::Kind::Mardroeme)special+=count;
    else {units+=count;strength+=card->power*count;if(card->hero)heroes+=count;}
  }
  const char* statIcons[]={"count","unit","special","strength","hero"};
  const std::string statLabels[]={pl?"KARTY W TALII":"TOTAL CARDS IN DECK",pl?"JEDNOSTKI":"UNIT CARDS",pl?"KARTY SPECJALNE":"SPECIAL CARDS",pl?"SILA JEDNOSTEK":"UNIT CARD STRENGTH",pl?"BOHATEROWIE":"HERO CARDS"};
  const std::string statValues[]={std::to_string(total),std::to_string(units)+(units<22?"/22":""),std::to_string(special)+"/10",std::to_string(strength),std::to_string(heroes)};
  for(int i=0;i<5;++i) {
    const int y=181+i*20;
    narrowText(r,280,y,statLabels[static_cast<std::size_t>(i)],Color{130,115,95});
    drawImage(r,a,"img/icons/deck_stats_"+std::string(statIcons[i])+".png",295,y+8,16,16);
    narrowText(r,312,y+12,statValues[static_cast<std::size_t>(i)],Color{159,125,62});
  }
  box(r,298,310,43,11,Color{20,20,20});frame(r,298,310,43,11,Color{55,55,55});
  narrowText(r,301,312,pl?"GRAJ":"START GAME",paper);
  const auto opponent=recipes().empty()?gwent::ReferenceDeck{}:recipes()[(a.deckIndex+1)%recipes().size()];
  const auto opponentLeader=recipeLeader((a.deckIndex+1)%recipes().size());
  narrowText(r,271,330,pl?"TALIA PRZECIWNIKA":"OPPONENT'S DECK",Color{130,115,95});
  narrowText(r,300,343,factionName(opponent.faction,pl),paper);
  drawImage(r,a,"img/icons/deck_back_"+std::string(opponent.faction==gwent::Faction::Monsters?"monsters":opponent.faction==gwent::Faction::Nilfgaard?"nilfgaard":opponent.faction==gwent::Faction::Scoiatael?"scoiatael":opponent.faction==gwent::Faction::Skellige?"skellige":"realms")+".jpg",354,330,23,28);
  const int toggleY=290;
  drawImage(r,a,"img/icons/ui_music.png",299,toggleY,16,16);drawImage(r,a,"img/icons/ui_sfx.png",321,toggleY,16,16);
  drawImage(r,a,"img/icons/ui_notification.png",343,toggleY,16,16);
  if(!a.musicEnabled) { color(r,gold);SDL_RenderDrawLine(r,299,toggleY,315,toggleY+16); }
  if(!a.audio.enabled()) { color(r,gold);SDL_RenderDrawLine(r,321,290,337,306); }
  if(!a.notificationsEnabled) { color(r,gold);SDL_RenderDrawLine(r,343,290,359,306); }
  if(!shownCards.empty()) {
    const auto& selected=*shownCards[static_cast<std::size_t>(cursor)];
    drawCardPreview(r,a,selected);
    if(a.cardDetailsOpen)drawCardDetails(r,a,selected);
    else drawCardRulesPanel(r,a,selected,8,DECK_RULES_PANEL_Y,W-16,DECK_RULES_PANEL_H);
  }
}
void drawMatch(SDL_Renderer* r,const App& a) {
  drawImage(r,a,"img/board.jpg",0,0,W,STAGE_H);
  const auto& me=a.game.players[0]; const auto& op=a.game.players[1];
  const auto factionKey=[](gwent::Faction faction){switch(faction){case gwent::Faction::Monsters:return "monsters";case gwent::Faction::Nilfgaard:return "nilfgaard";case gwent::Faction::Scoiatael:return "scoiatael";case gwent::Faction::Skellige:return "skellige";default:return "realms";}};
  const auto drawSide=[&](const gwent::Player& player,int owner) {
    const int leaderY=owner==1?26:279;
    drawImage(r,a,"img/sm/"+gwent::referenceCardImage(player.leader.referenceIndex),47,leaderY,33,45);
    const int statsY=owner==1?87:222;
    drawImage(r,a,"img/icons/profile.png",48,statsY+4,32,32);
    drawImage(r,a,"img/icons/icon_player_border.png",44,statsY+1,39,39);
    if(owner==0)text(r,91,statsY+27,shortName(displayCardName(a,player.leader),9),paper,1);
    else text(r,91,statsY+3,tr(a,"opponent_short"),paper,1);
    const int gemY=statsY+(owner==1?25:6);
    for(int gem=0;gem<2;++gem)
      drawImage(r,a,"img/icons/icon_gem_"+std::string(gem<2-player.roundLosses?"on":"off")+".png",109+gem*18,gemY,13,15);
    const int handY=statsY+(owner==1?26:8);
    // Keep the hand counter in its own narrow column, clear of the two gems.
    drawImage(r,a,"img/icons/icon_card_count.png",84,handY+1,11,12);
    tallTextInBox(r,96,handY,12,14,std::to_string(player.hand.size()),paper);
    const int scoreX=142,scoreY=statsY+12,scoreSize=24;
    const bool leading=owner==0?me.score>op.score:op.score>me.score;
    if(leading)drawImage(r,a,"img/icons/icon_high_score.png",131,statsY+4,47,41);
    drawImage(r,a,"img/icons/score_total_"+std::string(owner==0?"me":"op")+".png",scoreX,scoreY,scoreSize,scoreSize);
    const std::string score=std::to_string(player.score);
    const int scale=score.size()>=2?1:2;
    centeredTextInBox(r,scoreX,scoreY,scoreSize,scoreSize,score,ink,scale);
  };
  drawSide(op,1);drawSide(me,0);
  drawImage(r,a,"img/icons/icon_leader_active.png",79,43,13,13);
  drawImage(r,a,"img/icons/icon_leader_active.png",79,296,13,13);
  // Original CSS lays three 32%-height rows inside each 38.5%-height field,
  // with a 4% inset. At the reference 640x360 stage that maps to ~45px rows.
  for(int i=0;i<3;++i)drawRow(r,a,1,2-i,5+i*45);
  for(int i=0;i<3;++i)drawRow(r,a,0,i,143+i*45);
  int weatherX=47;
  for(const auto& active:a.game.weatherCards) {
    if(weatherX>133)break;
    if(!cardFlightHides(a,active.card,App::CardZone::Weather,active.owner))drawSourceCard(r,a,weatherX,149,active.card);
    weatherX+=29;
  }
  const auto drawPile=[&](int owner,bool grave,int y) {
    const auto& pile=grave?a.game.players[owner].graveyard:a.game.players[owner].deck;
    const int x=grave?514:572;
    const auto visible=std::find_if(pile.rbegin(),pile.rend(),[&](const gwent::Card& card){
      return !cardFlightHides(a,card,grave?App::CardZone::Grave:App::CardZone::Deck,owner);
    });
    if(grave&&visible!=pile.rend())drawImage(r,a,"img/sm/"+gwent::referenceCardImage(visible->referenceIndex),x,y,28,43);
    else drawImage(r,a,"img/icons/deck_back_"+std::string(factionKey(a.game.players[owner].faction))+".jpg",x,y,28,43);
    const std::string count=std::to_string(pile.size());
    centeredTextInBox(r,x,y+29,28,13,count,paper,1);
  };
  drawPile(1,true,23);drawPile(1,false,23);drawPile(0,true,275);drawPile(0,false,275);
  box(r,107,295,34,15,Color{20,20,20,230});frame(r,107,295,34,15,Color{143,108,54});text(r,112,299,tr(a,"pass_button"),paper,1);
  if(me.passed)text(r,110,282,tr(a,"passed"),paper,1);
  if(op.passed)text(r,110,80,tr(a,"passed"),paper,1);
  drawImage(r,a,"img/icons/ui_music.png",90,313,16,16);drawImage(r,a,"img/icons/ui_sfx.png",110,313,16,16);drawImage(r,a,"img/icons/ui_notification.png",128,313,16,16);
  if(!a.musicEnabled) { color(r,gold);SDL_RenderDrawLine(r,90,313,106,329); }
  if(!a.audio.enabled()) { color(r,gold);SDL_RenderDrawLine(r,110,313,126,329); }
  if(!a.notificationsEnabled) { color(r,gold);SDL_RenderDrawLine(r,128,313,144,329); }
  const int count=static_cast<int>(me.hand.size());
  if(count) {
    const int start=std::clamp(a.handIndex-5,0,std::max(0,count-11));
    const int shown=std::min(11,count-start);
    for(int i=0;i<shown;++i) {
      const int index=start+i,x=194+i*29;
      const bool marked=std::find(a.mulligan.begin(),a.mulligan.end(),me.hand[index].id)!=a.mulligan.end();
      if(!cardFlightHides(a,me.hand[index],App::CardZone::Hand,0))drawSourceCard(r,a,x,277,me.hand[index],index==a.handIndex);
      if(marked)drawImage(r,a,"img/icons/icon_gem_on.png",x+17,274,12,12);
    }
  }
  if(a.game.phase==gwent::Phase::Mulligan) {
    box(r,0,0,W,STAGE_H,Color{0,0,0,195});
    drawImage(r,a,"img/icons/notif_round_start.png",110,142,86,76);
    if(a.game.scoiataelChoicePending) {
      text(r,214,161,tr(a,"choose_first"),paper,1);
    } else {
      text(r,214,161,tr(a,"mulligan"),paper,1);
      const int remaining=std::max(0,2-static_cast<int>(a.mulligan.size()));
      const std::string counter=(a.language==Language::Polish?"WYMIANY: ":"REDRAWS: ")+std::to_string(remaining)+"/2";
      text(r,214,185,counter,gold,1);
    }
  } else if(a.game.phase==gwent::Phase::RoundSetup) {
    std::vector<const gwent::Card*> candidates;
    for(const auto& card:me.graveyard)if(std::find(a.game.skelligeAgileReviveIds.begin(),a.game.skelligeAgileReviveIds.end(),card.id)!=a.game.skelligeAgileReviveIds.end())candidates.push_back(&card);
    box(r,0,0,W,STAGE_H,Color{0,0,0,195});frame(r,0,0,W,STAGE_H,Color{20,20,20});
    const int selected=std::clamp(a.targetIndex,0,std::max(0,static_cast<int>(candidates.size())-1));
    const bool agile=!candidates.empty()&&candidates[static_cast<std::size_t>(selected)]->agile;
    const std::string controls=a.language==Language::Polish?
      (agile?"LEWO/PRAWO: KARTA   GORA/DOL: RZAD   A: WYBIERZ":"LEWO/PRAWO: KARTA   A: WYBIERZ"):
      (agile?"LEFT/RIGHT: CARD   UP/DOWN: ROW   A: SELECT":"LEFT/RIGHT: CARD   A: SELECT");
    drawChoiceCarousel(r,a,candidates,a.targetIndex,tr(a,"skellige_revival"),controls);
    if(agile)centeredTextInBox(r,180,252,280,15,tr(a,a.rowIndex==0?"melee":"ranged"),gold,1);
  }
  if(a.game.phase==gwent::Phase::RoundResult || a.game.phase==gwent::Phase::MatchResult) {
    const Uint8 alpha=endScreenAlpha(SDL_GetTicks()-a.endScreenStartedAt);
    box(r,0,0,W,STAGE_H,Color{10,10,10,static_cast<Uint8>(242u*alpha/255)});
    const bool done=a.game.phase==gwent::Phase::MatchResult;
    const bool won=a.game.matchWinner==0;
    drawImageAlpha(r,a,std::string("img/icons/end_")+(done?(won?"win":"lose"):(won?"win":"lose"))+".png",210,21,220,102,alpha);
    text(r,213,139,done?tr(a,won?"victory":"defeat"):tr(a,"round_complete"),Color{gold.r,gold.g,gold.b,alpha},2);
    text(r,220,174,tr(a,"player_short")+" "+std::to_string(me.score)+"  -  "+std::to_string(op.score)+" "+tr(a,"opponent_short"),Color{paper.r,paper.g,paper.b,alpha},1);
    text(r,265,217,tr(a,"continue"),Color{muted.r,muted.g,muted.b,alpha},1);
  }
  const bool showHand=a.game.phase!=gwent::Phase::RoundResult&&a.game.phase!=gwent::Phase::MatchResult;
  if((!me.hand.empty()||a.cardFlight.active)&&(showHand||a.cardFlight.active)) {
    // The engine removes a played card from the hand before its source preview
    // and flight animation finish. Keep the inspector locked to that card until
    // the flight ends instead of previewing the next card in the hand.
    const auto& preview=a.cardFlight.active?a.cardFlight.card:
      me.hand[static_cast<std::size_t>(std::clamp(a.handIndex,0,static_cast<int>(me.hand.size())-1))];
    drawCardPreview(r,a,preview);
  }
  if(a.cardFlight.active)drawCardFlight(r,a,a.cardFlight);
  for(const auto& flight:a.triggeredCardFlights)drawCardFlight(r,a,flight);
  drawCardEffect(r,a);
  // The full-screen inspector is drawn after the 16:9 viewport is removed.
  if(a.targetMode&&a.handIndex>=0&&a.handIndex<static_cast<int>(me.hand.size())&&
     me.hand[static_cast<std::size_t>(a.handIndex)].medic&&a.targetIndex>=0&&a.targetIndex<static_cast<int>(a.selectedTargets.size())) {
    const auto revived=std::find_if(me.graveyard.begin(),me.graveyard.end(),[&](const gwent::Card& c){return c.id==a.selectedTargets[static_cast<std::size_t>(a.targetIndex)];});
    if(revived!=me.graveyard.end()&&revived->agile) {
      box(r,148,240,344,39,Color{10,9,8,238});frame(r,148,240,344,39,gold);
      centeredTextInBox(r,154,244,332,12,a.language==Language::Polish?"WZNAWIANA KARTA AGILE":"REVIVING AGILE CARD",paper,1);
      centeredTextInBox(r,154,260,332,13,(a.reviveRowIndex==0?tr(a,"melee"):tr(a,"ranged"))+"  -  UP/DOWN  -  A/X",gold,1);
    }
  }
  if(a.game.phase==gwent::Phase::Mulligan&&!a.game.scoiataelChoicePending) {
    box(r,8,328,410,43,Color{7,6,5,238});frame(r,8,328,410,43,Color{143,108,54});
    centeredTextInBox(r,16,333,394,13,tr(a,"mulligan_controls"),paper,1);
    centeredTextInBox(r,16,351,394,13,tr(a,"mulligan_details"),Color{214,190,150},1);
  }
  if(a.leaderSelectionMode) {
    const auto cards=leaderChoiceCards(a);
    if(!cards.empty()) {
      box(r,0,0,W,STAGE_H,Color{0,0,0,238});
      frame(r,14,12,W-28,STAGE_H-24,Color{143,108,54});
      std::vector<const gwent::Card*> choices;
      for(const auto& card:cards)choices.push_back(&card);
      std::string title=leaderChoicePrompt(a);
      if(activeLeaderAbility(me.leader)=="eredin_destroyer")
        title+="  "+tr(a,"leader_selected")+" "+std::to_string(a.leaderSelections.size())+"/2";
      std::string controls=tr(a,"leader_controls");
      if(activeLeaderAbility(me.leader)!="eredin_destroyer"&&a.leaderSelections.empty())
        controls+="   "+tr(a,"leader_cancel");
      drawChoiceCarousel(r,a,choices,a.targetIndex,title,controls);
    }
  }
  if(a.targetMode) {
    std::vector<const gwent::Card*> choices;
    for(const auto& id:a.selectedTargets)if(const auto* card=findTargetCard(a.game,id))choices.push_back(card);
    std::string controls=a.language==Language::Polish?
      "LEWO/PRAWO: KARTA   A: WYBIERZ   B: ANULUJ":
      "LEFT/RIGHT: CARD   A: SELECT   B: CANCEL";
    drawChoiceCarousel(r,a,choices,a.targetIndex,tr(a,"target"),controls);
  }
  if(a.leaderRevealOpen) {
    box(r,0,0,W,STAGE_H,Color{0,0,0,242});
    frame(r,18,12,W-36,STAGE_H-24,Color{143,108,54});
    centeredTextInBox(r,32,20,W-64,24,tr(a,"leader_reveal"),gold,2);
    const auto& cards=a.game.revealedCards[0];
    const int width=std::min(106,static_cast<int>((W-100)/std::max<std::size_t>(1,cards.size())));
    const int gap=cards.size()>1?16:0;
    const int total=static_cast<int>(cards.size())*width+(static_cast<int>(cards.size())-1)*gap;
    const int firstX=(W-total)/2;
    for(std::size_t i=0;i<cards.size();++i) {
      const int x=firstX+static_cast<int>(i)*(width+gap);
      const int height=static_cast<int>(width*1.72);
      drawImage(r,a,"img/lg/"+gwent::referenceCardImage(cards[i].referenceIndex),x,58,width,height);
      frame(r,x-1,57,width+2,height+2,gold);
      const auto names=wrapDetailText(displayCardName(a,cards[i]),16);
      for(std::size_t line=0;line<names.size();++line)
        centeredTextInBox(r,x-8,58+height+8+static_cast<int>(line)*8,width+16,8,names[line],paper,1);
    }
    centeredTextInBox(r,20,STAGE_H-35,W-40,16,tr(a,"leader_continue"),muted,1);
  }
  if(a.passConfirmationOpen) {
    box(r,0,0,W,STAGE_H,Color{0,0,0,220});
    frame(r,150,115,340,112,gold);
    box(r,152,117,336,108,panel);
    centeredTextInBox(r,166,137,308,20,tr(a,"pass_confirm"),gold,2);
    centeredTextInBox(r,166,185,308,16,tr(a,"pass_confirm_controls"),paper,1);
  }
  if(a.pauseOpen) {
    box(r,0,0,W,STAGE_H,Color{0,0,0,230});
    frame(r,120,22,400,316,gold);
    box(r,122,24,396,312,panel);
    centeredTextInBox(r,142,35,356,20,tr(a,"pause"),gold,2);
    const std::array<std::string,8> options={
      tr(a,"pause_resume"),tr(a,"pause_save"),tr(a,"pause_load"),
      tr(a,"pause_language")+": "+(a.language==Language::Polish?"POLSKI":"ENGLISH"),
      tr(a,"pause_sfx")+": "+tr(a,a.sfxEnabled?"on":"off"),
      tr(a,"pause_music")+": "+tr(a,a.musicEnabled?"on":"off"),
      tr(a,"pause_notifications")+": "+tr(a,a.notificationsEnabled?"on":"off"),tr(a,"pause_exit")
    };
    for(std::size_t i=0;i<options.size();++i) {
      const int y=68+static_cast<int>(i)*28;
      if(static_cast<int>(i)==a.pauseIndex) {
        box(r,146,y-4,348,24,panelLight);
        frame(r,146,y-4,348,24,gold);
      }
      text(r,160,y,options[i],static_cast<int>(i)==a.pauseIndex?gold:paper,1);
    }
    centeredTextInBox(r,140,304,360,16,tr(a,"pause_controls"),muted,1);
  }
}
void drawMatchOnScreen(SDL_Renderer* r,const App& a) {
  // The reference match board is 16:9 (640x360); center it on Miyoo's 4:3
  // 640x480 screen instead of pinning the unused 120 px to the bottom.
  SDL_Rect stageViewport{0,(H-STAGE_H)/2,W,STAGE_H};
  SDL_RenderSetViewport(r,&stageViewport);
  drawMatch(r,a);
  drawNotification(r,a);
  SDL_RenderSetViewport(r,nullptr);
  if(a.cardDetailsOpen) {
    const auto& hand=a.game.players[0].hand;
    if(!hand.empty())drawCardDetails(r,a,hand[static_cast<std::size_t>(std::clamp(a.handIndex,0,static_cast<int>(hand.size())-1))]);
  } else if(!a.game.players[0].hand.empty()&&
            a.game.phase!=gwent::Phase::RoundResult&&a.game.phase!=gwent::Phase::MatchResult) {
    const auto& hand=a.game.players[0].hand;
    drawCardRulesPanel(r,a,hand[static_cast<std::size_t>(std::clamp(a.handIndex,0,static_cast<int>(hand.size())-1))],6,MATCH_RULES_PANEL_Y,W-12,MATCH_RULES_PANEL_H);
  }
}
void handleMatchKey(App& a,SDL_Keycode key) {
  if(a.pauseOpen) {
    constexpr int optionCount=8;
    if(key==SDLK_ESCAPE||key==SDLK_z||key==SDLK_BACKSPACE) { a.pauseOpen=false;return; }
    if(key==SDLK_UP||key==SDLK_LEFT) { a.pauseIndex=(a.pauseIndex+optionCount-1)%optionCount;return; }
    if(key==SDLK_DOWN||key==SDLK_RIGHT) { a.pauseIndex=(a.pauseIndex+1)%optionCount;return; }
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE) {
      switch(a.pauseIndex) {
        case 0:a.pauseOpen=false;break;
        case 1:saveCurrentMatch(a);break;
        case 2:a.pauseOpen=false;loadSavedMatch(a);break;
        case 3:a.language=a.language==Language::Polish?Language::English:Language::Polish;saveSettings(a);break;
        case 4:a.sfxEnabled=!a.sfxEnabled;a.audio.setEnabled(a.sfxEnabled);saveSettings(a);break;
      case 5:a.musicEnabled=!a.musicEnabled;a.audio.setMusicEnabled(a.musicEnabled);saveSettings(a);break;
      case 6:a.notificationsEnabled=!a.notificationsEnabled;saveSettings(a);break;
      case 7:saveCurrentMatch(a);a.pauseOpen=false;a.screen=Screen::DeckSelect;break;
      }
    }
    return;
  }
  if(a.passConfirmationOpen) {
    if(key==SDLK_ESCAPE||key==SDLK_z||key==SDLK_BACKSPACE) { a.passConfirmationOpen=false;return; }
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE) {
      a.passConfirmationOpen=false;
      if(gwent::pass(a.game,0)) { a.audio.play("pass");a.status=tr(a,"you_passed");if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
      return;
    }
    return;
  }
  if(key==SDLK_p) {
    if(a.game.phase==gwent::Phase::Mulligan) { handleMatchKey(a,SDLK_RETURN);return; }
    a.pauseOpen=true;a.pauseIndex=0;return;
  }
  if(key==SDLK_BACKQUOTE) {
    if(a.game.phase==gwent::Phase::PlayerTurn&&!a.leaderSelectionMode&&!a.leaderRevealOpen&&!a.targetMode)a.passConfirmationOpen=true;
    else if(a.game.phase==gwent::Phase::Mulligan)handleMatchKey(a,SDLK_RETURN);
    else if(a.game.phase==gwent::Phase::RoundSetup)handleMatchKey(a,SDLK_x);
    else if(a.game.phase==gwent::Phase::RoundResult)handleMatchKey(a,SDLK_RETURN);
    else if(a.game.phase==gwent::Phase::MatchResult)a.screen=Screen::DeckSelect;
    return;
  }
  if(a.leaderRevealOpen) {
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE||key==SDLK_ESCAPE||key==SDLK_z||key==SDLK_BACKSPACE) {
      a.leaderRevealOpen=false;
      if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+350;
    }
    return;
  }
  if(a.leaderSelectionMode) {
    const auto cards=leaderChoiceCards(a);
    if(cards.empty()) { a.leaderSelectionMode=false;a.leaderSelections.clear();a.status=tr(a,"leader_no_target");return; }
    if(key==SDLK_ESCAPE||key==SDLK_z||key==SDLK_BACKSPACE) {
      a.leaderSelectionMode=false;a.leaderSelections.clear();a.targetIndex=0;a.status=tr(a,"your_turn");return;
    }
    if(key==SDLK_LEFT||key==SDLK_a) { const int old=a.targetIndex;a.targetIndex=std::max(0,a.targetIndex-1);a.audio.play("ui_card");if(old!=a.targetIndex)noteCarouselMove(a,-1); }
    else if(key==SDLK_RIGHT||key==SDLK_d) { const int old=a.targetIndex;a.targetIndex=std::min(static_cast<int>(cards.size())-1,a.targetIndex+1);a.audio.play("ui_card");if(old!=a.targetIndex)noteCarouselMove(a,1); }
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE) {
      a.leaderSelections.push_back(cards[static_cast<std::size_t>(a.targetIndex)].id);
      if(activeLeaderAbility(a.game.players[0].leader)=="eredin_destroyer"&&a.leaderSelections.size()<3) {
        a.targetIndex=0;a.status=leaderChoicePrompt(a);a.audio.play("ui_card");noteCarouselMove(a,1);
      } else finishLeaderChoice(a);
    }
    return;
  }
  if(key==SDLK_ESCAPE) {
    if(a.targetMode) { a.targetMode=false;a.selectedTargets.clear();a.status=tr(a,"your_turn");return; }
    saveCurrentMatch(a);a.screen=Screen::DeckSelect;return;
  }
  if(key==SDLK_s||key==SDLK_F5) { saveCurrentMatch(a);return; }
  if(key==SDLK_o||key==SDLK_F9) { loadSavedMatch(a);return; }
  if(a.game.phase==gwent::Phase::MatchResult) {
    if(key==SDLK_RETURN||key==SDLK_x||key==SDLK_SPACE) { a.screen=Screen::DeckSelect; return; }
    return;
  }
  if(a.game.phase==gwent::Phase::RoundResult) {
    if(key==SDLK_RETURN||key==SDLK_x||key==SDLK_SPACE) {
      gwent::startNextRound(a.game); a.handIndex=0; a.rowIndex=0; a.targetIndex=0; a.status=tr(a,"your_turn");
      if(a.game.phase==gwent::Phase::RoundSetup)a.audio.play("open");
      if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
    }
    return;
  }
  if(a.game.phase==gwent::Phase::RoundSetup) {
    std::vector<const gwent::Card*> candidates;
    for(const auto& card:a.game.players[0].graveyard)if(std::find(a.game.skelligeAgileReviveIds.begin(),a.game.skelligeAgileReviveIds.end(),card.id)!=a.game.skelligeAgileReviveIds.end())candidates.push_back(&card);
    if(candidates.empty())return;
    a.targetIndex=std::clamp(a.targetIndex,0,static_cast<int>(candidates.size())-1);
    if(key==SDLK_LEFT||key==SDLK_a) { const int old=a.targetIndex;a.targetIndex=std::max(0,a.targetIndex-1);a.audio.play("ui_card");if(old!=a.targetIndex)noteCarouselMove(a,-1); }
    else if(key==SDLK_RIGHT||key==SDLK_d) { const int old=a.targetIndex;a.targetIndex=std::min(static_cast<int>(candidates.size())-1,a.targetIndex+1);a.audio.play("ui_card");if(old!=a.targetIndex)noteCarouselMove(a,1); }
    else if((key==SDLK_UP||key==SDLK_DOWN)&&candidates[static_cast<std::size_t>(a.targetIndex)]->agile)a.rowIndex=1-a.rowIndex;
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) {
      const auto* card=candidates[static_cast<std::size_t>(a.targetIndex)];
      const auto row=card->agile?static_cast<gwent::Row>(a.rowIndex):gwent::Row::None;
      if(gwent::chooseSkelligeRevive(a.game,0,card->id,row)) {
        a.status=tr(a,"skellige_revived")+": "+displayCardName(a,*card); a.targetIndex=0;
        if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+450;
      }
    }
    return;
  }
  if(a.game.phase==gwent::Phase::Mulligan) {
    if(key==SDLK_l) { handleMatchKey(a,SDLK_RETURN);return; }
    if(a.game.scoiataelChoicePending) {
      if(key==SDLK_LEFT||key==SDLK_a||key==SDLK_RIGHT||key==SDLK_d)a.game.firstPlayer=1-a.game.firstPlayer;
      else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x)gwent::chooseScoiataelFirstPlayer(a.game,a.game.firstPlayer);
      return;
    }
    auto& hand=a.game.players[0].hand;
    if(key==SDLK_LEFT||key==SDLK_a) { a.handIndex=(a.handIndex+static_cast<int>(hand.size())-1)%static_cast<int>(hand.size());a.audio.play("ui_card"); }
    else if(key==SDLK_RIGHT||key==SDLK_d) { a.handIndex=(a.handIndex+1)%static_cast<int>(hand.size());a.audio.play("ui_card"); }
    else if((key==SDLK_SPACE||key==SDLK_x)&&!hand.empty()) {
      const auto& id=hand[a.handIndex].id; const auto it=std::find(a.mulligan.begin(),a.mulligan.end(),id);
      if(it!=a.mulligan.end())a.mulligan.erase(it); else if(a.mulligan.size()<2)a.mulligan.push_back(id);
      a.audio.play("ui_card");
    } else if(key==SDLK_BACKSPACE||key==SDLK_z) { if(!a.mulligan.empty())a.mulligan.pop_back(); }
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER) {
      if(gwent::replaceMulliganCards(a.game,0,a.mulligan) && gwent::performAiMulligan(a.game,1)) {
        a.audio.play("game_start");a.audio.play("round1_start");
        a.mulligan.clear(); a.status=tr(a,"your_turn");
        if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+450;
      } else a.status=tr(a,"mulligan_error");
    }
    return;
  }
  if(a.game.phase!=gwent::Phase::PlayerTurn)return;
  auto& hand=a.game.players[0].hand;
  if(a.targetMode) {
    if(key==SDLK_ESCAPE||key==SDLK_z) { a.targetMode=false; a.selectedTargets.clear(); a.status=tr(a,"your_turn"); }
    else if((key==SDLK_UP||key==SDLK_DOWN||key==SDLK_LEFT||key==SDLK_RIGHT)&&!a.selectedTargets.empty()&&a.targetIndex>=0&&a.targetIndex<static_cast<int>(a.selectedTargets.size())&&
            a.handIndex>=0&&a.handIndex<static_cast<int>(hand.size())) {
      const auto& selectedId=a.selectedTargets[static_cast<std::size_t>(a.targetIndex)];
      const auto revived=std::find_if(a.game.players[0].graveyard.begin(),a.game.players[0].graveyard.end(),[&](const gwent::Card& c){return c.id==selectedId;});
      const bool chooseReviveRow=hand[static_cast<std::size_t>(a.handIndex)].medic&&
                                  revived!=a.game.players[0].graveyard.end()&&revived->agile;
      if(chooseReviveRow&&(key==SDLK_UP||key==SDLK_DOWN))a.reviveRowIndex=1-a.reviveRowIndex;
      else if(key==SDLK_UP||key==SDLK_LEFT) { const int old=a.targetIndex;a.targetIndex=std::max(0,a.targetIndex-1);a.audio.play("ui_card");if(old!=a.targetIndex)noteCarouselMove(a,-1); }
      else if(key==SDLK_DOWN||key==SDLK_RIGHT) { const int old=a.targetIndex;a.targetIndex=std::min(static_cast<int>(a.selectedTargets.size())-1,a.targetIndex+1);a.audio.play("ui_card");if(old!=a.targetIndex)noteCarouselMove(a,1); }
    }
    else if(key==SDLK_RETURN||key==SDLK_x) {
      if(!a.selectedTargets.empty()&&a.handIndex>=0&&a.handIndex<static_cast<int>(hand.size())) {
        const auto card=hand[a.handIndex];
        const int handSizeBefore=static_cast<int>(hand.size());
        const auto& selectedId=a.selectedTargets[static_cast<std::size_t>(a.targetIndex)];
        const auto revived=std::find_if(a.game.players[0].graveyard.begin(),a.game.players[0].graveyard.end(),[&](const gwent::Card& c){return c.id==selectedId;});
        const auto playRow=card.medic&&revived!=a.game.players[0].graveyard.end()&&revived->agile?
                           static_cast<gwent::Row>(a.reviveRowIndex):static_cast<gwent::Row>(a.rowIndex);
        const auto before=a.game;
        if(gwent::playCard(a.game,0,card.id,playRow,selectedId)) {
          animatePlayedCard(a,card,0,a.handIndex,handSizeBefore);
          queueTriggeredEffects(a,before,card,0);
          playCardSound(a,card,before,0,playRow);
          a.status=displayCardName(a,card)+(a.language==Language::Polish?" - ZAGRANO":" - PLAYED"); a.handIndex=std::min(a.handIndex,static_cast<int>(a.game.players[0].hand.size())-1);
        } else a.status=tr(a,"illegal_target");
      }
      a.targetMode=false; a.selectedTargets.clear();
    }
    return;
  }
  if(key==SDLK_LEFT||key==SDLK_a) { if(!hand.empty())a.handIndex=(a.handIndex+static_cast<int>(hand.size())-1)%static_cast<int>(hand.size());a.audio.play("ui_card"); }
  else if(key==SDLK_RIGHT||key==SDLK_d) { if(!hand.empty())a.handIndex=(a.handIndex+1)%static_cast<int>(hand.size());a.audio.play("ui_card"); }
  else if(key==SDLK_PAGEUP) { if(!hand.empty())a.handIndex=(a.handIndex+static_cast<int>(hand.size())-(5%static_cast<int>(hand.size())))%static_cast<int>(hand.size()); }
  else if(key==SDLK_PAGEDOWN) { if(!hand.empty())a.handIndex=(a.handIndex+5)%static_cast<int>(hand.size()); }
  else if(key==SDLK_UP) a.rowIndex=(a.rowIndex+2)%3;
  else if(key==SDLK_DOWN) a.rowIndex=(a.rowIndex+1)%3;
  else if(key==SDLK_l) {
    beginLeaderChoice(a);
  }
  else if(key==SDLK_BACKSPACE) {
    if(gwent::pass(a.game,0)) { a.audio.play("pass");a.status=tr(a,"you_passed"); if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
  }
  else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) doHumanPlay(a);
}
void handleKey(App& a,SDL_Keycode key,bool& running) {
  if(key==SDLK_HOME) {
    if(a.screen==Screen::Match)saveCurrentMatch(a);
    running=false;return;
  }
  if(a.screen==Screen::Match&&actionAnimationLocked(a,SDL_GetTicks()))return;
  if(a.screen==Screen::Match&&!a.activeNotification.empty())return;
  if(a.screen==Screen::Match&&(a.leaderSelectionMode||a.leaderRevealOpen||a.passConfirmationOpen||a.pauseOpen)) { handleMatchKey(a,key);return; }
  if(a.cardDetailsOpen) {
    if(key==SDLK_i||key==SDLK_ESCAPE||key==SDLK_BACKSPACE||key==SDLK_z)a.cardDetailsOpen=false;
    return;
  }
  if(key==SDLK_i&&(a.screen==Screen::DeckBuilder||a.screen==Screen::Match)) { a.cardDetailsOpen=true;a.audio.play("open");return; }
  if(key==SDLK_v) { a.audio.setEnabled(!a.audio.enabled());if(a.audio.enabled())a.audio.play("ui_card");return; }
  if(key==SDLK_m) { a.musicEnabled=!a.musicEnabled;a.audio.setMusicEnabled(a.musicEnabled);saveSettings(a);return; }
  if(key==SDLK_n) { a.notificationsEnabled=!a.notificationsEnabled;saveSettings(a);return; }
  if(a.screen==Screen::DeckSelect) {
    if(key==SDLK_ESCAPE) { openDeckEditor(a); return; }
    if(key==SDLK_z) { a.language=a.language==Language::Polish?Language::English:Language::Polish;saveSettings(a);return; }
    if(key==SDLK_r) { loadSavedMatch(a);return; }
    if(key==SDLK_b) { openDeckEditor(a);return; }
    if(key==SDLK_p) { a.userDeckSelected=false;return; }
    if(key==SDLK_PAGEUP||key==SDLK_LEFTBRACKET||key==SDLK_PAGEDOWN||key==SDLK_RIGHTBRACKET) {
      const bool previous=key==SDLK_PAGEUP||key==SDLK_LEFTBRACKET;
      a.deckSlot=previous?(a.deckSlot<=1?3:a.deckSlot-1):(a.deckSlot>=3?1:a.deckSlot+1);
      a.userDeckSelected=gwent::loadUserDeck(userDeckPath(a,a.deckSlot),a.editDeck);return;
    }
    if(key==SDLK_LEFT||key==SDLK_a)a.deckIndex=(a.deckIndex+recipes().size()-1)%recipes().size();
    else if(key==SDLK_RIGHT||key==SDLK_d)a.deckIndex=(a.deckIndex+1)%recipes().size();
    else if(key==SDLK_l) { a.language=a.language==Language::Polish?Language::English:Language::Polish;saveSettings(a); }
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x)startMatch(a);
  } else if(a.screen==Screen::DeckBuilder) {
    if(key==SDLK_ESCAPE||key==SDLK_z) { a.screen=Screen::DeckSelect;return; }
    if(key==SDLK_PAGEUP||key==SDLK_LEFTBRACKET) { a.deckSlot=a.deckSlot<=1?3:a.deckSlot-1;openDeckEditor(a);return; }
    if(key==SDLK_PAGEDOWN||key==SDLK_RIGHTBRACKET) { a.deckSlot=a.deckSlot>=3?1:a.deckSlot+1;openDeckEditor(a);return; }
    if(key==SDLK_f) { changeDeckFaction(a);return; }
    if(key==SDLK_l||key==SDLK_TAB) { a.editingLeader=!a.editingLeader;return; }
    if(key==SDLK_s||key==SDLK_F5) {
      std::string error;
      if(gwent::saveUserDeck(userDeckPath(a,a.deckSlot),a.editDeck,&error)) { a.userDeckSelected=true;a.status=tr(a,"deck_saved"); }
      else a.status=tr(a,"deck_save_error");
      return;
    }
    if(a.editingLeader) {
      const auto leaders=editorLeaders(a);if(leaders.empty())return;
      auto found=std::find_if(leaders.begin(),leaders.end(),[&](const gwent::Card* card){return card->referenceIndex==a.editDeck.leaderReferenceIndex;});
      int index=found==leaders.end()?0:static_cast<int>(found-leaders.begin());
      if(key==SDLK_LEFT||key==SDLK_a)index=(index+static_cast<int>(leaders.size())-1)%static_cast<int>(leaders.size());
      else if(key==SDLK_RIGHT||key==SDLK_d||key==SDLK_x)index=(index+1)%static_cast<int>(leaders.size());
      else if(key==SDLK_RETURN||key==SDLK_KP_ENTER) { a.editingLeader=false;return; }
      a.editDeck.leaderReferenceIndex=leaders[static_cast<std::size_t>(index)]->referenceIndex;return;
    }
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER) { startMatch(a);return; }
    if(key==SDLK_LEFT||key==SDLK_RIGHT) { a.editorPane=1-a.editorPane;a.editorCardIndex=0;return; }
    if(key==SDLK_UP||key==SDLK_w)a.editorCardIndex=std::max(0,a.editorCardIndex-3);
    else if(key==SDLK_DOWN||key==SDLK_j)++a.editorCardIndex;
    const auto allCards=editorCards(a);
    std::vector<const gwent::Card*> cards;
    for(const auto* card:allCards) {
      const int count=deckCopies(a.editDeck,card->referenceIndex);
      if((a.editorPane==0&&count<card->deckCopies)||(a.editorPane==1&&count>0))cards.push_back(card);
    }
    if(cards.empty())return;
    a.editorCardIndex=std::clamp(a.editorCardIndex,0,static_cast<int>(cards.size())-1);
    if(key==SDLK_x || (key==SDLK_RETURN&&a.editorPane==0)) {
      const auto* card=cards[static_cast<std::size_t>(a.editorCardIndex)];const int count=deckCopies(a.editDeck,card->referenceIndex);
      if(a.editorPane==0&&count<card->deckCopies) {
        const auto it=std::find_if(a.editDeck.cardCounts.begin(),a.editDeck.cardCounts.end(),[&](const auto& entry){return entry.first==card->referenceIndex;});
        if(it==a.editDeck.cardCounts.end())a.editDeck.cardCounts.emplace_back(card->referenceIndex,1);else ++it->second;
      } else if(a.editorPane==1) {
        const auto it=std::find_if(a.editDeck.cardCounts.begin(),a.editDeck.cardCounts.end(),[&](const auto& entry){return entry.first==card->referenceIndex;});
        if(it!=a.editDeck.cardCounts.end()) { if(it->second<=1)a.editDeck.cardCounts.erase(it);else --it->second; }
      }
    } else if(key==SDLK_BACKSPACE||key==SDLK_MINUS) {
      const auto* card=cards[static_cast<std::size_t>(a.editorCardIndex)];
      const auto it=std::find_if(a.editDeck.cardCounts.begin(),a.editDeck.cardCounts.end(),[&](const auto& entry){return entry.first==card->referenceIndex;});
      if(it!=a.editDeck.cardCounts.end()) { if(it->second<=1)a.editDeck.cardCounts.erase(it);else --it->second; }
    }
  } else handleMatchKey(a,key);
}
SDL_Keycode controllerKey(const SDL_ControllerButtonEvent& button,Screen screen) {
  switch(button.button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:return SDLK_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:return SDLK_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:return SDLK_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:return SDLK_RIGHT;
    case SDL_CONTROLLER_BUTTON_A:return SDLK_x;
    case SDL_CONTROLLER_BUTTON_B:return SDLK_ESCAPE;
    case SDL_CONTROLLER_BUTTON_X:return screen==Screen::DeckSelect?SDLK_b:SDLK_i;
    case SDL_CONTROLLER_BUTTON_Y:return screen==Screen::Match?SDLK_l:screen==Screen::DeckBuilder?SDLK_f:SDLK_z;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:return screen==Screen::Match?SDLK_UP:SDLK_PAGEUP;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:return screen==Screen::Match?SDLK_DOWN:SDLK_PAGEDOWN;
    case SDL_CONTROLLER_BUTTON_START:return screen==Screen::Match?SDLK_p:SDLK_RETURN;
    default:return SDLK_UNKNOWN;
  }
}
SDL_Keycode normalizeMiyooKey(const App& app,SDL_Keycode key) {
  if(!app.miyooControls)return key;
  switch(key) {
    case SDLK_LCTRL:return SDLK_x;       // A
    case SDLK_LALT:return app.screen==Screen::DeckBuilder&&app.editorPane==1?SDLK_BACKSPACE:SDLK_ESCAPE; // B
    case SDLK_LSHIFT:return app.screen==Screen::DeckSelect?SDLK_b:SDLK_i; // X
    case SDLK_SPACE:
      if(app.screen==Screen::Match)return SDLK_l; // Y = leader
      if(app.screen==Screen::DeckBuilder)return SDLK_f;
      return SDLK_z;                     // Y = language in deck menu
    case SDLK_TAB:return app.screen==Screen::Match?SDLK_UP:SDLK_TAB; // L1
    case SDLK_BACKSPACE:
      if(app.screen==Screen::Match||app.screen==Screen::DeckBuilder)return SDLK_DOWN; // R1
      return SDLK_PAGEDOWN;
    case SDLK_RETURN:return app.screen==Screen::Match?SDLK_p:SDLK_RETURN; // START
    case SDLK_ESCAPE:
      if(app.screen==Screen::Match&&app.cardDetailsOpen)return SDLK_ESCAPE;
      if(app.screen==Screen::Match&&(app.game.phase==gwent::Phase::PlayerTurn||app.game.phase==gwent::Phase::Mulligan||
         app.game.phase==gwent::Phase::RoundSetup||app.game.phase==gwent::Phase::RoundResult||app.game.phase==gwent::Phase::MatchResult))
        return SDLK_BACKQUOTE; // SELECT = guarded pass / advance
      if(app.screen==Screen::DeckBuilder)return SDLK_s; // SELECT saves the deck
      return SDLK_ESCAPE;
    default:return key;                  // D-pad, L2/R2 and system MENU
  }
}
}

int main(int argc,char** argv) {
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER)!=0)return 1;
  SDL_GameControllerEventState(SDL_ENABLE);
  for(int index=0;index<SDL_NumJoysticks();++index)if(SDL_IsGameController(index))SDL_GameControllerOpen(index);
  SDL_Window* window=SDL_CreateWindow("Gwent Classic - Witcher 3",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
  if(!window){SDL_Quit();return 2;}
  SDL_Renderer* presentationRenderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if(!presentationRenderer)presentationRenderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
  if(!presentationRenderer){std::cerr<<"Could not create SDL renderer: "<<SDL_GetError()<<"\n";SDL_DestroyWindow(window);SDL_Quit();return 3;}
  SDL_Renderer* renderer=presentationRenderer;
  SDL_Surface* softwareCanvas=nullptr;
  SDL_Texture* softwareFrame=nullptr;
  SDL_RendererInfo presentationInfo{};
  SDL_GetRendererInfo(presentationRenderer,&presentationInfo);
  const char* compositeSetting=SDL_getenv("GWENT_SOFTWARE_COMPOSITE");
  const bool useSoftwareComposite=(presentationInfo.name&&std::string(presentationInfo.name)=="Miyoo Mini")||
                                  (compositeSetting&&std::string(compositeSetting)=="1");
  if(useSoftwareComposite) {
    softwareCanvas=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
    if(softwareCanvas)renderer=SDL_CreateSoftwareRenderer(softwareCanvas);
    if(renderer!=presentationRenderer)
      softwareFrame=SDL_CreateTexture(presentationRenderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,W,H);
    if(!softwareCanvas||!renderer||renderer==presentationRenderer||!softwareFrame) {
      std::cerr<<"Could not initialize Miyoo-compatible software composition: "<<SDL_GetError()<<"\n";
      if(softwareFrame)SDL_DestroyTexture(softwareFrame);
      if(renderer!=presentationRenderer&&renderer)SDL_DestroyRenderer(renderer);
      if(softwareCanvas)SDL_FreeSurface(softwareCanvas);
      SDL_DestroyRenderer(presentationRenderer);SDL_DestroyWindow(window);SDL_Quit();return 3;
    }
    SDL_SetTextureBlendMode(softwareFrame,SDL_BLENDMODE_NONE);
  }
  SDL_Renderer* drawRenderer=renderer;
  const auto destroyRenderers=[&]() {
    if(softwareFrame)SDL_DestroyTexture(softwareFrame);
    if(drawRenderer!=presentationRenderer)SDL_DestroyRenderer(drawRenderer);
    if(softwareCanvas)SDL_FreeSurface(softwareCanvas);
    SDL_DestroyRenderer(presentationRenderer);
  };
  const auto presentFrame=[&]() {
    if(!softwareCanvas) { SDL_RenderPresent(presentationRenderer);return; }
    SDL_RenderPresent(drawRenderer);
    SDL_UpdateTexture(softwareFrame,nullptr,softwareCanvas->pixels,softwareCanvas->pitch);
    SDL_RenderCopy(presentationRenderer,softwareFrame,nullptr,nullptr);
    SDL_RenderPresent(presentationRenderer);
  };
  SDL_RenderSetLogicalSize(renderer,W,H);
  SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
  App app; bool running=true;
  app.miyooControls=(presentationInfo.name&&std::string(presentationInfo.name)=="Miyoo Mini");
  if(const char* mimic=SDL_getenv("GWENT_MIMIC_MIYOO_INPUT"))app.miyooControls=app.miyooControls||std::string(mimic)=="1";
  if(const char* configured=SDL_getenv("GWENT_ASSET_DIR"))app.assetDirectory=configured;
  if(app.assetDirectory.empty())if(char* base=SDL_GetBasePath()) {
    auto path=std::filesystem::path(base)/"assets"/"gwent-classic";
    if(!std::filesystem::exists(path/"img"/"board.jpg"))path=std::filesystem::current_path()/"assets"/"gwent-classic";
    app.assetDirectory=path.string();
    SDL_free(base);
  }
  app.audio.initialize(app.assetDirectory);
  if(argc>1&&std::string(argv[1])=="--audio-test") {
    std::vector<std::string> failures,musicFailures;
    const bool okay=app.audio.available()&&app.audio.validateAllSfx(failures)&&app.audio.validateBackgroundMusic(musicFailures);
    if(!okay) {
      for(const auto& failure:failures)std::cerr<<"Could not decode SFX: "<<failure<<"\n";
      for(const auto& failure:musicFailures)std::cerr<<"Could not decode background music: "<<failure<<"\n";
      if(!app.audio.available())std::cerr<<"Could not open SDL audio device: "<<SDL_GetError()<<"\n";
    } else {
      app.audio.playMusic("menu");SDL_Delay(80);app.audio.playMusic("match");SDL_Delay(80);
      app.audio.setMusicEnabled(false);
      const bool stopped=!app.audio.musicEnabled();
      app.audio.playMusic("menu");SDL_Delay(40);
      app.audio.setMusicEnabled(true);app.audio.playMusic("menu");SDL_Delay(80);
      app.audio.playMusic("match");SDL_Delay(80);
      if(!stopped||!app.audio.musicEnabled()) {
        std::cerr<<"Background music enable/disable state did not switch correctly.\n";
        app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 5;
      }
      std::cout<<"Decoded all 35 reference SFX and both background tracks; music looping/switch and mute controls initialized.\n";
    }
    app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return okay?0:5;
  }
  if(argc>1&&std::string(argv[1])=="--input-test") {
    const bool startsInDeckMaker=app.screen==Screen::DeckBuilder;
    app.miyooControls=true;
    app.screen=Screen::Match;
    bool okay=startsInDeckMaker&&
              normalizeMiyooKey(app,SDLK_LCTRL)==SDLK_x&&
              normalizeMiyooKey(app,SDLK_LALT)==SDLK_ESCAPE&&
              normalizeMiyooKey(app,SDLK_LSHIFT)==SDLK_i&&
              normalizeMiyooKey(app,SDLK_SPACE)==SDLK_l&&
              normalizeMiyooKey(app,SDLK_TAB)==SDLK_UP&&
              normalizeMiyooKey(app,SDLK_BACKSPACE)==SDLK_DOWN&&
              normalizeMiyooKey(app,SDLK_RETURN)==SDLK_p&&
              normalizeMiyooKey(app,SDLK_ESCAPE)==SDLK_BACKQUOTE&&
              sourceEaseIn(0.0)<0.001&&sourceEaseIn(1.0)>0.999&&
              sourceEase(0.0)<0.001&&sourceEase(0.5)>0.75&&sourceEase(0.5)<0.85&&sourceEase(1.0)>0.999;
    const gwent::Card* lowBase=nullptr;const gwent::Card* highBase=nullptr;
    for(const auto& card:gwent::referenceCardCatalog())if(card.kind==gwent::Kind::Unit&&!card.hero) {
      if(!lowBase||card.power<lowBase->power)lowBase=&card;
      if(!highBase||card.power>highBase->power)highBase=&card;
    }
    if(lowBase&&highBase&&lowBase->referenceIndex!=highBase->referenceIndex) {
      App medicOrder;auto low=*lowBase,high=*highBase;
      low.id="low-base-medic-sort";high.id="high-base-medic-sort";
      low.power=highBase->power+50;high.power=0;
      medicOrder.game.players[0].graveyard={high,low};
      gwent::Card sourceMedic;sourceMedic.kind=gwent::Kind::Unit;sourceMedic.medic=true;
      const auto sortedTargets=legalCardTargets(medicOrder,sourceMedic);
      okay=okay&&sortedTargets.size()==2&&sortedTargets[0]==low.id&&sortedTargets[1]==high.id;
    } else okay=false;
    if(lowBase) {
      App picker;auto valid=*lowBase;valid.id="valid-picker-target";
      auto hero=valid;hero.id="hero-picker-target";hero.hero=true;
      auto nonCombat=valid;nonCombat.id="noncombat-picker-target";nonCombat.row=gwent::Row::None;nonCombat.agile=false;
      picker.game.players[0].graveyard={hero,nonCombat,valid};
      gwent::Card medicPicker;medicPicker.kind=gwent::Kind::Unit;medicPicker.medic=true;
      const auto medicTargets=legalCardTargets(picker,medicPicker);
      okay=okay&&medicTargets==std::vector<std::string>{"valid-picker-target"};
      picker.game.players[0].graveyard.clear();picker.game.players[0].board[0]={hero,nonCombat,valid};
      gwent::Card decoyPicker;decoyPicker.kind=gwent::Kind::Decoy;
      const auto decoyTargets=legalCardTargets(picker,decoyPicker);
      okay=okay&&decoyTargets==std::vector<std::string>{"valid-picker-target"};
      picker.game.players[0].leader.abilities={"emhyr_relentless"};
      picker.game.players[1].graveyard={hero,nonCombat,valid};
      auto relentlessChoices=leaderChoiceCards(picker);
      okay=okay&&relentlessChoices.size()==1&&relentlessChoices.front().id=="valid-picker-target";
      picker.game.players[0].leader.abilities={"eredin_bringer_of_death"};
      picker.game.players[0].graveyard={hero,nonCombat,valid};
      auto bringerChoices=leaderChoiceCards(picker);
      okay=okay&&bringerChoices.size()==1&&bringerChoices.front().id=="valid-picker-target";
    } else okay=false;
    App notificationProbe;
    queueNotification(notificationProbe,"round-start");
    queueNotification(notificationProbe,"me-turn");
    okay=okay&&notificationProbe.activeNotification=="round-start"&&
         notificationProbe.notificationQueue.size()==1&&notificationAlpha(0)==0&&
         notificationAlpha(150)==255&&notificationAlpha(1050)==255&&notificationAlpha(1200)==0;
    notificationProbe.notificationStartedAt=SDL_GetTicks()-notificationDurationMs;
    updateNotification(notificationProbe,SDL_GetTicks());
    okay=okay&&notificationProbe.activeNotification=="me-turn"&&notificationProbe.notificationQueue.empty();
    okay=okay&&endScreenAlpha(0)==0&&endScreenAlpha(150)>=127&&endScreenAlpha(300)==255&&endScreenAlpha(600)==255;
    okay=okay&&sourceAbilityScalePercent(0)==40&&sourceAbilityScalePercent(50)==40&&
         sourceAbilityScalePercent(300)==100&&sourceAbilityScalePercent(350)==100&&
         sourceAbilityScalePercent(600)==80&&sourceAbilityScalePercent(1350)==80&&
         sourceAbilityScalePercent(1600)==40&&sourceAbilityScalePercent(1650)==40;
    okay=okay&&SOURCE_CARD_PREVIEW_MS==1000&&SOURCE_CARD_FLIGHT_MS==499&&
         SOURCE_ROW_PLACEMENT_AUDIO_WAIT_MS==1000&&SOURCE_ABILITY_ANIMATION_MS==1650;
    App timingProbe;gwent::Card moraleTiming;moraleTiming.id="source-timing-morale";
    moraleTiming.kind=gwent::Kind::Unit;moraleTiming.row=gwent::Row::Melee;moraleTiming.abilities={"morale"};
    timingProbe.game.players[0].board[0]={moraleTiming};
    const Uint32 timingStarted=SDL_GetTicks();
    animatePlayedCard(timingProbe,moraleTiming,0,0,1);
    const Uint32 previewFlightDelay=timingProbe.cardFlight.startedAt-timingStarted;
    const Uint32 moraleEffectDelay=timingProbe.cardEffects.empty()?0:timingProbe.cardEffects.front().startedAt-timingStarted;
    playCardSound(timingProbe,moraleTiming,gwent::Game{},0,gwent::Row::Melee);
    const auto placementSound=std::find_if(timingProbe.scheduledSounds.begin(),timingProbe.scheduledSounds.end(),
      [](const App::ScheduledSound& sound){return sound.key=="common_close";});
    okay=okay&&timingProbe.cardFlight.active&&previewFlightDelay>=950&&previewFlightDelay<=1050&&
         moraleEffectDelay>=2450&&moraleEffectDelay<=2550&&placementSound!=timingProbe.scheduledSounds.end()&&
         static_cast<Sint32>(placementSound->startedAt-SDL_GetTicks())>=1400&&
         static_cast<Sint32>(placementSound->startedAt-SDL_GetTicks())<=1550;
    timingProbe.screen=Screen::Match;timingProbe.game.phase=gwent::Phase::PlayerTurn;
    timingProbe.game.players[0].hand={moraleTiming};timingProbe.handIndex=0;
    lockCardActionUntilAnimationsFinish(timingProbe,moraleTiming,gwent::Game{},0,gwent::Row::Melee);
    const Uint32 lockCheckedAt=SDL_GetTicks(),lockUntil=timingProbe.actionAnimationLockUntil;
    bool timingProbeRunning=true;handleKey(timingProbe,SDLK_RIGHT,timingProbeRunning);
    okay=okay&&actionAnimationLocked(timingProbe,lockCheckedAt)&&
         !actionAnimationLocked(timingProbe,lockUntil)&&timingProbe.handIndex==0&&
         static_cast<Sint32>(lockUntil-(timingProbe.cardEffects.front().startedAt+
           timingProbe.cardEffects.front().duration+600))>=0;
    okay=okay&&std::string(sourceWeatherOverlayForRow(0))=="frost"&&
         std::string(sourceWeatherOverlayForRow(1))=="fog"&&
         std::string(sourceWeatherOverlayForRow(2))=="rain";
    App settingsProbe;
    settingsProbe.pauseOpen=true;settingsProbe.pauseIndex=5;
    handleMatchKey(settingsProbe,SDLK_x);
    okay=okay&&!settingsProbe.musicEnabled&&!settingsProbe.audio.musicEnabled()&&
         MATCH_RULES_PANEL_Y>=(H+STAGE_H)/2&&MATCH_RULES_PANEL_Y+MATCH_RULES_PANEL_H<=H&&
         DECK_RULES_PANEL_Y>=STAGE_H&&DECK_RULES_PANEL_Y+DECK_RULES_PANEL_H<=H;
    App movementProbe;
    gwent::Card musterA;musterA.id="muster-flight-a";musterA.kind=gwent::Kind::Unit;musterA.row=gwent::Row::Melee;
    gwent::Card musterB=musterA;musterB.id="muster-flight-b";
    gwent::Game movementBefore;movementBefore.players[1].deck={musterA,musterB};movementProbe.game=movementBefore;
    movementProbe.game.players[1].deck.clear();movementProbe.game.players[1].board[0]={musterA,musterB};
    gwent::Card musterAction;musterAction.id="muster-action";musterAction.muster=true;
    queueCardTransitionFlights(movementProbe,movementBefore,musterAction,1);
    okay=okay&&movementProbe.triggeredCardFlights.size()==2&&
         std::all_of(movementProbe.triggeredCardFlights.begin(),movementProbe.triggeredCardFlights.end(),[](const App::CardFlight& flight) {
           const Sint32 delay=static_cast<Sint32>(flight.startedAt-SDL_GetTicks());
           return flight.destination==App::CardZone::Board&&delay>=4000&&delay<=4250;
         });
    movementProbe.triggeredCardFlights.clear();
    movementBefore={};movementBefore.players[0].board[0]={musterA,musterB};movementProbe.game=movementBefore;
    movementProbe.game.players[0].board[0].clear();movementProbe.game.players[0].graveyard={musterA,musterB};
    gwent::Card scorchAction;scorchAction.kind=gwent::Kind::Scorch;scorchAction.id="scorch-action";
    queueCardTransitionFlights(movementProbe,movementBefore,scorchAction,0);
    okay=okay&&movementProbe.triggeredCardFlights.size()==2&&
         std::all_of(movementProbe.triggeredCardFlights.begin(),movementProbe.triggeredCardFlights.end(),[](const App::CardFlight& flight) {
           const Sint32 delay=static_cast<Sint32>(flight.startedAt-SDL_GetTicks());
           return flight.destination==App::CardZone::Grave&&delay>=2500&&delay<=2750;
         });
    const auto oneCardRow=sourceRowGeometry(1),nineCardRow=sourceRowGeometry(9),tenCardRow=sourceRowGeometry(10),
               twentyCardRow=sourceRowGeometry(20);
    okay=okay&&oneCardRow.firstX>356&&oneCardRow.firstX<358&&nineCardRow.firstX>238&&nineCardRow.firstX<240&&
         nineCardRow.pitch>29&&nineCardRow.pitch<30&&tenCardRow.pitch>26&&tenCardRow.pitch<27&&
         twentyCardRow.pitch>12&&twentyCardRow.pitch<13;
    App rowLayoutProbe;rowLayoutProbe.game.players[0].board[0].resize(9);
    updateRowLayouts(rowLayoutProbe,100);
    rowLayoutProbe.game.players[0].board[0].resize(10);updateRowLayouts(rowLayoutProbe,200);
    const auto rowMidpoint=currentRowGeometry(rowLayoutProbe.rowLayouts[0][0],10,325);
    const auto rowFinished=currentRowGeometry(rowLayoutProbe.rowLayouts[0][0],10,450);
    okay=okay&&rowMidpoint.pitch< nineCardRow.pitch&&rowMidpoint.pitch>tenCardRow.pitch&&
         std::abs(rowFinished.pitch-tenCardRow.pitch)<0.001;
    notificationProbe.notificationsEnabled=false;
    queueNotification(notificationProbe,"op-turn");
    okay=okay&&notificationProbe.activeNotification=="me-turn"&&notificationProbe.notificationQueue.empty();
    App openingNotice;openingNotice.screen=Screen::Match;openingNotice.game.phase=gwent::Phase::OpponentTurn;openingNotice.game.currentPlayer=1;
    queueGameplayNotifications(openingNotice,gwent::Phase::Mulligan,1,0,{{false,false}},{{0,0}});
    okay=okay&&openingNotice.activeNotification=="round-start"&&openingNotice.notificationQueue.size()==1&&
         openingNotice.notificationQueue.front()=="op-turn";
    App passNotice;passNotice.screen=Screen::Match;passNotice.game.phase=gwent::Phase::PlayerTurn;
    passNotice.game.currentPlayer=0;passNotice.game.players[1].passed=true;
    queueGameplayNotifications(passNotice,gwent::Phase::OpponentTurn,1,1,{{false,false}},{{0,0}});
    okay=okay&&passNotice.activeNotification=="op-pass"&&passNotice.notificationQueue.size()==1&&
         passNotice.notificationQueue.front()=="me-turn";
    App resultNotice;resultNotice.screen=Screen::Match;resultNotice.game.phase=gwent::Phase::RoundResult;
    resultNotice.game.players[0].roundWins=1;
    queueGameplayNotifications(resultNotice,gwent::Phase::PlayerTurn,1,0,{{true,true}},{{0,0}});
    okay=okay&&resultNotice.activeNotification=="win-round";
    app.game.phase=gwent::Phase::PlayerTurn;
    handleMatchKey(app,SDLK_BACKQUOTE);
    okay=okay&&app.passConfirmationOpen&&!app.game.players[0].passed;
    app.passConfirmationOpen=false;app.targetMode=true;
    handleMatchKey(app,SDLK_BACKQUOTE);
    okay=okay&&!app.passConfirmationOpen;
    app.targetMode=false;
    app.cardDetailsOpen=true;
    okay=okay&&normalizeMiyooKey(app,SDLK_ESCAPE)==SDLK_ESCAPE;
    app.cardDetailsOpen=false;
    app.game=gwent::Game{};app.game.phase=gwent::Phase::PlayerTurn;
    gwent::Card carouselInput;carouselInput.id="carousel-input";carouselInput.kind=gwent::Kind::Unit;
    app.game.players[0].hand={carouselInput};app.handIndex=0;
    app.targetMode=true;app.selectedTargets={"first-target","second-target"};app.targetIndex=0;
    handleMatchKey(app,SDLK_RIGHT);
    okay=okay&&app.targetIndex==1&&app.carouselMotionDirection==1;
    handleMatchKey(app,SDLK_RIGHT);
    okay=okay&&app.targetIndex==1;
    app.targetMode=false;app.selectedTargets.clear();
    app.pauseOpen=true;app.pauseIndex=3;app.language=Language::Polish;
    handleMatchKey(app,SDLK_x);
    okay=okay&&app.language==Language::English;
    app.pauseOpen=false;
    app.game=gwent::Game{};app.screen=Screen::Match;app.miyooControls=true;
    app.game.phase=gwent::Phase::Mulligan;
    gwent::Card mulliganCard;mulliganCard.id="mulligan-test";mulliganCard.kind=gwent::Kind::Unit;
    gwent::Card replacement;replacement.id="replacement-test";replacement.kind=gwent::Kind::Unit;
    app.game.players[0].hand={mulliganCard};app.game.players[0].deck={replacement};
    app.mulligan.clear();
    handleMatchKey(app,normalizeMiyooKey(app,SDLK_LCTRL));
    okay=okay&&app.mulligan.size()==1;
    handleMatchKey(app,normalizeMiyooKey(app,SDLK_SPACE));
    okay=okay&&app.game.phase==gwent::Phase::PlayerTurn&&app.game.players[0].mulliganDone&&app.mulligan.empty();
    app.game=gwent::Game{};app.game.phase=gwent::Phase::Mulligan;app.mulligan.clear();
    gwent::Card humanSpare;humanSpare.id="mulligan-spare-human";humanSpare.kind=gwent::Kind::Unit;
    gwent::Card opponentSpare;opponentSpare.id="mulligan-spare-opponent";opponentSpare.kind=gwent::Kind::Unit;
    app.game.players[0].hand.push_back(humanSpare);app.game.players[1].hand.push_back(opponentSpare);
    handleMatchKey(app,normalizeMiyooKey(app,SDLK_RETURN));
    okay=okay&&app.game.phase==gwent::Phase::PlayerTurn&&app.game.players[0].mulliganDone&&app.mulligan.empty();
    handleMatchKey(app,SDLK_BACKQUOTE);
    okay=okay&&app.passConfirmationOpen&&!app.game.players[0].passed;
    handleMatchKey(app,SDLK_x);
    okay=okay&&!app.passConfirmationOpen&&app.game.players[0].passed;
    app.game=gwent::Game{};app.screen=Screen::Match;app.miyooControls=true;
    app.game.phase=gwent::Phase::PlayerTurn;app.game.currentPlayer=0;
    gwent::Card medic;medic.id="medic-input";medic.name="Medic";medic.kind=gwent::Kind::Unit;
    medic.row=gwent::Row::Melee;medic.power=3;medic.medic=true;
    gwent::Card agile;agile.id="agile-input";agile.name="Agile unit";agile.kind=gwent::Kind::Unit;
    agile.row=gwent::Row::None;agile.power=7;agile.agile=true;agile.abilities={"agile"};
    app.game.players[0].hand={medic};app.game.players[0].graveyard={agile};
    app.handIndex=0;app.targetIndex=0;app.reviveRowIndex=0;app.targetMode=true;app.selectedTargets={agile.id};
    handleMatchKey(app,SDLK_UP);
    okay=okay&&app.reviveRowIndex==1&&app.game.players[0].graveyard.size()==1;
    handleMatchKey(app,SDLK_x);
    okay=okay&&app.game.players[0].board[1].size()==1&&
         std::any_of(app.game.players[0].board[1].begin(),app.game.players[0].board[1].end(),[](const gwent::Card& card){return card.id=="agile-input";})&&
         std::none_of(app.game.players[0].board[0].begin(),app.game.players[0].board[0].end(),[](const gwent::Card& card){return card.id=="agile-input";});
    if(okay)std::cout<<"Miyoo controls, source-timed action lock/preview/flight/audio/ability effects, mulligan, guarded pass and Agile Medic row selection passed.\n";
    else std::cerr<<"Miyoo controls, action timing/lock, target selection, Agile revival or guarded pass failed.\n";
    app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return okay?0:10;
  }
  if(char* directory=SDL_GetPrefPath("GwentClassic","MiyooMiniPlus")) {
    app.dataDirectory=directory;app.savePath=app.dataDirectory+"match-v1.gws";
    loadSettings(app);
    app.audio.setEnabled(app.sfxEnabled);
    app.audio.setMusicEnabled(app.musicEnabled);
    if(gwent::loadUserDeck(userDeckPath(app,app.deckSlot),app.editDeck))app.userDeckSelected=true;
    else if(!recipes().empty()) { const auto& recipe=recipes().front();app.editDeck={"Deck 1",recipe.faction,recipe.leaderReferenceIndex,recipe.cardCounts};app.userDeckSelected=true; }
    SDL_free(directory);
  }
  app.audio.play("menu_opening");
  if(argc>1 && std::string(argv[1])=="--smoke-test") {
    if(app.screen!=Screen::DeckBuilder) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 11; }
    const auto clearFrame=[&](){SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);};
    const auto saveFrame=[&](const char* path){
      SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
      if(!surface)return false;
      const bool okay=SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,surface->pixels,surface->pitch)==0&&SDL_SaveBMP(surface,path)==0;
      SDL_FreeSurface(surface);return okay;
    };
    clearFrame();
    app.screen=Screen::DeckBuilder;drawDeckBuilder(renderer,app);presentFrame();
    if(argc>3&&!saveFrame(argv[3])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    if(argc>4) {
      app.language=Language::Polish;
      clearFrame();
      const auto& catalog=gwent::referenceCardCatalog();
      const auto ciri=std::find_if(catalog.begin(),catalog.end(),[](const gwent::Card& card){return card.name=="Cirilla Fiona Elen Riannon";});
      if(ciri!=catalog.end())drawCardDetails(renderer,app,*ciri);
      else { app.cardDetailsOpen=true;drawDeckBuilder(renderer,app);app.cardDetailsOpen=false; }
      presentFrame();
      if(!saveFrame(argv[4])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    startMatch(app);
    if(argc>11) {
      app.game.phase=gwent::Phase::Mulligan;
      app.mulligan.clear();
      if(!app.game.players[0].hand.empty())app.mulligan.push_back(app.game.players[0].hand.front().id);
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[11])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    gwent::replaceMulliganCards(app.game,0,{}); gwent::performAiMulligan(app.game,1);
    app.game.phase=gwent::Phase::PlayerTurn;app.game.currentPlayer=0;
    clearFrame();
    drawMatchOnScreen(renderer,app);
    if(argc>2&&!saveFrame(argv[2])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    if(argc>5) {
      app.game.phase=gwent::Phase::PlayerTurn;app.game.currentPlayer=0;
      const auto& hand=app.game.players[0].hand;
      const auto it=std::find_if(hand.begin(),hand.end(),[](const gwent::Card& card){return card.kind==gwent::Kind::Unit&&!card.medic&&!card.spy&&(card.row!=gwent::Row::None||card.agile);});
      if(it==hand.end()) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 5; }
      const int index=static_cast<int>(it-hand.begin()),before=static_cast<int>(hand.size());
      const gwent::Card card=*it;
      const gwent::Row row=card.agile?gwent::Row::Melee:card.row;
      if(!gwent::playCard(app.game,0,card.id,row)) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 5; }
      animatePlayedCard(app,card,0,index,before);
      app.cardFlight.startedAt=SDL_GetTicks()>250?SDL_GetTicks()-250:0;
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[5])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>6) {
      app.screen=Screen::DeckSelect;
      app.language=Language::Polish;
      app.notificationsEnabled=true;
      bool smokeRunning=true;
      handleKey(app,SDLK_n,smokeRunning);
      handleKey(app,SDLK_z,smokeRunning);
      App settingsProbe;settingsProbe.dataDirectory=app.dataDirectory;loadSettings(settingsProbe);
      if(app.language!=Language::English ||
         (!app.dataDirectory.empty() &&
          (settingsProbe.language!=app.language||settingsProbe.notificationsEnabled||settingsProbe.musicEnabled!=app.musicEnabled))) {
        app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 6;
      }
      app.status.clear();
      clearFrame();drawDeckSelect(renderer,app);presentFrame();
      if(!saveFrame(argv[6])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>7) {
      app.screen=Screen::DeckSelect;
      bool smokeRunning=true;
      handleKey(app,SDLK_ESCAPE,smokeRunning);
      if(app.screen!=Screen::DeckBuilder) {
        app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 7;
      }
      clearFrame();drawDeckBuilder(renderer,app);presentFrame();
      if(!saveFrame(argv[7])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    const auto& catalog=gwent::referenceCardCatalog();
    const auto leaderFor=[&](const std::string& ability) {
      const auto it=std::find_if(catalog.begin(),catalog.end(),[&](const gwent::Card& card){
        return card.kind==gwent::Kind::Leader&&std::find(card.abilities.begin(),card.abilities.end(),ability)!=card.abilities.end();
      });
      return it==catalog.end()?gwent::Card{}:*it;
    };
    if(argc>8) {
      app.game=gwent::Game{};app.game.players[0].leader=leaderFor("eredin_king");
      app.game.players[0].leaderAbilityAvailable=true;
      for(const auto& card:catalog)if(card.kind==gwent::Kind::Weather)app.game.players[0].deck.push_back(card);
      app.leaderSelections.clear();app.leaderSelectionMode=false;app.leaderRevealOpen=false;app.cardDetailsOpen=false;
      beginLeaderChoice(app);
      if(!app.leaderSelectionMode||leaderChoiceCards(app).empty()) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 8; }
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[8])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
      handleMatchKey(app,SDLK_RIGHT);handleMatchKey(app,SDLK_x);
      if(app.leaderSelectionMode||app.game.weatherCards.size()!=1||
         app.game.weatherCards[0].card.kind!=gwent::Kind::Weather||app.game.players[0].leaderAbilityAvailable) {
        app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 8;
      }
    }
    if(argc>9) {
      app.game=gwent::Game{};app.game.players[0].leader=leaderFor("emhyr_emperor");
      app.game.players[0].leaderAbilityAvailable=true;
      for(const auto& card:catalog)if(card.kind==gwent::Kind::Unit&&app.game.players[1].hand.size()<4) {
        auto copy=card;copy.id+="-reveal-smoke-"+std::to_string(app.game.players[1].hand.size());
        app.game.players[1].hand.push_back(std::move(copy));
      }
      app.leaderSelections.clear();app.leaderSelectionMode=false;app.leaderRevealOpen=false;app.cardDetailsOpen=false;
      beginLeaderChoice(app);
      if(!app.leaderRevealOpen||app.game.revealedCards[0].size()!=3) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 9; }
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[9])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
      handleMatchKey(app,SDLK_x);
      if(app.leaderRevealOpen) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 9; }
    }
    if(argc>10) {
      app.screen=Screen::Match;app.game=gwent::Game{};app.game.phase=gwent::Phase::PlayerTurn;
      app.pauseOpen=true;app.pauseIndex=3;
      clearFrame();drawMatchOnScreen(renderer,app);
      if(!saveFrame(argv[10])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
      presentFrame();
    }
    if(argc>11) {
      app.screen=Screen::Match;app.game=gwent::Game{};app.game.phase=gwent::Phase::PlayerTurn;
      app.pauseOpen=false;app.language=Language::Polish;app.activeNotification="round-start";
      app.notificationStartedAt=SDL_GetTicks()-75;
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[11])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>13) {
      app.screen=Screen::Match;app.game=gwent::Game{};app.game.phase=gwent::Phase::MatchResult;
      app.game.matchWinner=0;app.game.players[0].score=42;app.game.players[1].score=37;
      app.pauseOpen=false;app.cardDetailsOpen=false;app.activeNotification.clear();
      app.cardFlight.active=false;app.triggeredCardFlights.clear();app.cardEffects.clear();
      app.endScreenStartedAt=SDL_GetTicks()-150;
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[13])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>14) {
      app.screen=Screen::Match;app.game=gwent::Game{};app.game.phase=gwent::Phase::PlayerTurn;
      const auto& effectCard=*std::find_if(gwent::referenceCardCatalog().begin(),gwent::referenceCardCatalog().end(),
        [](const gwent::Card& card){return card.kind==gwent::Kind::Unit&&!card.hero;});
      app.game.players[0].board[0].push_back(effectCard);
      app.game.players[0].board[0].back().id+="-effect-smoke";
      app.cardFlight.active=false;app.triggeredCardFlights.clear();app.cardEffects={{true,"morale",235,143,SDL_GetTicks()-175,1650}};
      app.activeNotification.clear();app.pauseOpen=false;app.cardDetailsOpen=false;
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[14])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>15) {
      app.screen=Screen::Match;app.game=gwent::Game{};app.game.phase=gwent::Phase::PlayerTurn;
      const auto& catalogCards=gwent::referenceCardCatalog();
      const auto sourceUnit=std::find_if(catalogCards.begin(),catalogCards.end(),
        [](const gwent::Card& card){return card.kind==gwent::Kind::Unit&&!card.hero;});
      if(sourceUnit==catalogCards.end()) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 5; }
      for(int i=0;i<10;++i) { auto card=*sourceUnit;card.id+="-row-layout-"+std::to_string(i);app.game.players[0].board[0].push_back(std::move(card)); }
      const auto from=sourceRowGeometry(9),to=sourceRowGeometry(10);auto& layout=app.rowLayouts[0][0];
      layout={true,10,from.firstX,from.pitch,to.firstX,to.pitch,SDL_GetTicks()-125};
      app.cardFlight.active=false;app.triggeredCardFlights.clear();app.cardEffects.clear();app.activeNotification.clear();
      app.pauseOpen=false;app.cardDetailsOpen=false;
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[15])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>17) {
      app.screen=Screen::Match;app.game=gwent::Game{};app.game.phase=gwent::Phase::PlayerTurn;
      app.language=Language::Polish;app.targetMode=true;app.targetIndex=2;app.handIndex=-1;
      app.selectedTargets.clear();
      for(const auto& source:gwent::referenceCardCatalog()) {
        if(source.kind!=gwent::Kind::Unit||source.hero)continue;
        auto card=source;card.id+="-carousel-smoke-"+std::to_string(app.selectedTargets.size());
        app.selectedTargets.push_back(card.id);app.game.players[1].board[0].push_back(std::move(card));
        if(app.selectedTargets.size()==5)break;
      }
      if(app.selectedTargets.size()!=5) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 5; }
      app.cardFlight.active=false;app.triggeredCardFlights.clear();app.cardEffects.clear();app.activeNotification.clear();
      app.pauseOpen=false;app.cardDetailsOpen=false;app.carouselMotionStartedAt=SDL_GetTicks()-75;app.carouselMotionDirection=1;
      clearFrame();drawMatchOnScreen(renderer,app);presentFrame();
      if(!saveFrame(argv[17])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    presentFrame();
    for(auto& image:app.textures)if(image.second)SDL_DestroyTexture(image.second);
    app.audio.shutdown();destroyRenderers(); SDL_DestroyWindow(window); SDL_Quit(); return 0;
  }
  auto observedPhase=app.game.phase;
  int observedRound=app.game.round;
  int observedCurrentPlayer=app.game.currentPlayer;
  std::array<bool,2> observedPassed{{app.game.players[0].passed,app.game.players[1].passed}};
  std::array<int,2> observedRoundWins{{app.game.players[0].roundWins,app.game.players[1].roundWins}};
  while(running) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type==SDL_QUIT)running=false;
      else if(event.type==SDL_KEYDOWN&&!event.key.repeat) {
        if(app.miyooControls&&event.key.keysym.sym==SDLK_ESCAPE) {
          app.selectDownAt=SDL_GetTicks();app.selectLongPressed=false;
        } else handleKey(app,normalizeMiyooKey(app,event.key.keysym.sym),running);
      } else if(event.type==SDL_KEYUP&&app.miyooControls&&event.key.keysym.sym==SDLK_ESCAPE) {
        if(app.selectDownAt&&!app.selectLongPressed)handleKey(app,normalizeMiyooKey(app,event.key.keysym.sym),running);
        app.selectDownAt=0;app.selectLongPressed=false;
      }
      else if(event.type==SDL_CONTROLLERBUTTONDOWN) {
        if(event.cbutton.button==SDL_CONTROLLER_BUTTON_BACK) {
          app.selectDownAt=SDL_GetTicks();app.selectLongPressed=false;
        } else { const auto mapped=controllerKey(event.cbutton,app.screen);if(mapped!=SDLK_UNKNOWN)handleKey(app,mapped,running); }
      } else if(event.type==SDL_CONTROLLERBUTTONUP&&event.cbutton.button==SDL_CONTROLLER_BUTTON_BACK) {
        if(app.selectDownAt&&!app.selectLongPressed&&!actionAnimationLocked(app,SDL_GetTicks())) {
          if(app.cardDetailsOpen)handleKey(app,SDLK_BACKSPACE,running);
          else if(app.screen==Screen::Match&&app.game.phase==gwent::Phase::PlayerTurn)app.passConfirmationOpen=true;
          else if(app.screen==Screen::Match&&app.game.phase==gwent::Phase::RoundResult)handleMatchKey(app,SDLK_RETURN);
          else if(app.screen==Screen::Match&&app.game.phase==gwent::Phase::MatchResult)app.screen=Screen::DeckSelect;
          else if(app.screen==Screen::DeckBuilder)handleKey(app,SDLK_s,running);
          else handleKey(app,SDLK_ESCAPE,running);
        }
        app.selectDownAt=0;app.selectLongPressed=false;
      }
    }
    if(app.selectDownAt&&!app.selectLongPressed&&app.activeNotification.empty()&&
       (app.screen==Screen::Match||app.screen==Screen::DeckBuilder)&&
       !actionAnimationLocked(app,SDL_GetTicks())&&
       SDL_TICKS_PASSED(SDL_GetTicks(),app.selectDownAt+500)) {
      app.cardDetailsOpen=true;app.selectLongPressed=true;app.audio.play("open");
    }
    if(app.cardFlight.active&&SDL_TICKS_PASSED(SDL_GetTicks(),app.cardFlight.startedAt+499))app.cardFlight.active=false;
    const Uint32 now=SDL_GetTicks();
    app.triggeredCardFlights.erase(std::remove_if(app.triggeredCardFlights.begin(),app.triggeredCardFlights.end(),[&](const App::CardFlight& flight) {
      return SDL_TICKS_PASSED(now,flight.startedAt+499);
    }),app.triggeredCardFlights.end());
    app.cardEffects.erase(std::remove_if(app.cardEffects.begin(),app.cardEffects.end(),[&](const App::CardEffect& effect) {
      return SDL_TICKS_PASSED(now,effect.startedAt+effect.duration);
    }),app.cardEffects.end());
    for(auto& sound:app.scheduledSounds)
      if(SDL_TICKS_PASSED(now,sound.startedAt))app.audio.play(sound.key);
    app.scheduledSounds.erase(std::remove_if(app.scheduledSounds.begin(),app.scheduledSounds.end(),[&](const App::ScheduledSound& sound) {
      return SDL_TICKS_PASSED(now,sound.startedAt);
    }),app.scheduledSounds.end());
    updateWeatherTransitions(app);
    updateRowLayouts(app,now);
    const auto previousPhase=observedPhase;
    if(app.screen==Screen::Match&&app.game.phase!=observedPhase) {
      if(app.game.phase==gwent::Phase::RoundResult||app.game.phase==gwent::Phase::MatchResult)
        app.endScreenStartedAt=now;
      if(app.game.phase==gwent::Phase::Mulligan)app.audio.play("redraw");
      else if(app.game.phase==gwent::Phase::PlayerTurn)app.audio.play("turn_me");
      else if(app.game.phase==gwent::Phase::OpponentTurn)app.audio.play("turn_op");
      else if(app.game.phase==gwent::Phase::RoundResult)app.audio.play(app.game.players[0].score>=app.game.players[1].score?"round_win":"round_lose");
      else if(app.game.phase==gwent::Phase::MatchResult)app.audio.play(app.game.matchWinner==0?"game_win":"game_lose");
      observedPhase=app.game.phase;
    }
    queueGameplayNotifications(app,previousPhase,observedRound,observedCurrentPlayer,
                               observedPassed,observedRoundWins);
    observedRound=app.game.round;observedCurrentPlayer=app.game.currentPlayer;
    observedPassed={app.game.players[0].passed,app.game.players[1].passed};
    observedRoundWins={app.game.players[0].roundWins,app.game.players[1].roundWins};
    updateNotification(app,now);
    if(app.screen==Screen::Match&&app.activeNotification.empty()&&
       !app.leaderSelectionMode&&!app.leaderRevealOpen&&!app.pauseOpen&&!app.passConfirmationOpen&&
       app.game.phase==gwent::Phase::OpponentTurn&&!actionAnimationLocked(app,now)&&SDL_GetTicks()>=app.aiAt)performAi(app);
    app.audio.playMusic(app.screen==Screen::Match?"match":"menu");
    SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
    if(app.screen==Screen::DeckSelect)drawDeckSelect(renderer,app);
    else if(app.screen==Screen::DeckBuilder)drawDeckBuilder(renderer,app);
    else drawMatchOnScreen(renderer,app);
    presentFrame(); SDL_Delay(8);
  }
  for(auto& image:app.textures)if(image.second)SDL_DestroyTexture(image.second);
  app.audio.shutdown();destroyRenderers(); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
