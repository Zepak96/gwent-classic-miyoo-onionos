#pragma once
#include "gwent/game.hpp"

namespace gwent {
struct ReferenceDeck {
  std::string id;
  Faction faction{Faction::Neutral};
  int leaderReferenceIndex{-1};
  std::vector<std::pair<int, int>> cardCounts;
};
// Generated from the pinned reference cards.json during the host build.
const std::vector<Card>& referenceCardCatalog();
const std::vector<ReferenceDeck>& referencePremadeDecks();
const std::string& referenceCardImage(int sourceIndex);
const std::string& referenceCardName(int sourceIndex, bool polish = false);
const std::string& referenceCardAbilityTitle(int sourceIndex, bool polish = false);
const std::string& referenceCardDescription(int sourceIndex, bool polish = false);
std::vector<Card> makeReferenceDeck(const ReferenceDeck& recipe);
bool referenceLeader(int sourceIndex, Card& leader);
}
