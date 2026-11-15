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
      if(a==0xC4) { if(b==0x85||b==0x84)base='A'; else if(b==0x87||b==0x86)base='C'; else if(b==0x99||b==0x98)base='E'; }
      if(a==0xC5) { if(b==0x82||b==0x81)base='L'; else if(b==0x84||b==0x83)base='N'; else if(b==0x9B||b==0x9A)base='S'; else if(b==0xBA||b==0xB9||b==0xBC||b==0xBB)base='Z'; }
      if(a==0xC3 && (b==0xB3||b==0x93))base='O';
      out.push_back(base); i+=2;
    } else { out.push_back('?'); ++i; }
  }
  return out;
}
void color(SDL_Renderer* r,Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
void box(SDL_Renderer* r,int x,int y,int w,int h,Color c) { color(r,c); SDL_Rect q{x,y,w,h}; SDL_RenderFillRect(r,&q); }
void frame(SDL_Renderer* r,int x,int y,int w,int h,Color c) { color(r,c); SDL_Rect q{x,y,w,h}; SDL_RenderDrawRect(r,&q); }
void text(SDL_Renderer* r,int x,int y,const std::string& value,Color c,int scale=1) {
  color(r,c); const std::string s=ascii(value); int cx=x;
  for(char ch:s) {
    const auto it=font().find(ch); const Glyph g=it==font().end()?font().at('?'):it->second;
    for(int row=0;row<7;++row) for(int col=0;col<5;++col) if(g[row]&(1u<<(4-col))) {
      SDL_Rect p{cx+col*scale,y+row*scale,scale,scale}; SDL_RenderFillRect(r,&p);
    }
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

enum class Screen { Title, DeckSelect, DeckBuilder, Match };
enum class Language { Polish, English };
struct App {
  struct CardFlight {
    bool active{false};
    gwent::Card card;
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
  Screen screen{Screen::Title};
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
  std::vector<std::string> leaderSelections;
  CardFlight cardFlight;
  CardEffect cardEffect;
  std::array<std::array<bool,3>,2> weatherFrom{};
  std::array<std::array<bool,3>,2> weatherTo{};
  std::array<std::array<Uint32,3>,2> weatherChangedAt{};
  std::array<std::array<bool,3>,2> weatherTransition{};
  std::array<std::array<std::string,3>,2> weatherOverlay{};
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
};
void animatePlayedCard(App& a,const gwent::Card& card,int player,int handIndex,int handSizeBefore);
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
  if(key=="choose")return pl?"WYBIERZ TALIE":"CHOOSE A DECK";
  if(key=="opponent")return pl?"PRZECIWNIK":"OPPONENT";
  if(key=="opponent_short")return pl?"RYWAL":"OPPONENT";
  if(key=="ai")return pl?"PRZECIWNIK AI":"AI OPPONENT";
  if(key=="language")return pl?"JEZYK":"LANGUAGE";
  if(key=="instructions")return pl?"LEWO/PRAWO - TALIA":"LEFT/RIGHT - DECK";
  if(key=="confirm")return pl?"ENTER - GRA   Z - JEZYK":"ENTER - PLAY   Z - LANGUAGE";
  if(key=="mulligan")return pl?"WYBIERZ DO 2 KART DO WYMIANY":"CHOOSE UP TO 2 CARDS TO MULLIGAN";
  if(key=="mulligan_controls")return pl?"A: WYBIERZ   START: ZATWIERDZ / POMIN":"A: MARK CARD   START: CONFIRM / SKIP";
  if(key=="mulligan_details")return pl?"X: POWIEKSZ KARTE   LEWO/PRAWO: WYBOR":"X: ENLARGE CARD   LEFT/RIGHT: SELECT";
  if(key=="choose_first")return pl?"SCOIA'TAEL WYBIERA KOLEJKE: LEWO/PRAWO, ENTER":"SCOIA'TAEL CHOOSES FIRST: LEFT/RIGHT, ENTER";
  if(key=="human_first")return pl?"ZACZYNASZ TY":"YOU START";
  if(key=="ai_first")return pl?"ZACZYNA PRZECIWNIK":"OPPONENT STARTS";
  if(key=="skellige_revival")return pl?"SKELLIGE: WYBIERZ JEDNOSTKE Z CMENTARZA":"SKELLIGE: CHOOSE A UNIT FROM THE GRAVEYARD";
  if(key=="skellige_agile")return pl?"ZWINNA: GORA/DOL WYBIERA RZAD":"AGILE: UP/DOWN SELECTS ROW";
  if(key=="melee")return pl?"WALKA WRECZ":"MELEE";
  if(key=="ranged")return pl?"DYSTANSOWY":"RANGED";
  if(key=="controls")return pl?"A GRAJ  B WROC  X PODGLAD  Y LIDER":"A PLAY  B BACK  X INSPECT  Y LEADER";
  if(key=="pass")return pl?"SELECT - PASUJ (POTWIERDZ)":"SELECT - PASS (CONFIRM)";
  if(key=="round")return pl?"RUNDA":"ROUND";
  if(key=="your_turn")return pl?"TWOJ RUCH":"YOUR TURN";
  if(key=="ai_turn")return pl?"RUCH PRZECIWNIKA":"OPPONENT TURN";
  if(key=="continue")return pl?"ENTER - DALEJ":"ENTER - CONTINUE";
  if(key=="defeat")return pl?"PRZEGRANA":"DEFEAT";
  if(key=="victory")return pl?"ZWYCIESTWO":"VICTORY";
  if(key=="target")return pl?"WYBIERZ CEL ENTEREM, ESC ANULUJE":"CHOOSE TARGET WITH ENTER, ESC CANCELS";
  if(key=="resume")return pl?"R - WCZYTAJ ZAPIS   ESC - WSTECZ":"R - RESUME SAVE   ESC - BACK";
  if(key=="save_controls")return pl?"SELECT/S - ZAPISZ   START/O - WCZYTAJ":"SELECT/S - SAVE   START/O - LOAD";
  if(key=="deck_builder")return pl?"BUDOWNICZY Talii":"DECK BUILDER";
  if(key=="deck_mode")return pl?"KARTY":"CARDS";
  if(key=="leader_mode")return pl?"LIDER":"LEADER";
  if(key=="deck_count")return pl?"KARTY W TALII":"CARDS IN DECK";
  if(key=="deck_valid")return pl?"TALIA POPRAWNA":"DECK IS VALID";
  if(key=="deck_invalid")return pl?"TALIA NIEPOPRAWNA":"DECK IS INVALID";
  if(key=="deck_saved")return pl?"ZAPISANO TALIE":"DECK SAVED";
  if(key=="deck_slot")return pl?"SLOT TALII":"DECK SLOT";
  if(key=="deck_controls")return pl?"GORA/DOL: KARTA  A: DODAJ  B: USUN":"UP/DOWN: CARD  A: ADD  B: REMOVE";
  if(key=="deck_controls2")return pl?"L: LIDER  F: FRAKCJA  SELECT: ZAPISZ":"L: LEADER  F: FACTION  SELECT: SAVE";
  if(key=="deck_need_cards")return pl?"MIN. 22 JEDNOSTKI / MAX. 10 SPECJALNYCH":"MIN 22 UNITS / MAX 10 SPECIALS";
  if(key=="deck_custom")return pl?"WLASNA TALIA":"CUSTOM DECK";
  if(key=="card_details")return pl?"SZCZEGOLY KARTY":"CARD DETAILS";
  if(key=="leader_reveal")return pl?"KARTY Z REKI PRZECIWNIKA":"OPPONENT'S REVEALED CARDS";
  if(key=="leader_continue")return pl?"A / ENTER - KONTYNUUJ":"A / ENTER - CONTINUE";
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
  if(key=="pause_exit")return pl?"WROC DO MENU TALII":"RETURN TO DECK MENU";
  if(key=="pause_controls")return pl?"GORA/DOL: WYBOR  A: ZMIEN  B: WROC":"UP/DOWN: SELECT  A: CHANGE  B: BACK";
  if(key=="pass_confirm")return pl?"SPASOWAC TE RUNDE?":"PASS THIS ROUND?";
  if(key=="pass_confirm_controls")return pl?"A: POTWIERDZ  B: ANULUJ":"A: CONFIRM  B: CANCEL";
  if(key=="on")return "ON";
  if(key=="off")return "OFF";
  if(key=="power")return pl?"SILA":"POWER";
  if(key=="row")return pl?"RZAD":"ROW";
  if(key=="special")return pl?"SPECJALNA":"SPECIAL";
  if(key=="siege")return pl?"OBLEZENIE":"SIEGE";
  if(key=="close_card")return pl?"B / ESC - ZAMKNIJ":"B / ESC - CLOSE";
  if(key=="no_ability")return pl?"BRAK ZDOLNOSCI SPECJALNEJ":"NO SPECIAL ABILITY";
  if(key=="saved")return pl?"ZAPISANO":"GAME SAVED";
  if(key=="loaded")return pl?"WCZYTANO ZAPIS":"SAVE LOADED";
  if(key=="no_save")return pl?"BRAK POPRAWNEGO ZAPISU":"NO VALID SAVE FOUND";
  return key;
}
bool loadSavedMatch(App& a) {
  gwent::Game restored;std::string error;
  if(a.savePath.empty()||!gwent::loadGame(a.savePath,restored,&error)) { a.status=tr(a,"no_save");return false; }
  a.game=std::move(restored);a.screen=Screen::Match;a.handIndex=0;a.rowIndex=0;a.targetIndex=0;
  a.targetMode=false;a.selectedTargets.clear();a.mulligan.clear();a.status=tr(a,"loaded");
  if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
  return true;
}
void saveCurrentMatch(App& a) {
  std::string error;
  if(a.screen!=Screen::Match||a.savePath.empty()||!gwent::saveGame(a.savePath,a.game,&error))a.status="SAVE ERROR";
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
  if(settings)settings<<static_cast<int>(a.language)<<' '<<static_cast<int>(a.sfxEnabled)<<'\n';
}
void loadSettings(App& a) {
  if(a.dataDirectory.empty())return;
  std::ifstream settings(a.dataDirectory+"settings-v1.cfg");
  int language=0,sfx=1;
  if(!(settings>>language))return;
  if(language>=0&&language<=1)a.language=static_cast<Language>(language);
  if(!(settings>>sfx))return;
  int legacySfx=0;
  if(sfx>=0&&sfx<=2&&settings>>legacySfx)sfx=legacySfx;
  a.sfxEnabled=sfx!=0;
}
void playCardSound(App& a,const gwent::Card& card) {
  std::string key;
  if(!card.abilities.empty()) {
    key=card.abilities.back();
    if(key=="cerys")key="muster";
    if(key.rfind("scorch_",0)==0)key="scorch";
  }
  if(key.empty()||!std::filesystem::exists(std::filesystem::path(a.assetDirectory)/"sfx"/(key+".mp3")))
    key=card.row==gwent::Row::Ranged?"common_ranged":card.row==gwent::Row::Siege?"common_siege":"common_close";
  a.audio.play(key);
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
  if(recipes().size()<2) { a.status="REFERENCE DECKS MISSING"; return; }
  gwent::Card humanLeader=recipeLeader(a.deckIndex);std::vector<gwent::Card> humanDeck;
  if(a.userDeckSelected) {
    if(!gwent::materializeUserDeck(a.editDeck,humanDeck,humanLeader)) { a.status=tr(a,"deck_invalid");return; }
  } else humanDeck=gwent::makeReferenceDeck(recipes()[a.deckIndex]);
  const auto seed=static_cast<std::uint64_t>(SDL_GetTicks())^SDL_GetPerformanceCounter();
  auto opponent=gwent::makeReferenceOpponentDeck(seed);
  if(opponent.sourceDeckIndex<0||opponent.cards.empty()||opponent.leader.kind!=gwent::Kind::Leader) {
    a.status="REFERENCE OPPONENT DECK INVALID";return;
  }
  gwent::beginMatchWithLeaders(a.game,std::move(humanDeck),humanLeader,
      std::move(opponent.cards),opponent.leader,seed,-1);
  a.mulligan.clear(); a.handIndex=0; a.rowIndex=0; a.screen=Screen::Match;
  a.audio.play("game_opening");
  a.status=tr(a,"mulligan");
}
std::vector<std::string> legalCardTargets(const App& a,const gwent::Card& card) {
  std::vector<std::string> ids;
  const auto& p=a.game.players[0];
  if(card.kind==gwent::Kind::Decoy) {
    for(const auto& row:p.board) for(const auto& target:row)
      if(target.kind==gwent::Kind::Unit && !target.hero) ids.push_back(target.id);
  } else if(card.kind==gwent::Kind::Unit && card.medic) {
    for(const auto& target:p.graveyard) if(target.kind==gwent::Kind::Unit && !target.hero) ids.push_back(target.id);
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
    for(const auto& card:grave)if(card.kind==gwent::Kind::Unit)cards.push_back(card);
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
  const bool okay=gwent::activateLeader(a.game,0,gwent::Row::None,a.leaderSelections);
  a.leaderSelectionMode=false;
  a.targetIndex=0;
  if(okay) {
    a.status="LEADER ABILITY USED";
    if(!a.game.revealedCards[0].empty())a.leaderRevealOpen=true;
    if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
  } else a.status="LEADER CANNOT BE USED";
  a.leaderSelections.clear();
}
void beginLeaderChoice(App& a) {
  a.leaderSelections.clear();
  const auto ability=activeLeaderAbility(a.game.players[0].leader);
  const bool needsChoice=ability=="emhyr_relentless"||ability=="eredin_bringer_of_death"||
                         ability=="eredin_destroyer"||ability=="eredin_king";
  if(!needsChoice) {
    if(gwent::activateLeader(a.game,0)) {
      a.status="LEADER ABILITY USED";
      if(!a.game.revealedCards[0].empty())a.leaderRevealOpen=true;
      if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
    } else a.status="LEADER CANNOT BE USED";
    return;
  }
  if((ability=="eredin_destroyer"&&(a.game.players[0].hand.size()<2||a.game.players[0].deck.empty()))||
     leaderChoiceCards(a).empty()) { a.status="LEADER HAS NO VALID TARGET";return; }
  a.leaderSelectionMode=true;
  a.targetIndex=0;
  a.audio.play("open");
  a.status=leaderChoicePrompt(a);
}
gwent::Card findById(const std::vector<gwent::Card>& cards,const std::string& id) {
  const auto it=std::find_if(cards.begin(),cards.end(),[&](const gwent::Card& c){return c.id==id;});
  return it==cards.end()?gwent::Card{}:*it;
}
std::string targetName(const gwent::Game& game,const std::string& id) {
  for(const auto& row:game.players[0].board) for(const auto& card:row) if(card.id==id)return card.name;
  for(const auto& card:game.players[0].graveyard) if(card.id==id)return card.name;
  return "?";
}
void doHumanPlay(App& a) {
  auto& hand=a.game.players[0].hand;
  if(a.handIndex<0 || a.handIndex>=static_cast<int>(hand.size())) { a.status="NO CARD SELECTED"; return; }
  const auto card=hand[a.handIndex];
  const auto targets=legalCardTargets(a,card);
  if(card.kind==gwent::Kind::Decoy && targets.empty()) { a.status="DECOY NEEDS A UNIT"; return; }
  if(!targets.empty()) {
    a.targetMode=true; a.selectedTargets=targets; a.targetIndex=0; a.reviveRowIndex=0; a.audio.play("open");a.status=tr(a,"target"); return;
  }
  const int handSizeBefore=static_cast<int>(hand.size());
  if(gwent::playCard(a.game,0,card.id,static_cast<gwent::Row>(a.rowIndex))) {
    animatePlayedCard(a,card,0,a.handIndex,handSizeBefore);
    playCardSound(a,card);
    a.status=card.name+" - PLAYED"; a.handIndex=std::min(a.handIndex,static_cast<int>(a.game.players[0].hand.size())-1);
  } else a.status="ILLEGAL PLAY";
}
void performAi(App& a) {
  if(a.game.phase!=gwent::Phase::OpponentTurn) return;
  const auto action=gwent::chooseAiAction(a.game,1);
  bool success=false;
  if(action.pass) { success=gwent::pass(a.game,1);a.audio.play("pass");a.status=tr(a,"ai_turn")+" - PASS"; }
  else if(action.activateLeader) {
    success=gwent::activateLeader(a.game,1,action.target,action.leaderSelection); a.status="OPPONENT USED LEADER";
  } else {
    const auto& aiHand=a.game.players[1].hand;
    const auto it=std::find_if(aiHand.begin(),aiHand.end(),[&](const gwent::Card& item){return item.id==action.cardId;});
    const int cardIndex=static_cast<int>(it-aiHand.begin());
    const int handSizeBefore=static_cast<int>(aiHand.size());
    const auto card=it==aiHand.end()?gwent::Card{}:*it;
    success=gwent::playCard(a.game,1,action.cardId,action.target,action.selectedCardId);
    if(success) { animatePlayedCard(a,card,1,cardIndex,handSizeBefore);playCardSound(a,card); }
    a.status="OPPONENT PLAYED "+card.name;
  }
  if(!success) {
    // Do not disguise an invalid ported AI action as a voluntary source-game
    // pass. Keep the turn state intact and make the failed action visible so
    // the mismatched rule can be corrected instead of silently changing play.
    a.status="AI ACTION REJECTED - SOURCE PARITY ERROR";
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
  const std::string name=shortName(card.name,std::max(3,(w-6)/6));
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
    flight.owner=owner;flight.row=row;flight.toX=237+index*29;
    flight.toY=owner==0?143+row*45:5+(2-row)*45;found=true;
  }
  if(!found) {
    const auto it=std::find_if(a.game.weatherCards.begin(),a.game.weatherCards.end(),[&](const gwent::ActiveWeather& item){return item.card.id==card.id;});
    if(it!=a.game.weatherCards.end()) {
      const int index=static_cast<int>(it-a.game.weatherCards.begin());
      flight.owner=-1;flight.row=-1;flight.toX=47+index*29;flight.toY=149;found=true;
    } else if(card.kind==gwent::Kind::CommanderHorn) {
      // Source places Commander's Horn in its row's special slot. In this
      // renderer that slot is represented by the horn marker at row's left.
      int targetRow=player==0?a.rowIndex:0;
      if(player==1)for(int row=0;row<3;++row)if(a.game.horn[player][row]){targetRow=row;break;}
      flight.owner=player;flight.row=targetRow;flight.toX=190;flight.toY=player==0?143+targetRow*45:5+(2-targetRow)*45;found=true;
    }
  }
  if(found) {
    flight.active=true;flight.startedAt=SDL_GetTicks();a.cardFlight=std::move(flight);
    // Source Card.animate() attaches the matching anim_<ability> artwork to
    // the card: 50 ms setup, 300 ms fade in, 1000 ms hold, 300 ms fade out.
    // Scorch has its own 50 + 1300 + 300 ms sequence and cover sizing.
    std::string effect;
    for(const auto* ability:{"horn","spy","muster","morale","bond","cerys"}) {
      if(std::find(card.abilities.begin(),card.abilities.end(),ability)!=card.abilities.end()) { effect=ability;break; }
    }
    if(effect=="cerys")effect="muster";
    if(effect.empty()&&card.kind==gwent::Kind::CommanderHorn)effect="horn";
    if(effect.empty()&&card.spy)effect="spy";
    if(!effect.empty()) {
      a.cardEffect.active=true;a.cardEffect.icon=effect;a.cardEffect.x=flight.toX;a.cardEffect.y=flight.toY;
      a.cardEffect.startedAt=SDL_GetTicks()+499;a.cardEffect.duration=1650;
    }
  }
}
void drawCardFlight(SDL_Renderer* r,const App& a) {
  const auto& flight=a.cardFlight;if(!flight.active)return;
  const float t=std::clamp(static_cast<float>(SDL_GetTicks()-flight.startedAt)/499.0f,0.0f,1.0f);
  const float eased=t*t*(3.0f-2.0f*t);
  const int x=static_cast<int>(std::lround(flight.fromX+(flight.toX-flight.fromX)*eased));
  const int y=static_cast<int>(std::lround(flight.fromY+(flight.toY-flight.fromY)*eased));
  drawSourceCard(r,a,x,y,flight.card,true);
}
void drawCardEffect(SDL_Renderer* r,const App& a) {
  const auto& effect=a.cardEffect;if(!effect.active)return;
  const Uint32 now=SDL_GetTicks();
  if(SDL_TICKS_PASSED(now,effect.startedAt+effect.duration))return;
  const Uint32 elapsed=now-effect.startedAt;
  Uint8 alpha=255;
  if(elapsed<50)alpha=0;
  else if(elapsed<350)alpha=static_cast<Uint8>(std::clamp((elapsed-50)*255/300,0u,255u));
  else if(elapsed>=effect.duration-300)alpha=static_cast<Uint8>(std::clamp((effect.duration-elapsed)*255/300,0u,255u));
  float size=1.0f;
  if(effect.icon!="scorch")size=elapsed<350?1.0f:elapsed<1350?0.8f:0.4f;
  const int width=std::max(1,static_cast<int>(std::lround(28*size)));
  const int height=std::max(1,static_cast<int>(std::lround(43*size)));
  drawImageAlpha(r,a,"img/icons/anim_"+effect.icon+".png",effect.x+(28-width)/2,effect.y+(43-height)/2,width,height,alpha);
}
std::vector<std::string> wrapDetailText(const std::string& value,std::size_t limit);
void drawCardPreview(SDL_Renderer* r,const App& a,const gwent::Card& card) {
  // CSS reference: .card-preview art at (80.3vw, 11.95vw), 16.1vw x 30.4vw.
  // Give the enlarged card enough pixels for its printed name and flavor text
  // to survive the Miyoo's 640x480 output without competing with board labels.
  drawImage(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),489,15,127,241);
  const std::string& title=gwent::referenceCardAbilityTitle(card.referenceIndex);
  const std::string& description=gwent::referenceCardDescription(card.referenceIndex);
  if(title.empty()&&description.empty())return;
  box(r,435,266,186,90,Color{12,12,12,255});
  box(r,435,266,186,1,Color{255,255,255,110});
  box(r,435,355,186,1,Color{255,255,255,87});
  std::string icon;
  if(card.kind!=gwent::Kind::Leader&&!card.abilities.empty()&&
     !(card.hero&&card.abilities.size()==1&&card.abilities.front()=="hero")) {
    icon=card.abilities.back();
    if(icon=="cerys")icon="muster";
    if(icon.rfind("avenger",0)==0)icon="avenger";
    if(icon.rfind("scorch",0)==0)icon="scorch";
    if(icon=="hero")icon.clear();
  } else if(card.agile)icon="agile";
  if(!icon.empty())drawImage(r,a,"img/icons/card_ability_"+icon+".png",441,271,14,14);
  const auto titleLines=wrapDetailText(title,26);
  int textY=271;
  for(const auto& line:titleLines) { text(r,459,textY,line,Color{255,220,153},1);textY+=9; }
  // The board preview is only 186x90 px on a 640x480 screen. Render fewer
  // characters at 2x scale so the visible rules can be read on Miyoo; the
  // fullscreen card inspector remains the place for the complete description.
  const auto lines=wrapDetailText(description,14);
  constexpr int descriptionLineHeight=14;
  const std::size_t visibleLines=std::min<std::size_t>(4,lines.size());
  for(std::size_t i=0;i<visibleLines;++i) {
    const auto& line=lines[i];
    const int lineY=textY+static_cast<int>(i)*descriptionLineHeight;
    if(lineY+14>354)break;
    const int lineX=435+(186-textWidth(line,2))/2;
    text(r,lineX+1,lineY+1,line,Color{0,0,0,255},2);
    text(r,lineX,lineY,line,Color{255,243,219,255},2);
  }
  if(lines.size()>visibleLines) {
    const int hintY=textY+static_cast<int>(visibleLines)*descriptionLineHeight;
    const std::string hint=a.language==Language::Polish?"I - PELNY OPIS":"I - FULL TEXT";
    text(r,441,hintY,hint,Color{205,182,137,255},1);
  }
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
void drawCardDetails(SDL_Renderer* r,const App& a,const gwent::Card& card) {
  box(r,0,0,W,H,Color{0,0,0,255});
  drawImage(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),8,8,246,465);
  frame(r,7,7,248,467,gold);
  box(r,264,23,354,434,Color{20,20,20,250});
  frame(r,264,23,354,434,Color{143,108,54});
  text(r,280,40,tr(a,"card_details"),gold,2);
  const auto nameLines=wrapDetailText(card.name,27);
  int y=68;
  for(const auto& line:nameLines) { text(r,280,y,line,paper,2);y+=18; }
  std::string row=card.row==gwent::Row::Ranged?tr(a,"ranged"):card.row==gwent::Row::Siege?tr(a,"siege"):card.row==gwent::Row::Melee?tr(a,"melee"):tr(a,"special");
  text(r,280,y+4,tr(a,"power")+" "+std::to_string(card.power)+"  "+tr(a,"row")+" "+row,Color{235,207,164},1);
  y+=28;
  const auto& title=gwent::referenceCardAbilityTitle(card.referenceIndex);
  const auto& description=gwent::referenceCardDescription(card.referenceIndex);
  if(!title.empty()) {
    for(const auto& line:wrapDetailText(title,27)) { text(r,280,y,line,gold,2);y+=18; }
    y+=4;
  }
  const auto descriptionLines=wrapDetailText(description.empty()?tr(a,"no_ability"):description,28);
  for(const auto& line:descriptionLines) {
    if(y>414)break;
    text(r,280,y,line,Color{242,234,218},2);y+=18;
  }
  text(r,280,433,tr(a,"close_card"),Color{185,185,185},1);
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
  const int start=left+67,shown=std::min(9,static_cast<int>(cards.size()));
  for(int i=0;i<shown;++i) {
    const auto& card=cards[static_cast<std::size_t>(i)];
    if(a.cardFlight.active&&a.cardFlight.owner==owner&&a.cardFlight.row==row&&a.cardFlight.card.id==card.id)continue;
    drawSourceCard(r,a,start+i*29,y,card);
  }
  if(cards.size()>static_cast<std::size_t>(shown))text(r,left+width-15,y+15,"+",paper,1);
  if(a.weatherTransition[owner][row]||a.game.weather[row]) {
    const std::string& overlay=a.weatherOverlay[owner][row].empty()?std::string("frost"):a.weatherOverlay[owner][row];
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
    if(active)for(const auto& weather:a.game.weatherCards)for(const auto& ability:weather.card.abilities)
      if(ability=="rain"||ability=="fog"||ability=="frost")a.weatherOverlay[owner][row]=ability;
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
void drawTitle(SDL_Renderer* r,const App& a) {
  drawImage(r,a,"img/board.jpg",0,0,W,H);box(r,0,0,W,H,Color{13,19,20,205});box(r,0,0,W,5,gold);box(r,0,H-5,W,5,gold);
  box(r,54,54,532,372,panel); frame(r,54,54,532,372,gold); frame(r,61,61,518,358,Color{95,83,58});
  text(r,134,103,tr(a,"title"),gold,3);
  text(r,132,151,"THE WITCHER 3 - CLASSIC GWENT",paper,1);
  text(r,116,177,"A SINGLE PLAYER PORT FOR MIYOO MINI PLUS",muted,1);
  box(r,118,231,404,2,gold);
  text(r,202,266,tr(a,"start"),paper,2);
  text(r,164,309,"D-PAD / ARROWS TO MOVE",muted,1);
  text(r,174,332,"X / ENTER TO SELECT",muted,1);
  text(r,250,389,"1.0 - PORT IN PROGRESS",gold,1);
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
  text(r,104,115,"LEADER: "+(custom?([&](){gwent::Card c;gwent::referenceLeader(a.editDeck.leaderReferenceIndex,c);return c.name;})():leader.name),muted,1);
  text(r,104,139,"CARDS: "+std::to_string(custom?gwent::makeReferenceDeck(a.editDeck.cardCounts).size():gwent::makeReferenceDeck(selected).size()),muted,1);
  box(r,73,174,493,1,Color{65,72,67});
  text(r,74,193,tr(a,"opponent"),muted,1);
  text(r,74,215,factionName(opponent.faction,a.language==Language::Polish)+" / "+opponentLeader.name,paper,1);
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
    "Draw a card from your deck whenever you win a round.",
    "Wins any round that ends in a draw.",
    "Decides who takes first turn.",
    "Keeps a random Unit Card out after each round.",
    "2 random cards from the graveyard are placed on the battlefield at the start of the third round."
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
      drawImage(r,a,"img/lg/"+gwent::referenceCardImage(card->referenceIndex),cx,cy,63,118);
      drawImage(r,a,"img/icons/preview_count.png",cx+45,cy+96,16,16);
      const auto countLabel=std::to_string(count);
      centeredTextInBox(r,cx+45,cy+96,16,16,countLabel,Color{95,73,35},1);
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
  if(!a.audio.enabled()) { color(r,gold);SDL_RenderDrawLine(r,321,290,337,306); }
  if(!shownCards.empty()) {
    const auto& selected=*shownCards[static_cast<std::size_t>(cursor)];
    drawCardPreview(r,a,selected);
    if(a.cardDetailsOpen)drawCardDetails(r,a,selected);
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
    if(owner==0)text(r,91,statsY+27,"GERALT",paper,1);
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
    if(!(a.cardFlight.active&&a.cardFlight.row==-1&&a.cardFlight.card.id==active.card.id))drawSourceCard(r,a,weatherX,149,active.card);
    weatherX+=29;
  }
  const auto drawPile=[&](int owner,bool grave,int y) {
    const auto& pile=grave?a.game.players[owner].graveyard:a.game.players[owner].deck;
    const int x=grave?514:572;
    if(grave&&!pile.empty())drawImage(r,a,"img/sm/"+gwent::referenceCardImage(pile.back().referenceIndex),x,y,28,43);
    else drawImage(r,a,"img/icons/deck_back_"+std::string(factionKey(a.game.players[owner].faction))+".jpg",x,y,28,43);
    const std::string count=std::to_string(pile.size());
    centeredTextInBox(r,x,y+29,28,13,count,paper,1);
  };
  drawPile(1,true,23);drawPile(1,false,23);drawPile(0,true,275);drawPile(0,false,275);
  box(r,107,295,34,15,Color{20,20,20,230});frame(r,107,295,34,15,Color{143,108,54});text(r,112,299,"PASS",paper,1);
  if(me.passed)text(r,110,282,"PASSED",paper,1);
  if(op.passed)text(r,110,80,"PASSED",paper,1);
  drawImage(r,a,"img/icons/ui_music.png",90,313,16,16);drawImage(r,a,"img/icons/ui_sfx.png",110,313,16,16);drawImage(r,a,"img/icons/ui_notification.png",128,313,16,16);
  if(!a.audio.enabled()) { color(r,gold);SDL_RenderDrawLine(r,110,313,126,329); }
  const int count=static_cast<int>(me.hand.size());
  if(count) {
    const int start=std::clamp(a.handIndex-5,0,std::max(0,count-11));
    const int shown=std::min(11,count-start);
    for(int i=0;i<shown;++i) {
      const int index=start+i,x=194+i*29;
      const bool marked=std::find(a.mulligan.begin(),a.mulligan.end(),me.hand[index].id)!=a.mulligan.end();
      drawSourceCard(r,a,x,277,me.hand[index],index==a.handIndex);
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
    text(r,170,26,tr(a,"skellige_revival"),gold,1);
    if(!candidates.empty()) {
      const int selected=std::clamp(a.targetIndex,0,static_cast<int>(candidates.size())-1);
      const int start=std::clamp(selected-4,0,std::max(0,static_cast<int>(candidates.size())-9));
      for(int i=start;i<std::min(static_cast<int>(candidates.size()),start+9);++i) {
        const int x=100+(i-start)*31;
        const auto* card=candidates[static_cast<std::size_t>(i)];
        drawSourceCard(r,a,x,115,*card,i==selected);
      }
      const auto* selectedCard=candidates[static_cast<std::size_t>(selected)];
      drawImage(r,a,"img/lg/"+gwent::referenceCardImage(selectedCard->referenceIndex),300,56,103,195);
      text(r,170,247,selectedCard->name+"  "+std::to_string(selectedCard->power),paper,1);
      text(r,170,270,selectedCard->agile?tr(a,"skellige_agile")+" "+tr(a,a.rowIndex==0?"melee":"ranged"):"ENTER - RETURN",muted,1);
    }
  }
  if(a.game.phase==gwent::Phase::RoundResult || a.game.phase==gwent::Phase::MatchResult) {
    box(r,0,0,W,STAGE_H,Color{10,10,10,242});
    const bool done=a.game.phase==gwent::Phase::MatchResult;
    const bool won=a.game.matchWinner==0;
    drawImage(r,a,std::string("img/icons/end_")+(done?(won?"win":"lose"):(won?"win":"lose"))+".png",210,21,220,102);
    text(r,213,139,done?tr(a,won?"victory":"defeat"):"ROUND COMPLETE",gold,2);
    text(r,220,174,"YOU "+std::to_string(me.score)+"  -  "+std::to_string(op.score)+" OPPONENT",paper,1);
    text(r,265,217,tr(a,"continue"),muted,1);
  }
  const bool showHand=a.game.phase!=gwent::Phase::RoundResult&&a.game.phase!=gwent::Phase::MatchResult;
  if(!me.hand.empty()&&showHand) {
    const int selected=std::clamp(a.handIndex,0,static_cast<int>(me.hand.size())-1);
    drawCardPreview(r,a,me.hand[static_cast<std::size_t>(selected)]);
  }
  if(a.cardFlight.active)drawCardFlight(r,a);
  drawCardEffect(r,a);
  if(a.cardDetailsOpen&&!me.hand.empty()&&showHand) {
    const int selected=std::clamp(a.handIndex,0,static_cast<int>(me.hand.size())-1);
    drawCardDetails(r,a,me.hand[static_cast<std::size_t>(selected)]);
  }
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
      text(r,28,25,leaderChoicePrompt(a),gold,2);
      if(activeLeaderAbility(me.leader)=="eredin_destroyer")
        text(r,28,49,tr(a,"leader_selected")+" "+std::to_string(a.leaderSelections.size())+"/2",paper,1);
      const int selected=std::clamp(a.targetIndex,0,static_cast<int>(cards.size())-1);
      const int start=std::clamp(selected-3,0,std::max(0,static_cast<int>(cards.size())-8));
      const int shown=std::min(8,static_cast<int>(cards.size())-start);
      for(int i=0;i<shown;++i) {
        const auto& card=cards[static_cast<std::size_t>(start+i)];
        const int x=26+i*61;
        drawImage(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),x,92,47,77);
        if(start+i==selected)frame(r,x-2,90,51,81,gold);
      }
      const auto& card=cards[static_cast<std::size_t>(selected)];
      drawImage(r,a,"img/lg/"+gwent::referenceCardImage(card.referenceIndex),521,77,96,168);
      frame(r,520,76,98,170,gold);
      text(r,28,207,card.name+"  "+std::to_string(card.power),paper,1);
      if(!a.leaderSelections.empty()) {
        std::string chosen;
        for(const auto& id:a.leaderSelections) {
          const auto found=std::find_if(me.hand.begin(),me.hand.end(),[&](const gwent::Card& c){return c.id==id;});
          if(found!=me.hand.end()) { if(!chosen.empty())chosen+=" / ";chosen+=found->name; }
        }
        text(r,28,229,shortName(chosen,69),Color{235,207,164},1);
      }
      text(r,28,286,tr(a,"leader_controls"),muted,1);
      text(r,28,305,tr(a,"leader_cancel"),muted,1);
    }
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
      const auto names=wrapDetailText(cards[i].name,16);
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
    const std::array<std::string,6> options={
      tr(a,"pause_resume"),tr(a,"pause_save"),tr(a,"pause_load"),
      tr(a,"pause_language")+": "+(a.language==Language::Polish?"POLSKI":"ENGLISH"),
      tr(a,"pause_sfx")+": "+tr(a,a.sfxEnabled?"on":"off"),tr(a,"pause_exit")
    };
    for(std::size_t i=0;i<options.size();++i) {
      const int y=76+static_cast<int>(i)*31;
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
  SDL_RenderSetViewport(r,nullptr);
}
void handleMatchKey(App& a,SDL_Keycode key) {
  if(a.pauseOpen) {
    constexpr int optionCount=6;
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
        case 5:saveCurrentMatch(a);a.pauseOpen=false;a.screen=Screen::DeckSelect;break;
      }
    }
    return;
  }
  if(a.passConfirmationOpen) {
    if(key==SDLK_ESCAPE||key==SDLK_z||key==SDLK_BACKSPACE) { a.passConfirmationOpen=false;return; }
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE) {
      a.passConfirmationOpen=false;
      if(gwent::pass(a.game,0)) { a.audio.play("pass");a.status="YOU PASSED";if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
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
    if(cards.empty()) { a.leaderSelectionMode=false;a.leaderSelections.clear();a.status="LEADER HAS NO VALID TARGET";return; }
    if(key==SDLK_ESCAPE||key==SDLK_z||key==SDLK_BACKSPACE) {
      a.leaderSelectionMode=false;a.leaderSelections.clear();a.targetIndex=0;a.status=tr(a,"your_turn");return;
    }
    if(key==SDLK_LEFT||key==SDLK_a) { a.targetIndex=(a.targetIndex+static_cast<int>(cards.size())-1)%static_cast<int>(cards.size());a.audio.play("ui_card"); }
    else if(key==SDLK_RIGHT||key==SDLK_d) { a.targetIndex=(a.targetIndex+1)%static_cast<int>(cards.size());a.audio.play("ui_card"); }
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE) {
      a.leaderSelections.push_back(cards[static_cast<std::size_t>(a.targetIndex)].id);
      if(activeLeaderAbility(a.game.players[0].leader)=="eredin_destroyer"&&a.leaderSelections.size()<3) {
        a.targetIndex=0;a.status=leaderChoicePrompt(a);a.audio.play("ui_card");
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
      if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
    }
    return;
  }
  if(a.game.phase==gwent::Phase::RoundSetup) {
    std::vector<const gwent::Card*> candidates;
    for(const auto& card:a.game.players[0].graveyard)if(std::find(a.game.skelligeAgileReviveIds.begin(),a.game.skelligeAgileReviveIds.end(),card.id)!=a.game.skelligeAgileReviveIds.end())candidates.push_back(&card);
    if(candidates.empty())return;
    a.targetIndex=std::clamp(a.targetIndex,0,static_cast<int>(candidates.size())-1);
    if(key==SDLK_LEFT||key==SDLK_a)a.targetIndex=(a.targetIndex+static_cast<int>(candidates.size())-1)%static_cast<int>(candidates.size());
    else if(key==SDLK_RIGHT||key==SDLK_d)a.targetIndex=(a.targetIndex+1)%static_cast<int>(candidates.size());
    else if((key==SDLK_UP||key==SDLK_DOWN)&&candidates[static_cast<std::size_t>(a.targetIndex)]->agile)a.rowIndex=1-a.rowIndex;
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) {
      const auto* card=candidates[static_cast<std::size_t>(a.targetIndex)];
      const auto row=card->agile?static_cast<gwent::Row>(a.rowIndex):gwent::Row::None;
      if(gwent::chooseSkelligeRevive(a.game,0,card->id,row)) {
        a.status="SKELLIGE REVIVED: "+card->name; a.targetIndex=0;
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
      const auto opponentMulligan=gwent::chooseAiMulligan(a.game,1);
      if(gwent::replaceMulliganCards(a.game,0,a.mulligan) && gwent::replaceMulliganCards(a.game,1,opponentMulligan)) {
        a.audio.play("game_start");a.audio.play("round1_start");
        a.mulligan.clear(); a.status=tr(a,"your_turn");
        if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+450;
      } else a.status="MULLIGAN ERROR";
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
      else if(key==SDLK_UP||key==SDLK_LEFT)a.targetIndex=(a.targetIndex+static_cast<int>(a.selectedTargets.size())-1)%static_cast<int>(a.selectedTargets.size());
      else if(key==SDLK_DOWN||key==SDLK_RIGHT)a.targetIndex=(a.targetIndex+1)%static_cast<int>(a.selectedTargets.size());
    }
    else if(key==SDLK_RETURN||key==SDLK_x) {
      if(!a.selectedTargets.empty()&&a.handIndex>=0&&a.handIndex<static_cast<int>(hand.size())) {
        const auto card=hand[a.handIndex];
        const int handSizeBefore=static_cast<int>(hand.size());
        const auto& selectedId=a.selectedTargets[static_cast<std::size_t>(a.targetIndex)];
        const auto revived=std::find_if(a.game.players[0].graveyard.begin(),a.game.players[0].graveyard.end(),[&](const gwent::Card& c){return c.id==selectedId;});
        const auto playRow=card.medic&&revived!=a.game.players[0].graveyard.end()&&revived->agile?
                           static_cast<gwent::Row>(a.reviveRowIndex):static_cast<gwent::Row>(a.rowIndex);
        if(gwent::playCard(a.game,0,card.id,playRow,selectedId)) {
          animatePlayedCard(a,card,0,a.handIndex,handSizeBefore);
          playCardSound(a,card);
          a.status=card.name+" - PLAYED"; a.handIndex=std::min(a.handIndex,static_cast<int>(a.game.players[0].hand.size())-1);
        } else a.status="ILLEGAL TARGET";
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
    if(gwent::pass(a.game,0)) { a.audio.play("pass");a.status="YOU PASSED"; if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
  }
  else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) doHumanPlay(a);
}
void handleKey(App& a,SDL_Keycode key,bool& running) {
  if(key==SDLK_HOME) {
    if(a.screen==Screen::Match)saveCurrentMatch(a);
    running=false;return;
  }
  if(a.screen==Screen::Match&&(a.leaderSelectionMode||a.leaderRevealOpen||a.passConfirmationOpen||a.pauseOpen)) { handleMatchKey(a,key);return; }
  if(a.cardDetailsOpen) {
    if(key==SDLK_i||key==SDLK_ESCAPE||key==SDLK_BACKSPACE||key==SDLK_z)a.cardDetailsOpen=false;
    return;
  }
  if(key==SDLK_i&&(a.screen==Screen::DeckBuilder||a.screen==Screen::Match)) { a.cardDetailsOpen=true;a.audio.play("open");return; }
  if(key==SDLK_v) { a.audio.setEnabled(!a.audio.enabled());if(a.audio.enabled())a.audio.play("ui_card");return; }
  if(key==SDLK_ESCAPE && a.screen==Screen::Title) { running=false; return; }
  if(a.screen==Screen::Title) {
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE)a.screen=Screen::DeckSelect;
  } else if(a.screen==Screen::DeckSelect) {
    if(key==SDLK_ESCAPE) { a.screen=Screen::Title; return; }
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
      else a.status=tr(a,"deck_invalid")+": "+error;
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
      std::cout<<"Decoded all reference SFX and both background music tracks; playback switched successfully.\n";
    }
    app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return okay?0:5;
  }
  if(argc>1&&std::string(argv[1])=="--input-test") {
    app.miyooControls=true;
    app.screen=Screen::Match;
    bool okay=normalizeMiyooKey(app,SDLK_LCTRL)==SDLK_x&&
              normalizeMiyooKey(app,SDLK_LALT)==SDLK_ESCAPE&&
              normalizeMiyooKey(app,SDLK_LSHIFT)==SDLK_i&&
              normalizeMiyooKey(app,SDLK_SPACE)==SDLK_l&&
              normalizeMiyooKey(app,SDLK_TAB)==SDLK_UP&&
              normalizeMiyooKey(app,SDLK_BACKSPACE)==SDLK_DOWN&&
              normalizeMiyooKey(app,SDLK_RETURN)==SDLK_p&&
              normalizeMiyooKey(app,SDLK_ESCAPE)==SDLK_BACKQUOTE;
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
    if(okay)std::cout<<"Miyoo controls, mulligan, guarded pass and Agile Medic row selection passed.\n";
    else std::cerr<<"Miyoo controls, target selection, Agile revival or guarded pass failed.\n";
    app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return okay?0:10;
  }
  if(char* directory=SDL_GetPrefPath("GwentClassic","MiyooMiniPlus")) {
    app.dataDirectory=directory;app.savePath=app.dataDirectory+"match-v1.gws";
    loadSettings(app);
    app.audio.setEnabled(app.sfxEnabled);
    if(gwent::loadUserDeck(userDeckPath(app,app.deckSlot),app.editDeck))app.userDeckSelected=true;
    else if(!recipes().empty()) { const auto& recipe=recipes().front();app.editDeck={"Deck 1",recipe.faction,recipe.leaderReferenceIndex,recipe.cardCounts};app.userDeckSelected=true; }
    SDL_free(directory);
  }
  app.audio.play("menu_opening");
  if(argc>1 && std::string(argv[1])=="--smoke-test") {
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
    gwent::replaceMulliganCards(app.game,0,{}); gwent::replaceMulliganCards(app.game,1,gwent::chooseAiMulligan(app.game,1));
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
      bool smokeRunning=true;
      handleKey(app,SDLK_z,smokeRunning);
      App settingsProbe;settingsProbe.dataDirectory=app.dataDirectory;loadSettings(settingsProbe);
      if(app.language!=Language::English ||
         (!app.dataDirectory.empty() && settingsProbe.language!=app.language)) {
        app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 6;
      }
      app.status.clear();
      clearFrame();drawDeckSelect(renderer,app);presentFrame();
      if(!saveFrame(argv[6])) { app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>7) {
      app.screen=Screen::Title;
      bool smokeRunning=true;
      handleKey(app,SDLK_RETURN,smokeRunning);
      if(app.screen!=Screen::DeckSelect) {
        app.audio.shutdown();destroyRenderers();SDL_DestroyWindow(window);SDL_Quit();return 7;
      }
      app.screen=Screen::Title;
      clearFrame();drawTitle(renderer,app);presentFrame();
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
    presentFrame();
    for(auto& image:app.textures)if(image.second)SDL_DestroyTexture(image.second);
    app.audio.shutdown();destroyRenderers(); SDL_DestroyWindow(window); SDL_Quit(); return 0;
  }
  auto observedPhase=app.game.phase;
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
        if(app.selectDownAt&&!app.selectLongPressed) {
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
    if(app.selectDownAt&&!app.selectLongPressed&&
       (app.screen==Screen::Match||app.screen==Screen::DeckBuilder)&&
       SDL_TICKS_PASSED(SDL_GetTicks(),app.selectDownAt+500)) {
      app.cardDetailsOpen=true;app.selectLongPressed=true;app.audio.play("open");
    }
    if(app.cardFlight.active&&SDL_TICKS_PASSED(SDL_GetTicks(),app.cardFlight.startedAt+499))app.cardFlight.active=false;
    if(app.cardEffect.active&&SDL_TICKS_PASSED(SDL_GetTicks(),app.cardEffect.startedAt+app.cardEffect.duration))app.cardEffect.active=false;
    updateWeatherTransitions(app);
    if(app.screen==Screen::Match && !app.leaderSelectionMode && !app.leaderRevealOpen && !app.pauseOpen && !app.passConfirmationOpen &&
       app.game.phase==gwent::Phase::OpponentTurn && SDL_GetTicks()>=app.aiAt)performAi(app);
    if(app.screen==Screen::Match&&app.game.phase!=observedPhase) {
      if(app.game.phase==gwent::Phase::Mulligan)app.audio.play("redraw");
      else if(app.game.phase==gwent::Phase::PlayerTurn)app.audio.play("turn_me");
      else if(app.game.phase==gwent::Phase::OpponentTurn)app.audio.play("turn_op");
      else if(app.game.phase==gwent::Phase::RoundResult)app.audio.play(app.game.players[0].score>=app.game.players[1].score?"round_win":"round_lose");
      else if(app.game.phase==gwent::Phase::MatchResult)app.audio.play(app.game.matchWinner==0?"game_win":"game_lose");
      observedPhase=app.game.phase;
    }
    app.audio.playMusic(app.screen==Screen::Match?"match":"menu");
    SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
    if(app.screen==Screen::Title)drawTitle(renderer,app);
    else if(app.screen==Screen::DeckSelect)drawDeckSelect(renderer,app);
    else if(app.screen==Screen::DeckBuilder)drawDeckBuilder(renderer,app);
    else drawMatchOnScreen(renderer,app);
    presentFrame(); SDL_Delay(8);
  }
  for(auto& image:app.textures)if(image.second)SDL_DestroyTexture(image.second);
  app.audio.shutdown();destroyRenderers(); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
