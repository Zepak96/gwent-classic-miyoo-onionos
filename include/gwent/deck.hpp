#pragma once

#include "gwent/game.hpp"
#include <string>
#include <utility>
#include <vector>

namespace gwent {
// A deck recipe references cards in the pinned Witcher 3 Gwent catalog.
// It deliberately stores no copied card text or assets.
struct UserDeck {
  std::string name;
  Faction faction{Faction::NorthernRealms};
  int leaderReferenceIndex{-1};
  std::vector<std::pair<int, int>> cardCounts;
};

std::string validateUserDeck(const UserDeck& deck);
bool materializeUserDeck(const UserDeck& recipe, std::vector<Card>& cards,
                         Card& leader, std::string* error = nullptr);
bool saveUserDeck(const std::string& path, const UserDeck& deck,
                  std::string* error = nullptr);
bool loadUserDeck(const std::string& path, UserDeck& deck,
                  std::string* error = nullptr);
}
