#include "gwent/campaign.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace gwent;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try {
 const auto& nodes=campaignNodes();require(nodes.size()==20,"twenty campaign nodes");CampaignProgress p;
 for(const auto& node:nodes){Card leader;auto cards=makeReferenceDeck(node.deck);require(referenceLeader(node.deck.leaderReferenceIndex,leader),"leader exists");require(validateDeck(cards,leader,node.faction).empty(),"legal faction deck");require(campaignUnlocked(p,node.id)==(node.stage==1),"initial unlocks");}
 require(!recordCampaignVictory(p,"realms-5"),"locked boss cannot be awarded");
 require(!recordCampaignVictory(p,"missing"),"unknown achievement rejected");
 require(recordCampaignVictory(p,"realms-1"),"first achievement awarded");
 require(!recordCampaignVictory(p,"realms-1"),"achievement idempotent");
 require(campaignUnlocked(p,"realms-2")&&!campaignUnlocked(p,"realms-3"),"one new tier unlocked");
 require(!campaignUnlocked(p,"nilfgaard-2"),"faction progression independent");
 const auto file=(std::filesystem::temp_directory_path()/"gwent-campaign-progress-test.bin").string();std::string error;
 require(saveCampaignProgress(file,p,&error),"save progress");CampaignProgress restored;
 require(loadCampaignProgress(file,restored,&error)&&restored.victories==p.victories,"progress roundtrip");
 {std::fstream corrupt(file,std::ios::binary|std::ios::in|std::ios::out);corrupt.seekp(25);corrupt.put('!');}
 require(!loadCampaignProgress(file,restored,&error)&&restored.victories==p.victories,"corrupt load preserves progress");
 for(int stage=2;stage<=5;++stage)require(recordCampaignVictory(p,"realms-"+std::to_string(stage)),"complete tower");
 require(saveCampaignProgress(file,p,&error)&&loadCampaignProgress(file,restored,&error)&&restored.victories==p.victories,"completed tower roundtrip");
 CampaignProgress illegal;const auto boss=std::find_if(nodes.begin(),nodes.end(),[](const auto& n){return n.id=="realms-5";});require(boss!=nodes.end(),"boss exists");illegal.victories[static_cast<std::size_t>(boss-nodes.begin())]=true;require(saveCampaignProgress(file,illegal,&error),"fixture invalid progress");require(!loadCampaignProgress(file,restored,&error),"reject out-of-order win");
 std::filesystem::remove(file);std::cout<<"Campaign legality, unlocks, achievements and persistence passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
