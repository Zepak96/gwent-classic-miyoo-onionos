#pragma once
#include "gwent/game.hpp"
#include <string>
#include <vector>

namespace gwent {
struct AiAction {
  bool pass{false};
  std::string cardId;
  Row target{Row::None};
  std::string selectedCardId;
  bool activateLeader{false};
  std::vector<std::string> leaderSelection;
};
// Weighted, seeded ControllerAI ported from the pinned Gwent Classic reference.
AiAction chooseAiAction(Game& game, int playerIndex);
std::vector<std::string> chooseAiMulligan(const Game& game, int playerIndex);
}
