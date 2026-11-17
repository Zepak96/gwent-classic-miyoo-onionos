#include "gwent/campaign.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#if defined(_WIN32)
#include <windows.h>
#endif
namespace gwent { namespace {
std::size_t nodeIndex(const std::string& id){const auto& n=campaignNodes();const auto i=std::find_if(n.begin(),n.end(),[&](const auto& x){return x.id==id;});return static_cast<std::size_t>(i-n.begin());}
void error(std::string* e,const char* text){if(e)*e=text;}
std::uint32_t crc(const std::string& s){std::uint32_t c=0xffffffffu;for(unsigned char b:s){c^=b;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<std::int32_t>(c&1u)));}return ~c;}
void put(std::string& s,std::uint32_t x){for(int i=0;i<4;++i)s.push_back(static_cast<char>(x>>(i*8)));}
std::uint32_t get(const std::string& s,std::size_t pos){std::uint32_t x=0;for(int i=0;i<4;++i)x|=static_cast<std::uint32_t>(static_cast<unsigned char>(s[pos+i]))<<(i*8);return x;}
}
bool campaignUnlocked(const CampaignProgress& p,const std::string& id){
 const auto i=nodeIndex(id);const auto& nodes=campaignNodes();if(i>=nodes.size()||i>=p.victories.size())return false;
 if(nodes[i].stage==1)return true;
 const auto prev=std::find_if(nodes.begin(),nodes.end(),[&](const auto& n){return n.faction==nodes[i].faction&&n.stage==nodes[i].stage-1;});
 return prev!=nodes.end()&&p.victories[static_cast<std::size_t>(prev-nodes.begin())];
}
bool recordCampaignVictory(CampaignProgress& p,const std::string& id){const auto i=nodeIndex(id);if(i>=p.victories.size()||!campaignUnlocked(p,id)||p.victories[i])return false;p.victories[i]=true;return true;}
bool saveCampaignProgress(const std::string& name,const CampaignProgress& p,std::string* e){
 if(name.empty()){error(e,"campaign progress path is empty");return false;}
 if(campaignNodes().size()!=20){error(e,"campaign catalog invalid");return false;}
 std::string payload;put(payload,20);
 for(std::size_t i=0;i<20;++i){const auto& id=campaignNodes()[i].id;put(payload,static_cast<std::uint32_t>(id.size()));payload+=id;payload.push_back(p.victories[i]?1:0);}
 std::string file="GWNCAMP1";put(file,1);put(file,static_cast<std::uint32_t>(payload.size()));put(file,crc(payload));file+=payload;
 const auto path=std::filesystem::u8path(name);std::error_code ec;if(path.has_parent_path())std::filesystem::create_directories(path.parent_path(),ec);
 if(ec){error(e,"cannot create campaign directory");return false;}
 auto temp=path;temp+=".tmp";
 {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write(file.data(),static_cast<std::streamsize>(file.size()));out.flush();if(!out){error(e,"cannot write campaign progress");return false;}}
#if defined(_WIN32)
 const bool moved=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 const bool moved=std::rename(temp.c_str(),path.c_str())==0;
#endif
 if(!moved){error(e,"cannot replace campaign progress");return false;}error(e,"");return true;
}
bool loadCampaignProgress(const std::string& name,CampaignProgress& progress,std::string* e){
 std::ifstream in(std::filesystem::u8path(name),std::ios::binary);if(!in){error(e,"campaign progress not found");return false;}
 in.seekg(0,std::ios::end);const auto size=in.tellg();if(size<20||size>4096){error(e,"campaign progress size invalid");return false;}in.seekg(0);
 const std::string file((std::istreambuf_iterator<char>(in)),{});
 if(file.size()!=static_cast<std::size_t>(size)||file.substr(0,8)!="GWNCAMP1"||get(file,8)!=1||get(file,12)!=file.size()-20||get(file,16)!=crc(file.substr(20))){error(e,"campaign progress checksum or version invalid");return false;}
 CampaignProgress decoded;std::set<std::string> ids;std::size_t pos=24;
 if(file.size()<24||get(file,20)!=20){error(e,"campaign entry count invalid");return false;}
 for(int n=0;n<20;++n){
  if(pos+4>file.size()){error(e,"campaign entry incomplete");return false;}const auto length=get(file,pos);pos+=4;
  if(length==0||length>64||pos+length+1>file.size()){error(e,"campaign id invalid");return false;}
  const auto id=file.substr(pos,length);pos+=length;const auto value=static_cast<unsigned char>(file[pos++]);const auto index=nodeIndex(id);
  if(index>=20||!ids.insert(id).second||value>1){error(e,"campaign entry invalid");return false;}decoded.victories[index]=value!=0;
 }
 if(pos!=file.size()){error(e,"campaign trailing data");return false;}
 for(std::size_t i=0;i<20;++i)if(decoded.victories[i]&&!campaignUnlocked(decoded,campaignNodes()[i].id)){error(e,"campaign unlock sequence invalid");return false;}
 progress=decoded;error(e,"");return true;
}
}
