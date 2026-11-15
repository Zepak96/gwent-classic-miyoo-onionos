#pragma once
#include "gwent/game.hpp"
#include <string>
#include <vector>

namespace gwent {
enum class Difficulty : std::uint8_t { Easy, Normal, Hard };
struct AiAction {
  bool pass{false};
  std::string cardId;
  Row target{Row::None};
  std::string selectedCardId;
  bool activateLeader{false};
  std::vector<std::string> leaderSelection;
};
AiAction chooseAiAction(Game& game, int playerIndex, Difficulty difficulty);
}
