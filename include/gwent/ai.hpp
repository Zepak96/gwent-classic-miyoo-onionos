#pragma once
#include "gwent/game.hpp"
#include <string>

namespace gwent {
enum class Difficulty : std::uint8_t { Easy, Normal, Hard };
struct AiAction {
  bool pass{false};
  std::string cardId;
  Row target{Row::None};
  std::string selectedCardId;
};
AiAction chooseAiAction(Game& game, int playerIndex, Difficulty difficulty);
}
