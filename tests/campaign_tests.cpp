#include "gwent/campaign.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <functional>
#include <numeric>
#include <stdexcept>
using namespace gwent;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void allFourTowersPersistIndependently() {
 const auto& nodes=campaignNodes();
 const std::array<std::string,4> factions{{"realms","nilfgaard","monsters","skellige"}};
 // Interleave towers in a non-catalog order; after every battle reload the
 // on-disk progress into a new object to simulate closing and reopening play.
 const std::array<int,4> order{{3,1,0,2}};
 std::array<int,4> completed{};
 CampaignProgress live;
 const auto file=(std::filesystem::temp_directory_path()/"gwent-campaign-four-towers-test.bin").string();
 std::string error;
 const auto checkGates=[&](const CampaignProgress& progress) {
   for(std::size_t faction=0;faction<factions.size();++faction)for(int stage=1;stage<=5;++stage) {
     const auto id=factions[faction]+"-"+std::to_string(stage);
     const auto node=std::find_if(nodes.begin(),nodes.end(),[&](const auto& n){return n.id==id;});
     require(node!=nodes.end(),"all four complete towers exist");
     const auto index=static_cast<std::size_t>(node-nodes.begin());
     require(progress.victories[index]==(stage<=completed[faction]),"only earned faction achievements survive");
     require(campaignUnlocked(progress,id)==(stage<=completed[faction]+1),"each faction unlock follows its own predecessor");
   }
 };
 checkGates(live);
 for(int stage=1;stage<=5;++stage)for(const auto faction:order) {
   const auto id=factions[static_cast<std::size_t>(faction)]+"-"+std::to_string(stage);
   const auto before=live.victories;
   require(recordCampaignVictory(live,id),"new victory advances selected tower");
   ++completed[static_cast<std::size_t>(faction)];
   const auto changed=std::inner_product(before.begin(),before.end(),live.victories.begin(),0,
     std::plus<int>{},[](bool a,bool b){return a!=b?1:0;});
   require(changed==1,"victory changes exactly one achievement");
   checkGates(live);
   const auto awarded=live.victories;
   require(!recordCampaignVictory(live,id)&&live.victories==awarded,"replaying a beaten opponent grants no duplicate achievement");
   require(!recordCampaignVictory(live,"not-a-campaign-opponent")&&live.victories==awarded,"unknown opponent does not alter any tower");
   require(saveCampaignProgress(file,live,&error),"save interleaved tower progress");
   CampaignProgress restarted;
   require(loadCampaignProgress(file,restarted,&error),"reload after each campaign victory");
   require(restarted.victories==live.victories,"all faction progress persists after restart");
   live=restarted;checkGates(live);
 }
 require(std::all_of(live.victories.begin(),live.victories.end(),[](bool won){return won;}),"all twenty opponents can be completed");
 const auto fullyCompleted=live.victories;
 {std::fstream corrupt(file,std::ios::binary|std::ios::in|std::ios::out);corrupt.seekp(0);corrupt.put('!');}
 require(!loadCampaignProgress(file,live,&error)&&live.victories==fullyCompleted,"failed load cannot erase completed towers");
 std::filesystem::remove(file);
}

int main(){try {
 allFourTowersPersistIndependently();
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
