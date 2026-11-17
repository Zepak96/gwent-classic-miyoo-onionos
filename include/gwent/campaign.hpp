#pragma once
#include "gwent/catalog.hpp"
#include <array>
namespace gwent {
struct CampaignNode {
  std::string id;
  Faction faction;
  int stage;
  std::string namePl,nameEn,introPl,introEn,portrait;
  ReferenceDeck deck;
};
const std::vector<CampaignNode>& campaignNodes();
struct CampaignProgress { std::array<bool,20> victories{}; };
bool campaignUnlocked(const CampaignProgress&,const std::string& nodeId);
bool recordCampaignVictory(CampaignProgress&,const std::string& nodeId);
bool saveCampaignProgress(const std::string&,const CampaignProgress&,std::string* error=nullptr);
bool loadCampaignProgress(const std::string&,CampaignProgress&,std::string* error=nullptr);
}
