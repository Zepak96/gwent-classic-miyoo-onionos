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
  Screen screen{Screen::Title};
  Language language{Language::Polish};
  gwent::AiDifficulty aiDifficulty{gwent::AiDifficulty::Normal};
  std::size_t deckIndex{0};
  int deckSlot{1};
  bool userDeckSelected{false};
  bool editingLeader{false};
  bool cardDetailsOpen{false};
  CardFlight cardFlight;
  Uint32 selectDownAt{0};
  bool selectLongPressed{false};
  int editorPane{0};
  int editorCardIndex{0};
  gwent::UserDeck editDeck;
  gwent::Game game;
  int handIndex{0};
  int rowIndex{0};
  int targetIndex{0};
  bool targetMode{false};
  std::vector<std::string> selectedTargets;
  std::vector<std::string> mulligan;
  std::string savePath;
  std::string dataDirectory;
  std::string assetDirectory;
  mutable std::unordered_map<std::string,SDL_Texture*> textures;
  gwent_port::AudioSystem audio;
  std::string status{"PRESS ENTER"};
  Uint32 aiAt{0};
};
void animatePlayedCard(App& a,const gwent::Card& card,int player,int handIndex,int handSizeBefore);
SDL_Texture* textureFor(SDL_Renderer* renderer,const App& app,const std::string& relative) {
  if(relative.empty()||app.assetDirectory.empty())return nullptr;
  const auto found=app.textures.find(relative);
  if(found!=app.textures.end())return found->second;
  const auto path=std::filesystem::path(app.assetDirectory)/relative;
  int width=0,height=0,channels=0;
  unsigned char* pixels=stbi_load(path.string().c_str(),&width,&height,&channels,STBI_rgb_alpha);
  if(!pixels)return nullptr;
  SDL_Texture* loaded=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,width,height);
  if(loaded) {
    SDL_UpdateTexture(loaded,nullptr,pixels,width*4);
    SDL_SetTextureBlendMode(loaded,SDL_BLENDMODE_BLEND);
  }
  stbi_image_free(pixels);
  app.textures.emplace(relative,loaded);
  return loaded;
}
void drawImage(SDL_Renderer* renderer,const App& app,const std::string& relative,int x,int y,int w,int h) {
  if(auto* image=textureFor(renderer,app,relative)) { SDL_Rect destination{x,y,w,h};SDL_RenderCopy(renderer,image,nullptr,&destination); }
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
  if(key=="difficulty")return pl?"POZIOM AI (GORA/DOL)":"AI LEVEL (UP/DOWN)";
  if(key=="language")return pl?"JEZYK":"LANGUAGE";
  if(key=="instructions")return pl?"LEWO/PRAWO - TALIA":"LEFT/RIGHT - DECK";
  if(key=="confirm")return pl?"ENTER - GRA   Z - JEZYK":"ENTER - PLAY   Z - LANGUAGE";
  if(key=="mulligan")return pl?"WYBIERZ DO 2 KART DO WYMIANY":"CHOOSE UP TO 2 CARDS TO MULLIGAN";
  if(key=="confirm_mulligan")return pl?"SPACJA - ZAZNACZ   ENTER - ZATWIERDZ":"SPACE - MARK   ENTER - CONFIRM";
  if(key=="choose_first")return pl?"SCOIA'TAEL WYBIERA KOLEJKE: LEWO/PRAWO, ENTER":"SCOIA'TAEL CHOOSES FIRST: LEFT/RIGHT, ENTER";
  if(key=="human_first")return pl?"ZACZYNASZ TY":"YOU START";
  if(key=="ai_first")return pl?"ZACZYNA PRZECIWNIK":"OPPONENT STARTS";
  if(key=="skellige_revival")return pl?"SKELLIGE: WYBIERZ JEDNOSTKE Z CMENTARZA":"SKELLIGE: CHOOSE A UNIT FROM THE GRAVEYARD";
  if(key=="skellige_agile")return pl?"ZWINNA: GORA/DOL WYBIERA RZAD":"AGILE: UP/DOWN SELECTS ROW";
  if(key=="melee")return pl?"WALKA WRECZ":"MELEE";
  if(key=="ranged")return pl?"DYSTANSOWY":"RANGED";
  if(key=="controls")return pl?"STRZALKI - RUCH   ENTER/X - GRAJ   L - LIDER":"ARROWS - MOVE   ENTER/X - PLAY   L - LEADER";
  if(key=="pass")return pl?"BACKSPACE - PASUJ   ESC - MENU":"BACKSPACE - PASS   ESC - MENU";
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
std::string difficultyName(gwent::AiDifficulty difficulty) {
  switch(difficulty) {
    case gwent::AiDifficulty::Easy:return "EASY";
    case gwent::AiDifficulty::Hard:return "HARD";
    default:return "NORMAL";
  }
}
void saveSettings(const App& a) {
  if(a.dataDirectory.empty())return;
  std::ofstream settings(a.dataDirectory+"settings-v1.cfg",std::ios::trunc);
  if(settings)settings<<static_cast<int>(a.language)<<' '<<static_cast<int>(a.aiDifficulty)<<'\n';
}
void loadSettings(App& a) {
  if(a.dataDirectory.empty())return;
  std::ifstream settings(a.dataDirectory+"settings-v1.cfg");
  int language=0,difficulty=1;
  if(!(settings>>language>>difficulty))return;
  if(language>=0&&language<=1)a.language=static_cast<Language>(language);
  if(difficulty>=0&&difficulty<=2)a.aiDifficulty=static_cast<gwent::AiDifficulty>(difficulty);
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
  const std::size_t aiIndex=(a.deckIndex+1)%recipes().size();
  gwent::Card humanLeader=recipeLeader(a.deckIndex);std::vector<gwent::Card> humanDeck;
  if(a.userDeckSelected) {
    if(!gwent::materializeUserDeck(a.editDeck,humanDeck,humanLeader)) { a.status=tr(a,"deck_invalid");return; }
  } else humanDeck=gwent::makeReferenceDeck(recipes()[a.deckIndex]);
  gwent::Card aiLeader=recipeLeader(aiIndex);
  gwent::beginMatchWithLeaders(a.game,std::move(humanDeck),humanLeader,gwent::makeReferenceDeck(recipes()[aiIndex]),aiLeader,SDL_GetTicks(),-1);
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
    a.targetMode=true; a.selectedTargets=targets; a.targetIndex=0; a.audio.play("open");a.status=tr(a,"target"); return;
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
  const auto action=gwent::chooseAiAction(a.game,1,a.aiDifficulty);
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
  if(!success) { gwent::pass(a.game,1); a.status="OPPONENT PASSED (RECOVERY)"; }
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
    if(it==a.game.weatherCards.end())return;
    const int index=static_cast<int>(it-a.game.weatherCards.begin());
    flight.owner=-1;flight.row=-1;flight.toX=47+index*29;flight.toY=149;found=true;
  }
  if(found) { flight.active=true;flight.startedAt=SDL_GetTicks();a.cardFlight=std::move(flight); }
}
void drawCardFlight(SDL_Renderer* r,const App& a) {
  const auto& flight=a.cardFlight;if(!flight.active)return;
  const float t=std::clamp(static_cast<float>(SDL_GetTicks()-flight.startedAt)/500.0f,0.0f,1.0f);
  const float eased=t*t*(3.0f-2.0f*t);
  const int x=static_cast<int>(std::lround(flight.fromX+(flight.toX-flight.fromX)*eased));
  const int y=static_cast<int>(std::lround(flight.fromY+(flight.toY-flight.fromY)*eased));
  drawSourceCard(r,a,x,y,flight.card,true);
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
  if(a.game.weather[row]) {
    std::string overlay="frost";
    for(const auto& active:a.game.weatherCards)for(const auto& ability:active.card.abilities)
      if(ability=="rain"||ability=="fog"||ability=="frost")overlay=ability;
    drawImage(r,a,"img/icons/overlay_"+overlay+".png",left+19,y,width-20,rowHeight);
  }
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
  text(r,74,251,tr(a,"difficulty"),muted,1);
  text(r,226,251,difficultyName(a.aiDifficulty),gold,1);
  text(r,74,276,tr(a,"language"),muted,1); text(r,226,276,a.language==Language::Polish?"POLSKI":"ENGLISH",gold,1);
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
    }
  } else if(a.game.phase==gwent::Phase::RoundSetup) {
    std::vector<const gwent::Card*> candidates;
    for(const auto& card:me.graveyard)if(card.kind==gwent::Kind::Unit&&!card.hero&&(card.row!=gwent::Row::None||card.agile))candidates.push_back(&card);
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
  if(a.cardDetailsOpen&&!me.hand.empty()&&showHand) {
    const int selected=std::clamp(a.handIndex,0,static_cast<int>(me.hand.size())-1);
    drawCardDetails(r,a,me.hand[static_cast<std::size_t>(selected)]);
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
  if(key==SDLK_ESCAPE) { a.screen=Screen::DeckSelect; a.targetMode=false; return; }
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
    for(const auto& card:a.game.players[0].graveyard)if(card.kind==gwent::Kind::Unit&&!card.hero&&(card.row!=gwent::Row::None||card.agile))candidates.push_back(&card);
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
    else if(key==SDLK_UP||key==SDLK_LEFT) a.targetIndex=(a.targetIndex+static_cast<int>(a.selectedTargets.size())-1)%static_cast<int>(a.selectedTargets.size());
    else if(key==SDLK_DOWN||key==SDLK_RIGHT) a.targetIndex=(a.targetIndex+1)%static_cast<int>(a.selectedTargets.size());
    else if(key==SDLK_RETURN||key==SDLK_x) {
      if(!a.selectedTargets.empty()&&a.handIndex>=0&&a.handIndex<static_cast<int>(hand.size())) {
        const auto card=hand[a.handIndex];
        const int handSizeBefore=static_cast<int>(hand.size());
        if(gwent::playCard(a.game,0,card.id,static_cast<gwent::Row>(a.rowIndex),a.selectedTargets[a.targetIndex])) {
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
  else if(key==SDLK_UP) a.rowIndex=(a.rowIndex+2)%3;
  else if(key==SDLK_DOWN) a.rowIndex=(a.rowIndex+1)%3;
  else if(key==SDLK_l) {
    if(gwent::activateLeader(a.game,0)) { a.status="LEADER ABILITY USED"; if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
    else a.status="LEADER CANNOT BE USED";
  }
  else if(key==SDLK_BACKSPACE) {
    if(gwent::pass(a.game,0)) { a.audio.play("pass");a.status="YOU PASSED"; if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
  }
  else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) doHumanPlay(a);
}
void handleKey(App& a,SDL_Keycode key,bool& running) {
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
    if(key==SDLK_UP||key==SDLK_DOWN) {
      const int direction=key==SDLK_DOWN?1:2;
      a.aiDifficulty=static_cast<gwent::AiDifficulty>((static_cast<int>(a.aiDifficulty)+direction)%3);
      saveSettings(a);return;
    }
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
    case SDL_CONTROLLER_BUTTON_B:return screen==Screen::DeckSelect?SDLK_b:SDLK_BACKSPACE;
    case SDL_CONTROLLER_BUTTON_X:return screen==Screen::DeckSelect?SDLK_b:SDLK_l;
    case SDL_CONTROLLER_BUTTON_Y:return screen==Screen::DeckBuilder?SDLK_f:SDLK_z;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:return (screen==Screen::DeckBuilder||screen==Screen::DeckSelect)?SDLK_PAGEUP:SDLK_v;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:return (screen==Screen::DeckBuilder||screen==Screen::DeckSelect)?SDLK_PAGEDOWN:SDLK_UNKNOWN;
    case SDL_CONTROLLER_BUTTON_START:return SDLK_RETURN;
    default:return SDLK_UNKNOWN;
  }
}
}

int main(int argc,char** argv) {
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER)!=0)return 1;
  SDL_GameControllerEventState(SDL_ENABLE);
  for(int index=0;index<SDL_NumJoysticks();++index)if(SDL_IsGameController(index))SDL_GameControllerOpen(index);
  SDL_Window* window=SDL_CreateWindow("Gwent Classic - Witcher 3",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
  if(!window){SDL_Quit();return 2;}
  SDL_Renderer* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
  if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}
  SDL_RenderSetLogicalSize(renderer,W,H);
  SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
  App app; bool running=true;
  if(const char* configured=SDL_getenv("GWENT_ASSET_DIR"))app.assetDirectory=configured;
  if(app.assetDirectory.empty())if(char* base=SDL_GetBasePath()) {
    auto path=std::filesystem::path(base)/"assets"/"gwent-classic";
    if(!std::filesystem::exists(path/"img"/"board.jpg"))path=std::filesystem::current_path()/"assets"/"gwent-classic";
    app.assetDirectory=path.string();
    SDL_free(base);
  }
  app.audio.initialize(app.assetDirectory);
  if(argc>1&&std::string(argv[1])=="--audio-test") {
    std::vector<std::string> failures;
    const bool okay=app.audio.available()&&app.audio.validateAllSfx(failures);
    if(!okay) {
      for(const auto& failure:failures)std::cerr<<"Could not decode SFX: "<<failure<<"\n";
      if(!app.audio.available())std::cerr<<"Could not open SDL audio device: "<<SDL_GetError()<<"\n";
    } else std::cout<<"Decoded every reference SFX file.\n";
    app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return okay?0:5;
  }
  app.audio.play("menu_opening");
  if(char* directory=SDL_GetPrefPath("GwentClassic","MiyooMiniPlus")) {
    app.dataDirectory=directory;app.savePath=app.dataDirectory+"match-v1.gws";
    loadSettings(app);
    if(gwent::loadUserDeck(userDeckPath(app,app.deckSlot),app.editDeck))app.userDeckSelected=true;
    else if(!recipes().empty()) { const auto& recipe=recipes().front();app.editDeck={"Deck 1",recipe.faction,recipe.leaderReferenceIndex,recipe.cardCounts};app.userDeckSelected=true; }
    SDL_free(directory);
  }
  if(argc>1 && std::string(argv[1])=="--smoke-test") {
    const auto clearFrame=[&](){SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);};
    const auto saveFrame=[&](const char* path){
      SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
      if(!surface)return false;
      const bool okay=SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,surface->pixels,surface->pitch)==0&&SDL_SaveBMP(surface,path)==0;
      SDL_FreeSurface(surface);return okay;
    };
    clearFrame();
    app.screen=Screen::DeckBuilder;drawDeckBuilder(renderer,app);SDL_RenderPresent(renderer);
    if(argc>3&&!saveFrame(argv[3])) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    if(argc>4) {
      clearFrame();
      const auto& catalog=gwent::referenceCardCatalog();
      const auto ciri=std::find_if(catalog.begin(),catalog.end(),[](const gwent::Card& card){return card.name=="Cirilla Fiona Elen Riannon";});
      if(ciri!=catalog.end())drawCardDetails(renderer,app,*ciri);
      else { app.cardDetailsOpen=true;drawDeckBuilder(renderer,app);app.cardDetailsOpen=false; }
      SDL_RenderPresent(renderer);
      if(!saveFrame(argv[4])) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    startMatch(app);
    gwent::replaceMulliganCards(app.game,0,{}); gwent::replaceMulliganCards(app.game,1,gwent::chooseAiMulligan(app.game,1));
    app.game.phase=gwent::Phase::PlayerTurn;app.game.currentPlayer=0;
    clearFrame();
    drawMatchOnScreen(renderer,app);
    if(argc>2&&!saveFrame(argv[2])) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    if(argc>5) {
      app.game.phase=gwent::Phase::PlayerTurn;app.game.currentPlayer=0;
      const auto& hand=app.game.players[0].hand;
      const auto it=std::find_if(hand.begin(),hand.end(),[](const gwent::Card& card){return card.kind==gwent::Kind::Unit&&!card.medic&&!card.spy&&(card.row!=gwent::Row::None||card.agile);});
      if(it==hand.end()) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 5; }
      const int index=static_cast<int>(it-hand.begin()),before=static_cast<int>(hand.size());
      const gwent::Card card=*it;
      const gwent::Row row=card.agile?gwent::Row::Melee:card.row;
      if(!gwent::playCard(app.game,0,card.id,row)) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 5; }
      animatePlayedCard(app,card,0,index,before);
      app.cardFlight.startedAt=SDL_GetTicks()>250?SDL_GetTicks()-250:0;
      clearFrame();drawMatchOnScreen(renderer,app);SDL_RenderPresent(renderer);
      if(!saveFrame(argv[5])) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>6) {
      app.screen=Screen::DeckSelect;
      app.aiDifficulty=gwent::AiDifficulty::Normal;
      bool smokeRunning=true;
      handleKey(app,SDLK_DOWN,smokeRunning);
      App settingsProbe;settingsProbe.dataDirectory=app.dataDirectory;loadSettings(settingsProbe);
      if(app.aiDifficulty!=gwent::AiDifficulty::Hard ||
         (!app.dataDirectory.empty() && settingsProbe.aiDifficulty!=app.aiDifficulty)) {
        app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 6;
      }
      app.status.clear();
      clearFrame();drawDeckSelect(renderer,app);SDL_RenderPresent(renderer);
      if(!saveFrame(argv[6])) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    if(argc>7) {
      app.screen=Screen::Title;
      bool smokeRunning=true;
      handleKey(app,SDLK_RETURN,smokeRunning);
      if(app.screen!=Screen::DeckSelect) {
        app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 7;
      }
      app.screen=Screen::Title;
      clearFrame();drawTitle(renderer,app);SDL_RenderPresent(renderer);
      if(!saveFrame(argv[7])) { app.audio.shutdown();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    }
    SDL_RenderPresent(renderer);
    for(auto& image:app.textures)if(image.second)SDL_DestroyTexture(image.second);
    app.audio.shutdown();SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
  }
  auto observedPhase=app.game.phase;
  while(running) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type==SDL_QUIT)running=false;
      else if(event.type==SDL_KEYDOWN&&!event.key.repeat)handleKey(app,event.key.keysym.sym,running);
      else if(event.type==SDL_CONTROLLERBUTTONDOWN) {
        if(event.cbutton.button==SDL_CONTROLLER_BUTTON_BACK) {
          app.selectDownAt=SDL_GetTicks();app.selectLongPressed=false;
        } else if(event.cbutton.button==SDL_CONTROLLER_BUTTON_START&&app.screen==Screen::Match)loadSavedMatch(app);
        else { const auto mapped=controllerKey(event.cbutton,app.screen);if(mapped!=SDLK_UNKNOWN)handleKey(app,mapped,running); }
      } else if(event.type==SDL_CONTROLLERBUTTONUP&&event.cbutton.button==SDL_CONTROLLER_BUTTON_BACK) {
        if(app.selectDownAt&&!app.selectLongPressed) {
          if(app.cardDetailsOpen)handleKey(app,SDLK_BACKSPACE,running);
          else if(app.screen==Screen::Match||app.screen==Screen::DeckBuilder)handleKey(app,SDLK_s,running);
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
    if(app.cardFlight.active&&SDL_TICKS_PASSED(SDL_GetTicks(),app.cardFlight.startedAt+500))app.cardFlight.active=false;
    if(app.screen==Screen::Match && app.game.phase==gwent::Phase::OpponentTurn && SDL_GetTicks()>=app.aiAt)performAi(app);
    if(app.screen==Screen::Match&&app.game.phase!=observedPhase) {
      if(app.game.phase==gwent::Phase::Mulligan)app.audio.play("redraw");
      else if(app.game.phase==gwent::Phase::PlayerTurn)app.audio.play("turn_me");
      else if(app.game.phase==gwent::Phase::OpponentTurn)app.audio.play("turn_op");
      else if(app.game.phase==gwent::Phase::RoundResult)app.audio.play(app.game.players[0].score>=app.game.players[1].score?"round_win":"round_lose");
      else if(app.game.phase==gwent::Phase::MatchResult)app.audio.play(app.game.matchWinner==0?"game_win":"game_lose");
      observedPhase=app.game.phase;
    }
    SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
    if(app.screen==Screen::Title)drawTitle(renderer,app);
    else if(app.screen==Screen::DeckSelect)drawDeckSelect(renderer,app);
    else if(app.screen==Screen::DeckBuilder)drawDeckBuilder(renderer,app);
    else drawMatchOnScreen(renderer,app);
    SDL_RenderPresent(renderer); SDL_Delay(8);
  }
  for(auto& image:app.textures)if(image.second)SDL_DestroyTexture(image.second);
  app.audio.shutdown();SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
