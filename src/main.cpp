#include <SDL.h>
#include "gwent/ai.hpp"
#include "gwent/catalog.hpp"
#include "gwent/game.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
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

enum class Screen { Title, DeckSelect, Match };
enum class Language { Polish, English };
struct App {
  Screen screen{Screen::Title};
  Language language{Language::Polish};
  std::size_t deckIndex{0};
  gwent::Game game;
  int handIndex{0};
  int rowIndex{0};
  int targetIndex{0};
  bool targetMode{false};
  std::vector<std::string> selectedTargets;
  std::vector<std::string> mulligan;
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
  if(key=="controls")return pl?"STRZALKI - RUCH   ENTER/X - GRAJ   L - LIDER":"ARROWS - MOVE   ENTER/X - PLAY   L - LEADER";
  if(key=="pass")return pl?"BACKSPACE - PASUJ   ESC - MENU":"BACKSPACE - PASS   ESC - MENU";
  if(key=="round")return pl?"RUNDA":"ROUND";
  if(key=="your_turn")return pl?"TWOJ RUCH":"YOUR TURN";
  if(key=="ai_turn")return pl?"RUCH PRZECIWNIKA":"OPPONENT TURN";
  if(key=="continue")return pl?"ENTER - DALEJ":"ENTER - CONTINUE";
  if(key=="defeat")return pl?"PRZEGRANA":"DEFEAT";
  if(key=="victory")return pl?"ZWYCIESTWO":"VICTORY";
  if(key=="target")return pl?"WYBIERZ CEL ENTEREM, ESC ANULUJE":"CHOOSE TARGET WITH ENTER, ESC CANCELS";
  return key;
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
gwent::Card recipeLeader(std::size_t index) {
  gwent::Card leader;
  if(!recipes().empty()) gwent::referenceLeader(recipes()[index%recipes().size()].leaderReferenceIndex,leader);
  return leader;
}
void startMatch(App& a) {
  if(recipes().size()<2) { a.status="REFERENCE DECKS MISSING"; return; }
  const std::size_t aiIndex=(a.deckIndex+1)%recipes().size();
  gwent::Card humanLeader=recipeLeader(a.deckIndex), aiLeader=recipeLeader(aiIndex);
  gwent::beginMatchWithLeaders(a.game,gwent::makeReferenceDeck(recipes()[a.deckIndex]),humanLeader,
      gwent::makeReferenceDeck(recipes()[aiIndex]),aiLeader,SDL_GetTicks(),-1);
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
  text(r,104,87,factionName(selected.faction,a.language==Language::Polish),paper,2);
  text(r,104,115,"LEADER: "+leader.name,muted,1);
  text(r,104,139,"CARDS: "+std::to_string(gwent::makeReferenceDeck(selected).size()),muted,1);
  box(r,73,174,493,1,Color{65,72,67});
  text(r,74,193,tr(a,"opponent"),muted,1);
  text(r,74,215,factionName(opponent.faction,a.language==Language::Polish)+" / "+opponentLeader.name,paper,1);
  text(r,74,251,tr(a,"ai"),muted,1);
  text(r,226,251,"CLASSIC",gold,1);
  text(r,74,276,tr(a,"language"),muted,1); text(r,226,276,a.language==Language::Polish?"POLSKI":"ENGLISH",gold,1);
  text(r,25,332,tr(a,"instructions"),paper,1);
  text(r,119,370,tr(a,"confirm"),gold,1);
  text(r,170,414,"ESC - BACK",muted,1);
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
    box(r,0,366,W,45,panel); text(r,18,373,tr(a,"mulligan"),paper,1); text(r,18,390,tr(a,"confirm_mulligan"),gold,1);
  } else {
    text(r,18,372,tr(a,"controls"),paper,1); text(r,18,391,tr(a,"pass"),muted,1);
  static const char* rows[]={"MELEE","RANGED","SIEGE"};
  text(r,18,415,"ROW: "+std::string(rows[a.rowIndex]),gold,1);
  }
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
  if(a.game.phase==gwent::Phase::MatchResult) {
    if(key==SDLK_RETURN||key==SDLK_x||key==SDLK_SPACE) { a.screen=Screen::DeckSelect; return; }
    return;
  }
  if(a.game.phase==gwent::Phase::RoundResult) {
    if(key==SDLK_RETURN||key==SDLK_x||key==SDLK_SPACE) {
      gwent::startNextRound(a.game); a.handIndex=0; a.status=tr(a,"your_turn");
      if(a.game.phase==gwent::Phase::OpponentTurn)a.aiAt=SDL_GetTicks()+400;
    }
    return;
  }
  if(a.game.phase==gwent::Phase::Mulligan) {
    auto& hand=a.game.players[0].hand;
    if(key==SDLK_LEFT||key==SDLK_a)a.handIndex=(a.handIndex+static_cast<int>(hand.size())-1)%static_cast<int>(hand.size());
    else if(key==SDLK_RIGHT||key==SDLK_d)a.handIndex=(a.handIndex+1)%static_cast<int>(hand.size());
    else if((key==SDLK_SPACE||key==SDLK_x)&&!hand.empty()) {
      const auto& id=hand[a.handIndex].id; const auto it=std::find(a.mulligan.begin(),a.mulligan.end(),id);
      if(it!=a.mulligan.end())a.mulligan.erase(it); else if(a.mulligan.size()<2)a.mulligan.push_back(id);
    } else if(key==SDLK_BACKSPACE||key==SDLK_z) { if(!a.mulligan.empty())a.mulligan.pop_back(); }
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER) {
      if(gwent::replaceMulliganCards(a.game,0,a.mulligan) && gwent::replaceMulliganCards(a.game,1,{})) {
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
    if(key==SDLK_LEFT||key==SDLK_a)a.deckIndex=(a.deckIndex+recipes().size()-1)%recipes().size();
    else if(key==SDLK_RIGHT||key==SDLK_d)a.deckIndex=(a.deckIndex+1)%recipes().size();
    else if(key==SDLK_l)a.language=a.language==Language::Polish?Language::English:Language::Polish;
    else if(key==SDLK_RETURN||key==SDLK_KP_ENTER||key==SDLK_x)startMatch(a);
  } else handleMatchKey(a,key);
}
}

int main(int argc,char** argv) {
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER)!=0)return 1;
  SDL_Window* window=SDL_CreateWindow("Gwent Classic - Witcher 3",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
  if(!window){SDL_Quit();return 2;}
  SDL_Renderer* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
  if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}
  SDL_RenderSetLogicalSize(renderer,W,H);
  App app; bool running=true;
  if(argc>1 && std::string(argv[1])=="--smoke-test") {
    drawTitle(renderer,app); SDL_RenderPresent(renderer);
    app.screen=Screen::DeckSelect; drawDeckSelect(renderer,app); SDL_RenderPresent(renderer);
    startMatch(app);
    gwent::replaceMulliganCards(app.game,0,{}); gwent::replaceMulliganCards(app.game,1,{});
    drawMatch(renderer,app);
    if(argc>2) {
      SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
      if(!surface || SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,surface->pixels,surface->pitch)!=0 || SDL_SaveBMP(surface,argv[2])!=0) {
        if(surface)SDL_FreeSurface(surface);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 4;
      }
      SDL_FreeSurface(surface);
    }
    SDL_RenderPresent(renderer);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
  }
  while(running) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type==SDL_QUIT)running=false;
      else if(event.type==SDL_KEYDOWN&&!event.key.repeat)handleKey(app,event.key.keysym.sym,running);
    }
    if(app.screen==Screen::Match && app.game.phase==gwent::Phase::OpponentTurn && SDL_GetTicks()>=app.aiAt)performAi(app);
    if(app.screen==Screen::Title)drawTitle(renderer,app);
    else if(app.screen==Screen::DeckSelect)drawDeckSelect(renderer,app);
    else drawMatch(renderer,app);
    SDL_RenderPresent(renderer); SDL_Delay(8);
  }
  SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
