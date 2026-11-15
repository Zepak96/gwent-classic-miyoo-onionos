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
std::vector<Card> makeReferenceDeck(const ReferenceDeck& recipe);
bool referenceLeader(int sourceIndex, Card& leader);
}
