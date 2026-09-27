#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace mafia {

using PlayerId = std::size_t;
using StepId = std::size_t;

enum class GamePhase {
    Day,
    Voting,
    Night,
    Finished,
};

enum class ActionType {
    Vote,
    MafiaKill,
    Heal,
    Check,
    Shoot,
    ManiacKill,
};

enum class RoleType {
    Mafia,
    Civilian,
    Doctor,
    Commissioner,
    Maniac,
};

enum class Winner {
    Civilians,
    Mafia,
    Maniac,
};

enum class AnnouncementMode {
    Open,
    Closed,
};

enum class LogLevel {
    Brief,
    Full,
};

struct GameConfig {
    std::size_t playerCount = 0;
    std::size_t mafiaDivisor = 3;
    bool interactive = false;
    AnnouncementMode announcementMode = AnnouncementMode::Closed;
    LogLevel logLevel = LogLevel::Brief;
    std::string logDirectory = "logs";
};

struct PlayerState {
    PlayerId id;
    bool alive;
};

struct GameState {
    int round;
    GamePhase phase;
    std::vector<PlayerState> players;
    std::optional<PlayerId> lastDoctorTarget;
    std::optional<Winner> winner;
};

struct GameSnapshot {
    int round;
    GamePhase phase;
    std::vector<PlayerState> players;
    std::optional<PlayerId> lastDoctorTarget;
    std::optional<Winner> winner;
};

struct StepRequest {
    StepId id;
    GamePhase phase;
};

struct TurnContext {
    StepId stepId;
    GamePhase phase;
    std::vector<PlayerId> availableTargets;
    std::vector<ActionType> availableActions;
};

struct Action {
    StepId stepId;
    PlayerId actor;
    ActionType type;
    PlayerId target;
};

struct InvestigationResult {
    PlayerId investigator;
    PlayerId target;
    bool targetIsMafia;
};

struct StepResult {
    std::vector<PlayerId> eliminated;
    std::vector<Action> actions;
    std::vector<Action> actionHistory;
    std::optional<PlayerId> mafiaTarget;
    bool mafiaConsensusRequired = false;
    std::vector<InvestigationResult> investigations;
    std::optional<PlayerId> doctorTarget;
    std::chrono::system_clock::time_point completedAt;
};

}  // namespace mafia
