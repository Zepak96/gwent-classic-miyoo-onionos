#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace gwent {
enum class Row : std::uint8_t { Melee, Ranged, Siege, None };
enum class Kind : std::uint8_t { Unit, Leader, Weather, ClearWeather, CommanderHorn, Scorch, Mardroeme, Decoy };
enum class Faction : std::uint8_t { NorthernRealms, Nilfgaard, Scoiatael, Monsters, Skellige, Neutral };
enum class Phase : std::uint8_t { Mulligan, RoundSetup, PlayerTurn, OpponentTurn, RoundResult, MatchResult };

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
  bool muster{false};
  bool berserker{false};
  std::string group;
  std::vector<std::string> abilities;
  int referenceIndex{-1};
  int deckCopies{1};
};
struct ActiveWeather { Card card; int owner{0}; };
struct Player {
  std::vector<Card> deck, hand, graveyard;
  std::array<std::vector<Card>, 3> board;
  Card leader;
  bool passed{false};
  bool mulliganDone{false};
  bool leaderAbilityAvailable{false};
  bool leaderAbilityCancelled{false};
  bool leaderPassivesApplied{false};
  bool halfWeather{false};
  bool doubleSpyPower{false};
  std::array<bool, 3> mardroeme{false, false, false};
  std::array<bool, 3> specialOccupied{false, false, false};
  // Physical cards occupying the row's special slot. Leader Horn abilities
  // only set their effects and therefore leave this slot empty.
  std::array<Card, 3> specialCards{};
  int score{0};
  int roundWins{0};
  int roundLosses{0};
  Faction faction{Faction::NorthernRealms};
};
struct Game {
  std::array<Player, 2> players;
  Phase phase{Phase::PlayerTurn};
  int currentPlayer{0};
  int firstPlayer{0};
  int matchWinner{-1}; // -1 while active; 0/1 for winner; 2 for a drawn match.
  int round{1};
  std::array<bool, 3> weather{false, false, false};
  std::array<std::array<bool, 3>, 2> horn{{{false, false, false}, {false, false, false}}};
  std::vector<ActiveWeather> weatherCards;
  bool randomRespawn{false};
  int pendingMedicPlayer{-1};
  std::string pendingRandomMedicId;
  std::string pendingMedicAgileId;
  bool pendingMedicDuringRoundSetup{false};
  int skelligeRoundOwner{-1};
  int skelligeRoundRemaining{0};
  std::vector<Card> skelligeAiReviveQueue;
  bool scoiataelChoicePending{false};
  int skelligeRevivesRemaining{0};
  int skelligeRevivePlayer{-1};
  std::vector<std::string> skelligeAgileReviveIds;
  bool factionAbilitiesInitialized{false};
  std::array<std::vector<Card>, 2> revealedCards;
  std::uint64_t rngState{1};
  std::string lastError;
};

int calculateScore(const Player&, const std::array<bool,3>& weather,
                   const std::array<bool,3>& horn);
int calculateCardPower(const Card&, const std::vector<Card>& row, bool weather, bool horn,
                       bool halfWeather = false, bool doubleSpyPower = false);
bool playCard(Game&, int playerIndex, const std::string& cardId, Row target = Row::None,
              const std::string& selectedCardId = {});
bool chooseMedicRevive(Game&, int playerIndex, const std::string& cardId, Row target = Row::None);
bool cancelMedicRevive(Game&, int playerIndex);
// Preview the random unit a Medic will revive without advancing match RNG.
std::string previewRandomMedicTargetId(const Game&, int playerIndex);
void beginMatch(Game&, std::vector<Card> playerDeck, Faction playerFaction,
                std::vector<Card> opponentDeck, Faction opponentFaction,
                std::uint64_t seed, int firstPlayer = -1);
void beginMatchWithLeaders(Game&, std::vector<Card> playerDeck, Card playerLeader,
                           std::vector<Card> opponentDeck, Card opponentLeader,
                           std::uint64_t seed, int firstPlayer = -1);
// Match the source Hand/HandAI insertion rules when a card is drawn or returned.
void insertSourceHandCard(Player&, Card, int ownerIndex, int requestedIndex = -1);
bool assignLeader(Game&, int playerIndex, const Card& leader);
void resolveLeaderPassives(Game&);
bool chooseScoiataelFirstPlayer(Game&, int firstPlayer);
bool activateLeader(Game&, int playerIndex, Row target = Row::None,
                    const std::vector<std::string>& selectedCardIds = {});
bool replaceMulliganCards(Game&, int playerIndex, const std::vector<std::string>& cardIds);
bool pass(Game&, int playerIndex);
bool startNextRound(Game&);
bool chooseSkelligeRevive(Game&, int playerIndex, const std::string& cardId, Row target = Row::None);
std::string validateDeck(const std::vector<Card>& deck, Faction faction);
std::string validateDeck(const std::vector<Card>& deck, const Card& leader, Faction faction);
const std::vector<Card>& referenceCardCatalog();
std::vector<Card> makeReferenceDeck(const std::vector<std::pair<int, int>>& cardCounts);
}
