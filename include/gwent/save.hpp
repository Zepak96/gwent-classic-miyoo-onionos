#pragma once

#include "gwent/game.hpp"
#include <string>

namespace gwent {
// Saves are versioned, checksummed, and replaced atomically. A failed load never
// changes the caller's current game.
bool saveGame(const std::string& path, const Game& game, std::string* error = nullptr);
bool loadGame(const std::string& path, Game& game, std::string* error = nullptr);
}
