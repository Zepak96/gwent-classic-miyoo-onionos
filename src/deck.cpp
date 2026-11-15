#include "gwent/deck.hpp"
#include "gwent/catalog.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <system_error>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace gwent {
namespace {
constexpr std::array<std::uint8_t,8> kMagic{{'G','W','N','T','D','E','C','K'}};
constexpr std::uint32_t kVersion=1;
constexpr std::size_t kMaxFile=64*1024;
constexpr std::size_t kMaxName=96;
constexpr std::uint32_t kMaxEntries=512;
constexpr std::size_t kHeader=20;

class Writer {
 public:
  std::vector<std::uint8_t> data;
  void u8(std::uint8_t x){data.push_back(x);}
  void u32(std::uint32_t x){for(int i=0;i<4;++i)u8(static_cast<std::uint8_t>(x>>(i*8)));}
  void i32(std::int32_t x){u32(static_cast<std::uint32_t>(x));}
  void string(const std::string& x){u32(static_cast<std::uint32_t>(x.size()));data.insert(data.end(),x.begin(),x.end());}
};
class Reader {
 public:
  explicit Reader(const std::vector<std::uint8_t>& bytes):data(bytes){}
  const std::vector<std::uint8_t>& data;
  std::size_t pos{0}; bool good{true};
  std::uint8_t u8(){if(pos>=data.size()){good=false;return 0;}return data[pos++];}
  std::uint32_t u32(){std::uint32_t x=0;for(int i=0;i<4;++i)x|=static_cast<std::uint32_t>(u8())<<(i*8);return x;}
  std::int32_t i32(){return static_cast<std::int32_t>(u32());}
  std::string string(std::size_t limit){const auto n=u32();if(n>limit||n>data.size()-std::min(pos,data.size())){good=false;return {};}
    std::string x(data.begin()+static_cast<std::ptrdiff_t>(pos),data.begin()+static_cast<std::ptrdiff_t>(pos+n));pos+=n;return x;}
  bool done()const{return good&&pos==data.size();}
};
std::uint32_t checksum(const std::vector<std::uint8_t>& bytes){
  std::uint32_t crc=0xffffffffu;
  for(const auto byte:bytes){crc^=byte;for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<std::int32_t>(crc&1u)));}
  return ~crc;
}
void setError(std::string* error,const std::string& value){if(error)*error=value;}
bool replaceFile(const std::filesystem::path& temporary,const std::filesystem::path& target){
#if defined(_WIN32)
  return MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
  return std::rename(temporary.c_str(),target.c_str())==0;
#endif
}
bool validFaction(Faction faction){return static_cast<std::uint8_t>(faction)<=static_cast<std::uint8_t>(Faction::Skellige);}
}

std::string validateUserDeck(const UserDeck& recipe){
  if(recipe.name.empty()||recipe.name.size()>kMaxName)return "deck name must contain 1 to 96 UTF-8 bytes";
  if(!validFaction(recipe.faction))return "deck faction is invalid";
  if(recipe.cardCounts.size()>kMaxEntries)return "deck has too many card entries";
  std::map<int,int> unique;
  for(const auto& entry:recipe.cardCounts){
    if(entry.first<0||entry.second<=0)return "card entries must have a valid reference and positive count";
    if(!unique.emplace(entry.first,entry.second).second)return "deck contains a duplicate card entry";
  }
  Card leader;
  if(!referenceLeader(recipe.leaderReferenceIndex,leader)||leader.faction!=recipe.faction)
    return "leader is not available for this faction";
  const auto cards=makeReferenceDeck(recipe.cardCounts);
  if(cards.empty())return "deck card list is empty or references an unavailable card";
  return validateDeck(cards,leader,recipe.faction);
}

bool materializeUserDeck(const UserDeck& recipe,std::vector<Card>& cards,Card& leader,std::string* error){
  const auto invalid=validateUserDeck(recipe);if(!invalid.empty()){setError(error,invalid);return false;}
  Card resolvedLeader;referenceLeader(recipe.leaderReferenceIndex,resolvedLeader);
  auto resolvedCards=makeReferenceDeck(recipe.cardCounts);
  cards=std::move(resolvedCards);leader=std::move(resolvedLeader);setError(error,{});return true;
}

bool saveUserDeck(const std::string& path,const UserDeck& deck,std::string* error){
  const auto invalid=validateUserDeck(deck);if(!invalid.empty()){setError(error,invalid);return false;}
  Writer payload;payload.string(deck.name);payload.u8(static_cast<std::uint8_t>(deck.faction));payload.i32(deck.leaderReferenceIndex);
  payload.u32(static_cast<std::uint32_t>(deck.cardCounts.size()));
  for(const auto& entry:deck.cardCounts){payload.i32(entry.first);payload.i32(entry.second);}
  Writer file;file.data.insert(file.data.end(),kMagic.begin(),kMagic.end());file.u32(kVersion);file.u32(static_cast<std::uint32_t>(payload.data.size()));file.u32(checksum(payload.data));
  file.data.insert(file.data.end(),payload.data.begin(),payload.data.end());
  const auto target=std::filesystem::u8path(path);std::error_code ec;
  if(target.has_parent_path())std::filesystem::create_directories(target.parent_path(),ec);
  if(ec){setError(error,"cannot create deck directory: "+ec.message());return false;}
  auto temporary=target;temporary+=".tmp";
  {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out){setError(error,"cannot open temporary deck file");return false;}
    out.write(reinterpret_cast<const char*>(file.data.data()),static_cast<std::streamsize>(file.data.size()));out.flush();
    if(!out){setError(error,"cannot write deck file");out.close();std::filesystem::remove(temporary,ec);return false;}}
  if(!replaceFile(temporary,target)){setError(error,"cannot replace deck file");std::filesystem::remove(temporary,ec);return false;}
  setError(error,{});return true;
}

bool loadUserDeck(const std::string& path,UserDeck& deck,std::string* error){
  std::ifstream in(std::filesystem::u8path(path),std::ios::binary);if(!in){setError(error,"deck file not found");return false;}
  in.seekg(0,std::ios::end);const auto end=in.tellg();
  if(end<static_cast<std::streamoff>(kHeader)||end>static_cast<std::streamoff>(kHeader+kMaxFile)){setError(error,"deck file size is invalid");return false;}
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));in.seekg(0,std::ios::beg);in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
  if(!in){setError(error,"cannot read deck file");return false;}
  if(!std::equal(kMagic.begin(),kMagic.end(),bytes.begin())){setError(error,"deck signature is invalid");return false;}
  const std::vector<std::uint8_t> headerBytes(bytes.begin()+8,bytes.begin()+kHeader);Reader header(headerBytes);
  const auto version=header.u32(),length=header.u32(),sum=header.u32();
  if(version!=kVersion){setError(error,"deck version is not supported");return false;}
  if(length!=bytes.size()-kHeader){setError(error,"deck length is invalid");return false;}
  std::vector<std::uint8_t> payload(bytes.begin()+kHeader,bytes.end());if(checksum(payload)!=sum){setError(error,"deck checksum does not match");return false;}
  Reader reader(payload);UserDeck decoded;decoded.name=reader.string(kMaxName);const auto faction=reader.u8();
  if(faction>static_cast<std::uint8_t>(Faction::Skellige)){setError(error,"deck faction is invalid");return false;}
  decoded.faction=static_cast<Faction>(faction);decoded.leaderReferenceIndex=reader.i32();const auto count=reader.u32();
  if(count>kMaxEntries){setError(error,"deck has too many card entries");return false;}
  decoded.cardCounts.reserve(count);
  for(std::uint32_t i=0;i<count;++i){const auto referenceIndex=reader.i32();const auto copies=reader.i32();decoded.cardCounts.emplace_back(referenceIndex,copies);}
  if(!reader.done()){setError(error,"deck data is incomplete or invalid");return false;}
  const auto invalid=validateUserDeck(decoded);if(!invalid.empty()){setError(error,invalid);return false;}
  deck=std::move(decoded);setError(error,{});return true;
}
}
