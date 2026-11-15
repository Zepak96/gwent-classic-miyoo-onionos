#include "gwent/save.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <system_error>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace gwent {
namespace {
constexpr std::array<std::uint8_t,8> kMagic{{'G','W','N','T','M','Y','S','V'}};
constexpr std::uint32_t kVersion=2;
constexpr std::size_t kMaxSaveBytes=4*1024*1024;
constexpr std::uint32_t kMaxItems=4096;
constexpr std::uint32_t kMaxString=8192;

class Writer {
 public:
  std::vector<std::uint8_t> data;
  void u8(std::uint8_t x){data.push_back(x);}
  void u32(std::uint32_t x){for(int i=0;i<4;++i)u8(static_cast<std::uint8_t>(x>>(i*8)));}
  void u64(std::uint64_t x){for(int i=0;i<8;++i)u8(static_cast<std::uint8_t>(x>>(i*8)));}
  void i32(std::int32_t x){u32(static_cast<std::uint32_t>(x));}
  void boolean(bool x){u8(x?1:0);}
  void string(const std::string& x){u32(static_cast<std::uint32_t>(x.size()));data.insert(data.end(),x.begin(),x.end());}
};

class Reader {
 public:
  explicit Reader(const std::vector<std::uint8_t>& bytes):data(bytes){}
  const std::vector<std::uint8_t>& data;
  std::size_t pos{0};
  bool good{true};
  std::uint8_t u8(){if(pos>=data.size()){good=false;return 0;}return data[pos++];}
  std::uint32_t u32(){std::uint32_t x=0;for(int i=0;i<4;++i)x|=static_cast<std::uint32_t>(u8())<<(i*8);return x;}
  std::uint64_t u64(){std::uint64_t x=0;for(int i=0;i<8;++i)x|=static_cast<std::uint64_t>(u8())<<(i*8);return x;}
  std::int32_t i32(){return static_cast<std::int32_t>(u32());}
  bool boolean(){const auto x=u8();if(x>1)good=false;return x!=0;}
  std::string string(){const auto n=u32();if(n>kMaxString||n>data.size()-std::min(pos,data.size())){good=false;return {};}std::string x(data.begin()+static_cast<std::ptrdiff_t>(pos),data.begin()+static_cast<std::ptrdiff_t>(pos+n));pos+=n;return x;}
  bool done()const{return good&&pos==data.size();}
};

std::uint32_t crc32(const std::vector<std::uint8_t>& bytes){
  std::uint32_t crc=0xffffffffu;
  for(const auto byte:bytes){crc^=byte;for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<std::int32_t>(crc&1u)));}
  return ~crc;
}

template<class E> void enumWrite(Writer& w,E e){w.u8(static_cast<std::uint8_t>(e));}
template<class E> bool enumRead(Reader& r,E& e,std::uint8_t max){const auto x=r.u8();if(x>max){r.good=false;return false;}e=static_cast<E>(x);return true;}
void writeCard(Writer& w,const Card& c){
  w.string(c.id);w.string(c.name);enumWrite(w,c.kind);enumWrite(w,c.row);enumWrite(w,c.faction);w.i32(c.power);
  std::uint16_t flags=0;const bool values[]={c.hero,c.spy,c.medic,c.agile,c.morale,c.bond,c.muster,c.berserker};
  for(unsigned i=0;i<8;++i)if(values[i])flags|=static_cast<std::uint16_t>(1u<<i);
  w.u32(flags);w.string(c.group);w.u32(static_cast<std::uint32_t>(c.abilities.size()));for(const auto& a:c.abilities)w.string(a);
  w.i32(c.referenceIndex);w.i32(c.deckCopies);
}
bool readCard(Reader& r,Card& c){
  c.id=r.string();c.name=r.string();enumRead(r,c.kind,static_cast<std::uint8_t>(Kind::Decoy));enumRead(r,c.row,static_cast<std::uint8_t>(Row::None));
  enumRead(r,c.faction,static_cast<std::uint8_t>(Faction::Neutral));c.power=r.i32();const auto flags=r.u32();if(flags&~0xffu)r.good=false;
  c.hero=(flags&1)!=0;c.spy=(flags&2)!=0;c.medic=(flags&4)!=0;c.agile=(flags&8)!=0;c.morale=(flags&16)!=0;c.bond=(flags&32)!=0;c.muster=(flags&64)!=0;c.berserker=(flags&128)!=0;
  c.group=r.string();const auto n=r.u32();if(n>64){r.good=false;return false;}c.abilities.clear();c.abilities.reserve(n);for(std::uint32_t i=0;i<n;++i)c.abilities.push_back(r.string());
  c.referenceIndex=r.i32();c.deckCopies=r.i32();return r.good;
}
void writeCards(Writer& w,const std::vector<Card>& cards){w.u32(static_cast<std::uint32_t>(cards.size()));for(const auto& c:cards)writeCard(w,c);}
bool readCards(Reader& r,std::vector<Card>& cards){const auto n=r.u32();if(n>kMaxItems){r.good=false;return false;}cards.clear();cards.reserve(n);for(std::uint32_t i=0;i<n;++i){Card c;if(!readCard(r,c))return false;cards.push_back(std::move(c));}return r.good;}
void writePlayer(Writer& w,const Player& p){
  writeCards(w,p.deck);writeCards(w,p.hand);writeCards(w,p.graveyard);for(const auto& row:p.board)writeCards(w,row);writeCard(w,p.leader);
  const bool values[]={p.passed,p.mulliganDone,p.leaderAbilityAvailable,p.leaderAbilityCancelled,p.leaderPassivesApplied,p.halfWeather,p.doubleSpyPower};
  std::uint16_t flags=0;for(unsigned i=0;i<7;++i)if(values[i])flags|=static_cast<std::uint16_t>(1u<<i);w.u32(flags);
  w.i32(p.score);w.i32(p.roundWins);w.i32(p.roundLosses);enumWrite(w,p.faction);
}
bool readPlayer(Reader& r,Player& p){
  if(!readCards(r,p.deck)||!readCards(r,p.hand)||!readCards(r,p.graveyard))return false;for(auto& row:p.board)if(!readCards(r,row))return false;
  if(!readCard(r,p.leader))return false;const auto flags=r.u32();if(flags&~0x7fu)r.good=false;
  p.passed=(flags&1)!=0;p.mulliganDone=(flags&2)!=0;p.leaderAbilityAvailable=(flags&4)!=0;p.leaderAbilityCancelled=(flags&8)!=0;
  p.leaderPassivesApplied=(flags&16)!=0;p.halfWeather=(flags&32)!=0;p.doubleSpyPower=(flags&64)!=0;
  p.score=r.i32();p.roundWins=r.i32();p.roundLosses=r.i32();enumRead(r,p.faction,static_cast<std::uint8_t>(Faction::Neutral));
  return r.good&&p.roundWins>=0&&p.roundLosses>=0&&p.roundWins<=2&&p.roundLosses<=2;
}
void encodeGame(Writer& w,const Game& g){
  for(const auto& p:g.players)writePlayer(w,p);enumWrite(w,g.phase);w.i32(g.currentPlayer);w.i32(g.firstPlayer);w.i32(g.matchWinner);w.i32(g.round);
  for(bool x:g.weather)w.boolean(x);for(const auto& side:g.horn)for(bool x:side)w.boolean(x);
  w.u32(static_cast<std::uint32_t>(g.weatherCards.size()));for(const auto& x:g.weatherCards){writeCard(w,x.card);w.i32(x.owner);}
  w.boolean(g.randomRespawn);w.boolean(g.scoiataelChoicePending);w.i32(g.skelligeRevivesRemaining);w.i32(g.skelligeRevivePlayer);w.u32(static_cast<std::uint32_t>(g.skelligeAgileReviveIds.size()));for(const auto& id:g.skelligeAgileReviveIds)w.string(id);w.boolean(g.factionAbilitiesInitialized);
  for(const auto& cards:g.revealedCards)writeCards(w,cards);w.u64(g.rngState);w.string(g.lastError);
}
bool decodeGame(Reader& r,Game& g){
  for(auto& p:g.players)if(!readPlayer(r,p))return false;
  enumRead(r,g.phase,static_cast<std::uint8_t>(Phase::MatchResult));g.currentPlayer=r.i32();g.firstPlayer=r.i32();g.matchWinner=r.i32();g.round=r.i32();
  for(auto& x:g.weather)x=r.boolean();for(auto& side:g.horn)for(auto& x:side)x=r.boolean();
  const auto n=r.u32();if(n>128){r.good=false;return false;}g.weatherCards.clear();g.weatherCards.reserve(n);
  for(std::uint32_t i=0;i<n;++i){ActiveWeather x;if(!readCard(r,x.card))return false;x.owner=r.i32();if(x.owner<0||x.owner>1)r.good=false;g.weatherCards.push_back(std::move(x));}
  g.randomRespawn=r.boolean();g.scoiataelChoicePending=r.boolean();g.skelligeRevivesRemaining=r.i32();g.skelligeRevivePlayer=r.i32();const auto reviveIds=r.u32();if(reviveIds>2){r.good=false;return false;}g.skelligeAgileReviveIds.clear();for(std::uint32_t i=0;i<reviveIds;++i)g.skelligeAgileReviveIds.push_back(r.string());g.factionAbilitiesInitialized=r.boolean();
  for(auto& cards:g.revealedCards)if(!readCards(r,cards))return false;g.rngState=r.u64();g.lastError=r.string();
  return r.good&&g.currentPlayer>=0&&g.currentPlayer<2&&g.firstPlayer>=0&&g.firstPlayer<2&&g.matchWinner>=-1&&g.matchWinner<=2&&g.round>=1&&g.round<=3&&
    g.skelligeRevivesRemaining>=0&&g.skelligeRevivesRemaining<=2&&g.skelligeRevivePlayer>=-1&&g.skelligeRevivePlayer<2&&
    (g.skelligeRevivesRemaining==0||g.skelligeRevivePlayer>=0)&&g.skelligeRevivesRemaining==static_cast<int>(g.skelligeAgileReviveIds.size());
}
void setError(std::string* error,const std::string& value){if(error)*error=value;}
bool replaceFile(const std::filesystem::path& temporary,const std::filesystem::path& path){
#if defined(_WIN32)
  return MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
  return std::rename(temporary.c_str(),path.c_str())==0;
#endif
}
}

bool saveGame(const std::string& path,const Game& game,std::string* error){
  Writer payload;encodeGame(payload,game);if(payload.data.size()>kMaxSaveBytes){setError(error,"save is larger than the supported limit");return false;}
  Writer file;file.data.insert(file.data.end(),kMagic.begin(),kMagic.end());file.u32(kVersion);file.u32(static_cast<std::uint32_t>(payload.data.size()));file.u32(crc32(payload.data));
  file.data.insert(file.data.end(),payload.data.begin(),payload.data.end());
  std::error_code ec;const std::filesystem::path target=std::filesystem::u8path(path);if(target.has_parent_path())std::filesystem::create_directories(target.parent_path(),ec);
  if(ec){setError(error,"cannot create save directory: "+ec.message());return false;}
  std::filesystem::path temporary=target;temporary+=".tmp";
  {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out){setError(error,"cannot open temporary save file");return false;}
    out.write(reinterpret_cast<const char*>(file.data.data()),static_cast<std::streamsize>(file.data.size()));out.flush();
    if(!out){setError(error,"cannot write save file");out.close();std::filesystem::remove(temporary,ec);return false;}}
  if(!replaceFile(temporary,path)){setError(error,"cannot replace save file");std::filesystem::remove(temporary,ec);return false;}
  setError(error,{});return true;
}

bool loadGame(const std::string& path,Game& game,std::string* error){
  std::ifstream in(std::filesystem::u8path(path),std::ios::binary);if(!in){setError(error,"save file not found");return false;}
  constexpr std::size_t headerSize=20;
  in.seekg(0,std::ios::end);const auto end=in.tellg();
  if(end<0||static_cast<std::uint64_t>(end)<headerSize||static_cast<std::uint64_t>(end)>kMaxSaveBytes+headerSize){setError(error,"save file size is invalid");return false;}
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));in.seekg(0,std::ios::beg);
  in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
  if(!in){setError(error,"cannot read save file");return false;}
  if(!std::equal(kMagic.begin(),kMagic.end(),bytes.begin())){setError(error,"save file signature is invalid");return false;}
  std::vector<std::uint8_t> header(bytes.begin()+8,bytes.begin()+headerSize);Reader hr(header);
  const auto version=hr.u32(),length=hr.u32(),checksum=hr.u32();
  if(version!=kVersion){setError(error,"save version is not supported");return false;}
  if(length!=bytes.size()-headerSize){setError(error,"save file length is invalid");return false;}
  std::vector<std::uint8_t> payload(bytes.begin()+headerSize,bytes.end());if(crc32(payload)!=checksum){setError(error,"save checksum does not match");return false;}
  Reader reader(payload);Game decoded;
  if(!decodeGame(reader,decoded)||!reader.done()){setError(error,"save data is invalid or incomplete");return false;}
  game=std::move(decoded);setError(error,{});return true;
}
}
