#include <SDL.h>
#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include "gwent/deck.hpp"
#include "gwent/game.hpp"
#include "gwent/save.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr int W=640,H=480;
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
int textWidth(const std::string& s,int scale=1) { return static_cast<int>(ascii(s).size())*6*scale; }
std::string shortName(std::string s,int n) { if(static_cast<int>(ascii(s).size())>n) s=ascii(s).substr(0,n); return ascii(s); }

enum class Screen { Title, DeckSelect, DeckBuilder, Match };
enum class Language { Polish, English };
struct App {
  Screen screen{Screen::Title};
  Language language{Language::Polish};
  std::size_t deckIndex{0};
  int deckSlot{1};
  bool userDeckSelected{false};
  bool editingLeader{false};
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
  std::string status{"PRESS ENTER"};
  Uint32 aiAt{0};
};
const std::vector<gwent::ReferenceDeck>& recipes() { return gwent::referencePremadeDecks(); }
std::string tr(const App& a,const std::string& key) {
  const bool pl=a.language==Language::Polish;
  if(key=="title")return pl?"GWINT KLASYCZNY":"GWENT CLASSIC";
  if(key=="start")return pl?"ENTER - ROZPOCZNIJ":"ENTER - START";
  if(key=="choose")return pl?"WYBIERZ TALIE":"CHOOSE A DECK";
  if(key=="opponent")return pl?"PRZECIWNIK":"OPPONENT";
  if(key=="ai")return pl?"PRZECIWNIK AI":"AI OPPONENT";
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
std::vector<const gwent::Card*> editorCards(const App& a) {
  std::vector<const gwent::Card*> cards;std::vector<int> seen;
  for(const auto& card:gwent::referenceCardCatalog()) {
    if(card.kind==gwent::Kind::Leader||card.deckCopies<=0||
       (card.faction!=gwent::Faction::Neutral&&card.faction!=a.editDeck.faction)||
       std::find(seen.begin(),seen.end(),card.referenceIndex)!=seen.end())continue;
    seen.push_back(card.referenceIndex);cards.push_back(&card);
  }
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
    a.targetMode=true; a.selectedTargets=targets; a.targetIndex=0; a.status=tr(a,"target"); return;
  }
  if(gwent::playCard(a.game,0,card.id,static_cast<gwent::Row>(a.rowIndex))) {
    a.status=card.name+" - PLAYED"; a.handIndex=std::min(a.handIndex,static_cast<int>(a.game.players[0].hand.size())-1);
  } else a.status="ILLEGAL PLAY";
}
void performAi(App& a) {
  if(a.game.phase!=gwent::Phase::OpponentTurn) return;
  const auto action=gwent::chooseAiAction(a.game,1);
  bool success=false;
  if(action.pass) { success=gwent::pass(a.game,1); a.status=tr(a,"ai_turn")+" - PASS"; }
  else if(action.activateLeader) {
    success=gwent::activateLeader(a.game,1,action.target,action.leaderSelection); a.status="OPPONENT USED LEADER";
  } else {
    const auto card=findById(a.game.players[1].hand,action.cardId);
    success=gwent::playCard(a.game,1,action.cardId,action.target,action.selectedCardId);
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
void drawCardTile(SDL_Renderer* r,int x,int y,int w,int h,const gwent::Card& card,bool selected,bool dim=false) {
  const Color base=card.hero?Color{85,96,107}:card.kind==gwent::Kind::Unit?Color{53,69,63}:Color{78,63,43};
  box(r,x,y,w,h,selected?Color{101,79,44}:base); frame(r,x,y,w,h,selected?paper:gold);
  if(dim) box(r,x+2,y+2,w-4,h-4,Color{20,24,24,130});
  const std::string name=shortName(card.name,std::max(3,(w-6)/6));
  text(r,x+3,y+5,name,card.hero?paper:paper,1);
  if(card.kind==gwent::Kind::Unit) text(r,x+3,y+h-12,std::to_string(card.power)+(card.hero?" H":""),gold,1);
  else text(r,x+3,y+h-12,card.kind==gwent::Kind::Weather?"WEA":"SPC",gold,1);
  if(card.spy) text(r,x+w-10,y+4,"S",blue,1);
  if(card.medic) text(r,x+w-10,y+4,"M",green,1);
}
void drawRow(SDL_Renderer* r,const App& a,int owner,int row,int y) {
  const int x=20+row*200,w=193,h=57;
  const bool player=owner==0;
  const bool chosen=player && a.game.phase==gwent::Phase::PlayerTurn && a.rowIndex==row;
  box(r,x,y,w,h,chosen?Color{43,51,41}:panel); frame(r,x,y,w,h,chosen?gold:Color{65,72,67});
  static const char* labels[]={"MELEE","RANGED","SIEGE"};
  text(r,x+4,y+3,labels[row],muted,1);
  const int total=rowScore(a.game,owner,row);
  const std::string totalText=std::to_string(total);
  text(r,x+w-textWidth(totalText)-5,y+3,totalText,gold,1);
  const auto& cards=a.game.players[owner].board[row];
  const int cardWidth=29,cardHeight=30,gap=2,maxCards=6;
  const int shown=std::min(maxCards,static_cast<int>(cards.size()));
  for(int i=0;i<shown;++i) drawCardTile(r,x+4+i*(cardWidth+gap),y+18,cardWidth,cardHeight,cards[i],false);
  if(static_cast<int>(cards.size())>maxCards) text(r,x+w-25,y+h-12,"+",paper,1);
  if(a.game.weather[row]) { box(r,x+w-31,y+18,27,12,blue); text(r,x+w-29,y+20,"WX",paper,1); }
  if(a.game.horn[owner][row]) { box(r,x+w-31,y+32,27,12,gold); text(r,x+w-29,y+34,"H",ink,1); }
}
void drawTitle(SDL_Renderer* r,const App& a) {
  box(r,0,0,W,H,ink); box(r,0,0,W,5,gold); box(r,0,H-5,W,5,gold);
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
  text(r,74,251,tr(a,"ai"),muted,1);
  text(r,226,251,"CLASSIC",gold,1);
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
  box(r,0,0,W,H,ink);box(r,0,0,W,5,gold);box(r,0,H-5,W,5,gold);
  text(r,20,17,tr(a,"deck_builder"),gold,2);
  text(r,20,45,tr(a,"deck_slot")+" "+std::to_string(a.deckSlot),muted,1);
  text(r,183,45,"FACTION: "+factionName(a.editDeck.faction,a.language==Language::Polish),paper,1);
  text(r,20,68,a.editingLeader?tr(a,"leader_mode"):tr(a,"deck_mode"),gold,1);
  box(r,18,88,604,294,panel);frame(r,18,88,604,294,gold);
  const std::string validation=gwent::validateUserDeck(a.editDeck);
  const auto resolved=gwent::makeReferenceDeck(a.editDeck.cardCounts);
  int units=0;for(const auto& card:resolved)if(card.kind==gwent::Kind::Unit)++units;
  if(a.editingLeader) {
    auto leaders=editorLeaders(a);
    if(!leaders.empty()) {
      const auto selected=std::find_if(leaders.begin(),leaders.end(),[&](const gwent::Card* card){return card->referenceIndex==a.editDeck.leaderReferenceIndex;});
      const int current=selected==leaders.end()?0:static_cast<int>(selected-leaders.begin());
      text(r,42,109,"<  LEFT / RIGHT  >",muted,1);
      const int start=std::clamp(current-3,0,std::max(0,static_cast<int>(leaders.size())-7));
      for(int i=start;i<std::min(static_cast<int>(leaders.size()),start+7);++i) {
        const auto* card=leaders[static_cast<std::size_t>(i)];const int y=138+(i-start)*31;
        box(r,38,y,560,25,i==current?panelLight:panel);frame(r,38,y,560,25,i==current?gold:Color{62,70,65});
        text(r,48,y+8,card->name,paper,1);
        if(i==current)text(r,566,y+8,"*",gold,1);
      }
      text(r,42,366,leaders[static_cast<std::size_t>(current)]->name,gold,1);
    }
  } else {
    const auto cards=editorCards(a);
    if(!cards.empty()) {
      const int current=std::clamp(a.editorCardIndex,0,static_cast<int>(cards.size())-1);
      const int start=std::clamp(current-4,0,std::max(0,static_cast<int>(cards.size())-8));
      for(int i=start;i<std::min(static_cast<int>(cards.size()),start+8);++i) {
        const auto* card=cards[static_cast<std::size_t>(i)];const int y=101+(i-start)*32;
        box(r,29,y,582,27,i==current?panelLight:panel);frame(r,29,y,582,27,i==current?gold:Color{62,70,65});
        text(r,38,y+9,shortName(card->name,39),paper,1);
        const std::string type=card->kind==gwent::Kind::Unit?std::to_string(card->power):"SPC";
        text(r,500,y+9,type,muted,1);
        text(r,562,y+9,std::to_string(deckCopies(a.editDeck,card->referenceIndex))+"/"+std::to_string(card->deckCopies),gold,1);
      }
      const auto* card=cards[static_cast<std::size_t>(current)];
      text(r,35,365,card->name+"  "+std::to_string(deckCopies(a.editDeck,card->referenceIndex))+" / "+std::to_string(card->deckCopies),gold,1);
    }
  }
  text(r,20,394,tr(a,"deck_count")+": "+std::to_string(resolved.size())+"   UNITS: "+std::to_string(units)+" / 22",paper,1);
  text(r,20,414,validation.empty()?tr(a,"deck_valid"):tr(a,"deck_invalid"),validation.empty()?green:Color{196,105,82},1);
  text(r,20,434,tr(a,"deck_controls"),muted,1);
  text(r,20,449,tr(a,"deck_controls2"),muted,1);
}
void drawMatch(SDL_Renderer* r,const App& a) {
  box(r,0,0,W,H,ink);
  box(r,0,0,W,33,panel); box(r,0,0,W,2,gold);
  const auto& me=a.game.players[0]; const auto& op=a.game.players[1];
  text(r,18,12,"GWINT",gold,1);
  text(r,98,12,tr(a,"round")+" "+std::to_string(a.game.round),paper,1);
  text(r,206,12,"YOU "+std::to_string(me.score)+"  -  "+std::to_string(op.score)+" AI",gold,1);
  text(r,468,12,"GEMS "+std::to_string(std::max(0,2-me.roundLosses))+" : "+std::to_string(std::max(0,2-op.roundLosses)),paper,1);
  text(r,18,43,"OPPONENT - "+shortName(op.leader.name,25)+"  "+std::to_string(op.hand.size())+" CARDS",muted,1);
  for(int row=0;row<3;++row) drawRow(r,a,1,row,59);
  box(r,18,124,604,46,panelLight); frame(r,18,124,604,46,Color{64,70,63});
  text(r,31,132,a.game.phase==gwent::Phase::OpponentTurn?tr(a,"ai_turn"):tr(a,"your_turn"),gold,1);
  const std::string hint=a.targetMode?(a.selectedTargets.empty()?"NO TARGETS":"TARGET: "+targetName(a.game,a.selectedTargets[std::clamp(a.targetIndex,0,static_cast<int>(a.selectedTargets.size())-1)])):a.status;
  text(r,31,149,hint,paper,1);
  text(r,350,132,"WEATHER",muted,1);
  const char* weatherNames[]={"FROST","FOG","RAIN"};
  for(int i=0;i<3;++i) { const int x=425+i*58; box(r,x,130,48,17,a.game.weather[i]?blue:panel); frame(r,x,130,48,17,a.game.weather[i]?paper:Color{68,73,65}); text(r,x+3,135,weatherNames[i],a.game.weather[i]?paper:muted,1); }
  text(r,18,178,"YOU - "+shortName(me.leader.name,25)+"  "+std::to_string(me.hand.size())+" CARDS",muted,1);
  for(int row=0;row<3;++row) drawRow(r,a,0,row,192);
  if(a.targetMode && !a.selectedTargets.empty()) {
    std::vector<gwent::Card> targets;
    for(const auto& row:me.board) for(const auto& card:row) if(std::find(a.selectedTargets.begin(),a.selectedTargets.end(),card.id)!=a.selectedTargets.end())targets.push_back(card);
    for(const auto& card:me.graveyard) if(std::find(a.selectedTargets.begin(),a.selectedTargets.end(),card.id)!=a.selectedTargets.end())targets.push_back(card);
    if(a.targetIndex>=0 && a.targetIndex<static_cast<int>(targets.size())) text(r,408,180,"TARGET: "+targets[a.targetIndex].name,gold,1);
  }
  box(r,0,260,W,2,gold);
  const int count=static_cast<int>(me.hand.size());
  if(count) {
    const int start=std::clamp(a.handIndex-5,0,std::max(0,count-10));
    const int shown=std::min(10,count-start);
    for(int i=0;i<shown;++i) {
      const int index=start+i,x=18+i*60;
      const bool marked=std::find(a.mulligan.begin(),a.mulligan.end(),me.hand[index].id)!=a.mulligan.end();
      drawCardTile(r,x,270,56,76,me.hand[index],index==a.handIndex,marked);
      if(marked) { box(r,x+43,272,10,10,gold); text(r,x+45,273,"X",ink,1); }
    }
  }
  if(a.handIndex>=0 && a.handIndex<static_cast<int>(me.hand.size())) {
    const auto& selected=me.hand[a.handIndex];
    text(r,18,350,selected.name+"  POWER "+std::to_string(selected.power),gold,1);
    std::string abilities;
    for(const auto& ability:selected.abilities) {
      if(ability=="hero"||ability=="spy"||ability=="medic"||ability=="morale"||ability=="bond"||ability=="muster"||ability=="horn"||ability=="berserker"||ability.rfind("scorch",0)==0) {
        if(!abilities.empty()) abilities+=" / ";
        abilities+=ability;
      }
    }
    if(!abilities.empty())text(r,18,358,abilities,muted,1);
  }
  if(a.game.phase==gwent::Phase::Mulligan) {
    box(r,0,366,W,45,panel);
    if(a.game.scoiataelChoicePending) {
      text(r,18,373,tr(a,"choose_first"),paper,1);
      text(r,18,390,a.game.firstPlayer==0?tr(a,"human_first"):tr(a,"ai_first"),gold,1);
    } else {
      text(r,18,373,tr(a,"mulligan"),paper,1); text(r,18,390,tr(a,"confirm_mulligan"),gold,1);
    }
  } else if(a.game.phase==gwent::Phase::RoundSetup) {
    std::vector<const gwent::Card*> candidates;
    for(const auto& card:me.graveyard)if(card.kind==gwent::Kind::Unit&&!card.hero&&(card.row!=gwent::Row::None||card.agile))candidates.push_back(&card);
    box(r,65,267,510,151,ink); frame(r,65,267,510,151,gold);
    text(r,82,278,tr(a,"skellige_revival"),gold,1);
    if(!candidates.empty()) {
      const int selected=std::clamp(a.targetIndex,0,static_cast<int>(candidates.size())-1);
      const int start=std::clamp(selected-4,0,std::max(0,static_cast<int>(candidates.size())-9));
      for(int i=start;i<std::min(static_cast<int>(candidates.size()),start+9);++i) {
        const int x=82+(i-start)*53;
        const auto* card=candidates[static_cast<std::size_t>(i)];
        box(r,x,300,49,75,i==selected?panelLight:panel); frame(r,x,300,49,75,i==selected?gold:Color{64,70,63});
        text(r,x+4,307,shortName(card->name,7),paper,1); text(r,x+4,330,"POWER",muted,1); text(r,x+4,346,std::to_string(card->power),gold,1);
      }
      const auto* selectedCard=candidates[static_cast<std::size_t>(selected)];
      text(r,82,385,selectedCard->name+"  "+std::to_string(selectedCard->power),paper,1);
      text(r,352,385,selectedCard->agile?tr(a,"skellige_agile")+" "+tr(a,a.rowIndex==0?"melee":"ranged"):"ENTER - RETURN",muted,1);
    }
  } else {
    text(r,18,372,tr(a,"controls"),paper,1); text(r,18,391,tr(a,"pass"),muted,1);
  }
  if(a.game.phase!=gwent::Phase::RoundSetup) {
    static const char* rows[]={"MELEE","RANGED","SIEGE"};
    text(r,18,415,"ROW: "+std::string(rows[a.rowIndex]),gold,1);
  }
  text(r,18,433,tr(a,"save_controls"),muted,1);
  if(a.game.phase==gwent::Phase::RoundResult || a.game.phase==gwent::Phase::MatchResult) {
    box(r,115,153,410,110,ink); frame(r,115,153,410,110,gold);
    const bool done=a.game.phase==gwent::Phase::MatchResult;
    const bool won=a.game.matchWinner==0;
    text(r,165,176,done?tr(a,won?"victory":"defeat"):"ROUND COMPLETE",gold,2);
    text(r,162,208,"YOU "+std::to_string(me.score)+"  -  "+std::to_string(op.score)+" OPPONENT",paper,1);
    text(r,188,238,tr(a,"continue"),muted,1);
  }
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
    if(key==SDLK_LEFT||key==SDLK_a)a.handIndex=(a.handIndex+static_cast<int>(hand.size())-1)%static_cast<int>(hand.size());
    else if(key==SDLK_RIGHT||key==SDLK_d)a.handIndex=(a.handIndex+1)%static_cast<int>(hand.size());
    else if((key==SDLK_SPACE||key==SDLK_x)&&!hand.empty()) {
      const auto& id=hand[a.handIndex].id; const auto it=std::find(a.mulligan.begin(),a.mulligan.end(),id);
      if(it!=a.mulligan.end())a.mulligan.erase(it); else if(a.mulligan.size()<2)a.mulligan.push_back(id);
    } else if(key==SDLK_BACKSPACE||key==SDLK_z) { if(!a.mulligan.empty())a.mulligan.pop_back(); }
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER) {
      const auto opponentMulligan=gwent::chooseAiMulligan(a.game,1);
      if(gwent::replaceMulliganCards(a.game,0,a.mulligan) && gwent::replaceMulliganCards(a.game,1,opponentMulligan)) {
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
        if(gwent::playCard(a.game,0,card.id,static_cast<gwent::Row>(a.rowIndex),a.selectedTargets[a.targetIndex])) {
          a.status=card.name+" - PLAYED"; a.handIndex=std::min(a.handIndex,static_cast<int>(a.game.players[0].hand.size())-1);
        } else a.status="ILLEGAL TARGET";
      }
      a.targetMode=false; a.selectedTargets.clear();
    }
    return;
  }
  if(key==SDLK_LEFT||key==SDLK_a) { if(!hand.empty())a.handIndex=(a.handIndex+static_cast<int>(hand.size())-1)%static_cast<int>(hand.size()); }
  else if(key==SDLK_RIGHT||key==SDLK_d) { if(!hand.empty())a.handIndex=(a.handIndex+1)%static_cast<int>(hand.size()); }
  else if(key==SDLK_UP) a.rowIndex=(a.rowIndex+2)%3;
  else if(key==SDLK_DOWN) a.rowIndex=(a.rowIndex+1)%3;
  else if(key==SDLK_l) {
    if(gwent::activateLeader(a.game,0)) { a.status="LEADER ABILITY USED"; if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
    else a.status="LEADER CANNOT BE USED";
  }
  else if(key==SDLK_BACKSPACE) {
    if(gwent::pass(a.game,0)) { a.status="YOU PASSED"; if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400; }
  }
  else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) doHumanPlay(a);
}
void handleKey(App& a,SDL_Keycode key,bool& running) {
  if(key==SDLK_ESCAPE && a.screen==Screen::Title) { running=false; return; }
  if(a.screen==Screen::Title) {
    if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x||key==SDLK_SPACE)a.screen=Screen::DeckSelect;
  } else if(a.screen==Screen::DeckSelect) {
    if(key==SDLK_ESCAPE) { a.screen=Screen::Title; return; }
    if(key==SDLK_z) { a.language=a.language==Language::Polish?Language::English:Language::Polish; return; }
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
    else if(key==SDLK_l)a.language=a.language==Language::Polish?Language::English:Language::Polish;
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x)startMatch(a);
  } else if(a.screen==Screen::DeckBuilder) {
    if(key==SDLK_ESCAPE||key==SDLK_z) { a.screen=Screen::DeckSelect;return; }
    if(key==SDLK_PAGEUP||key==SDLK_LEFTBRACKET) { a.deckSlot=a.deckSlot<=1?3:a.deckSlot-1;openDeckEditor(a);return; }
    if(key==SDLK_PAGEDOWN||key==SDLK_RIGHTBRACKET) { a.deckSlot=a.deckSlot>=3?1:a.deckSlot+1;openDeckEditor(a);return; }
    if(key==SDLK_f) { changeDeckFaction(a);return; }
    if(key==SDLK_l||key==SDLK_TAB) { a.editingLeader=!a.editingLeader;return; }
    if(key==SDLK_s||key==SDLK_F5) {
      std::string error;
      if(gwent::saveUserDeck(userDeckPath(a,a.deckSlot),a.editDeck,&error)) { a.userDeckSelected=true;a.screen=Screen::DeckSelect;a.status=tr(a,"deck_saved"); }
      else a.status=tr(a,"deck_invalid")+": "+error;
      return;
    }
    if(a.editingLeader) {
      const auto leaders=editorLeaders(a);if(leaders.empty())return;
      auto found=std::find_if(leaders.begin(),leaders.end(),[&](const gwent::Card* card){return card->referenceIndex==a.editDeck.leaderReferenceIndex;});
      int index=found==leaders.end()?0:static_cast<int>(found-leaders.begin());
      if(key==SDLK_LEFT||key==SDLK_a)index=(index+static_cast<int>(leaders.size())-1)%static_cast<int>(leaders.size());
      else if(key==SDLK_RIGHT||key==SDLK_d||key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x)index=(index+1)%static_cast<int>(leaders.size());
      a.editDeck.leaderReferenceIndex=leaders[static_cast<std::size_t>(index)]->referenceIndex;return;
    }
    const auto cards=editorCards(a);if(cards.empty())return;
    a.editorCardIndex=std::clamp(a.editorCardIndex,0,static_cast<int>(cards.size())-1);
    if(key==SDLK_UP||key==SDLK_w)a.editorCardIndex=(a.editorCardIndex+static_cast<int>(cards.size())-1)%static_cast<int>(cards.size());
    else if(key==SDLK_DOWN||key==SDLK_j)a.editorCardIndex=(a.editorCardIndex+1)%static_cast<int>(cards.size());
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x) {
      const auto* card=cards[static_cast<std::size_t>(a.editorCardIndex)];const int count=deckCopies(a.editDeck,card->referenceIndex);
      if(count<card->deckCopies) {
        const auto it=std::find_if(a.editDeck.cardCounts.begin(),a.editDeck.cardCounts.end(),[&](const auto& entry){return entry.first==card->referenceIndex;});
        if(it==a.editDeck.cardCounts.end())a.editDeck.cardCounts.emplace_back(card->referenceIndex,1);else ++it->second;
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
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:return (screen==Screen::DeckBuilder||screen==Screen::DeckSelect)?SDLK_PAGEUP:SDLK_UNKNOWN;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:return (screen==Screen::DeckBuilder||screen==Screen::DeckSelect)?SDLK_PAGEDOWN:SDLK_UNKNOWN;
    case SDL_CONTROLLER_BUTTON_START:return screen==Screen::DeckBuilder?SDLK_ESCAPE:SDLK_RETURN;
    default:return SDLK_UNKNOWN;
  }
}
}

int main(int argc,char** argv) {
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER)!=0)return 1;
  SDL_GameControllerEventState(SDL_ENABLE);
  for(int index=0;index<SDL_NumJoysticks();++index)if(SDL_IsGameController(index))SDL_GameControllerOpen(index);
  SDL_Window* window=SDL_CreateWindow("Gwent Classic - Witcher 3",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
  if(!window){SDL_Quit();return 2;}
  SDL_Renderer* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
  if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}
  SDL_RenderSetLogicalSize(renderer,W,H);
  App app; bool running=true;
  if(char* directory=SDL_GetPrefPath("GwentClassic","MiyooMiniPlus")) {
    app.dataDirectory=directory;app.savePath=app.dataDirectory+"match-v1.gws";
    if(gwent::loadUserDeck(userDeckPath(app,app.deckSlot),app.editDeck))app.userDeckSelected=true;
    SDL_free(directory);
  }
  if(argc>1 && std::string(argv[1])=="--smoke-test") {
    const auto saveFrame=[&](const char* path){
      SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
      if(!surface)return false;
      const bool okay=SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,surface->pixels,surface->pitch)==0&&SDL_SaveBMP(surface,path)==0;
      SDL_FreeSurface(surface);return okay;
    };
    drawTitle(renderer,app); SDL_RenderPresent(renderer);
    app.screen=Screen::DeckSelect; drawDeckSelect(renderer,app); SDL_RenderPresent(renderer);
    if(argc>3) {
      const auto& recipe=recipes().front();app.editDeck={"Smoke test",recipe.faction,recipe.leaderReferenceIndex,recipe.cardCounts};
      app.screen=Screen::DeckBuilder;drawDeckBuilder(renderer,app);SDL_RenderPresent(renderer);
      if(!saveFrame(argv[3])) { SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
      app.screen=Screen::DeckSelect;
    }
    startMatch(app);
    gwent::replaceMulliganCards(app.game,0,{}); gwent::replaceMulliganCards(app.game,1,gwent::chooseAiMulligan(app.game,1));
    drawMatch(renderer,app);
    if(argc>2&&!saveFrame(argv[2])) { SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 4; }
    SDL_RenderPresent(renderer);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
  }
  while(running) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type==SDL_QUIT)running=false;
      else if(event.type==SDL_KEYDOWN&&!event.key.repeat)handleKey(app,event.key.keysym.sym,running);
      else if(event.type==SDL_CONTROLLERBUTTONDOWN) {
        if(event.cbutton.button==SDL_CONTROLLER_BUTTON_BACK) {
          if(app.screen==Screen::Match)saveCurrentMatch(app);
          else if(app.screen==Screen::DeckBuilder)handleKey(app,SDLK_s,running);
          else handleKey(app,SDLK_ESCAPE,running);
        } else if(event.cbutton.button==SDL_CONTROLLER_BUTTON_START&&app.screen==Screen::Match)loadSavedMatch(app);
        else { const auto mapped=controllerKey(event.cbutton,app.screen);if(mapped!=SDLK_UNKNOWN)handleKey(app,mapped,running); }
      }
    }
    if(app.screen==Screen::Match && app.game.phase==gwent::Phase::OpponentTurn && SDL_GetTicks()>=app.aiAt)performAi(app);
    if(app.screen==Screen::Title)drawTitle(renderer,app);
    else if(app.screen==Screen::DeckSelect)drawDeckSelect(renderer,app);
    else if(app.screen==Screen::DeckBuilder)drawDeckBuilder(renderer,app);
    else drawMatch(renderer,app);
    SDL_RenderPresent(renderer); SDL_Delay(8);
  }
  SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
