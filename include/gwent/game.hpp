#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gwent {
enum class Row : std::uint8_t { Melee, Ranged, Siege, None };
enum class Kind : std::uint8_t { Unit, Weather, ClearWeather, CommanderHorn, Scorch, Decoy };
enum class Faction : std::uint8_t { NorthernRealms, Nilfgaard, Scoiatael, Monsters, Skellige, Neutral };
enum class Phase : std::uint8_t { Mulligan, PlayerTurn, OpponentTurn, RoundResult, MatchResult };

struct Card {
  std::string id;
  std::string name;
  Kind kind{Kind::Unit};
  Row row{Row::None};
  Faction faction{Faction::Neutral};
  int power{0};
  bool hero{false};
  bool spy{false};
  bool medic{false};
  bool agile{false};
  bool morale{false};
  bool bond{false};
  std::string group;
};
struct Player {
  std::vector<Card> deck, hand, graveyard;
  std::array<std::vector<Card>, 3> board;
  bool passed{false};
  int score{0};
  int roundWins{0};
  Faction faction{Faction::NorthernRealms};
};
struct Game {
  std::array<Player, 2> players;
  Phase phase{Phase::PlayerTurn};
  int currentPlayer{0};
  int round{1};
  std::array<bool, 3> weather{false, false, false};
  std::array<bool, 3> horn{false, false, false};
  std::uint64_t rngState{1};
  std::string lastError;
};

int calculateScore(const Player&, const std::array<bool,3>& weather,
                   const std::array<bool,3>& horn);
bool playCard(Game&, int playerIndex, const std::string& cardId, Row target = Row::None);
bool pass(Game&, int playerIndex);
bool startNextRound(Game&);
std::string validateDeck(const std::vector<Card>& deck, Faction faction);
}
