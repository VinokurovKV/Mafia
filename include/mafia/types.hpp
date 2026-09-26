#pragma once

#include <cstddef>
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

struct PlayerState {
    PlayerId id;
    bool alive;
};

struct GameState {
    int round;
    GamePhase phase;
    std::vector<PlayerState> players;
};

struct GameSnapshot {
    int round;
    GamePhase phase;
    std::vector<PlayerState> players;
};

struct StepRequest {
    StepId id;
    GamePhase phase;
};

struct TurnContext {
    StepId stepId;
    GamePhase phase;
    std::vector<PlayerId> availableTargets;
};

struct Action {
    StepId stepId;
    PlayerId actor;
    ActionType type;
    PlayerId target;
};

struct StepResult {
    std::vector<PlayerId> eliminated;
};

}  // namespace mafia
