#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
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
    Listen,
    Observe,
};

enum class RoleType {
    Mafia,
    Civilian,
    Doctor,
    Commissioner,
    Maniac,
    Eavesdropper,
    Witness,
    Bull,
};

constexpr bool isMafiaRole(RoleType role) noexcept {
    return role == RoleType::Mafia || role == RoleType::Bull;
}

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
    std::string roleConfigFile;
    bool aiEnabled = false;
    std::size_t aiPlayerCount = 1;
    std::string aiBaseUrl = "https://vireonix.ai/v1";
    std::string aiModel = "auto";
    std::string aiApiKeyEnvironment = "MAFIA_AI_API_KEY";
    std::string aiPersonality =
        "Cautious, concise, and willing to revise suspicions";
    int aiTimeoutSeconds = 20;
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
    std::vector<std::string> publicHistory;
};

struct GameSnapshot {
    GameSnapshot(
        int roundValue,
        GamePhase phaseValue,
        std::vector<PlayerState> playerValues,
        std::optional<PlayerId> doctorTarget,
        std::optional<Winner> winnerValue,
        std::vector<std::string> history = {}
    )
        : round(roundValue),
          phase(phaseValue),
          players(std::move(playerValues)),
          lastDoctorTarget(doctorTarget),
          winner(winnerValue),
          publicHistory(std::move(history)) {}

    int round;
    GamePhase phase;
    std::vector<PlayerState> players;
    std::optional<PlayerId> lastDoctorTarget;
    std::optional<Winner> winner;
    std::vector<std::string> publicHistory;
};

struct StepRequest {
    StepId id;
    GamePhase phase;
};

struct AgentContext {
    int round = 0;
    PlayerId selfId = 0;
    RoleType role = RoleType::Civilian;
    std::string personality;
    std::vector<PlayerId> livingPlayers;
    std::vector<PlayerId> eliminatedPlayers;
    std::vector<std::string> publicHistory;
    std::vector<std::string> privateKnowledge;
};

struct TurnContext {
    TurnContext(
        StepId step,
        GamePhase phaseValue,
        std::vector<PlayerId> targets,
        std::vector<ActionType> actions,
        AgentContext agentValue = {}
    )
        : stepId(step),
          phase(phaseValue),
          availableTargets(std::move(targets)),
          availableActions(std::move(actions)),
          agent(std::move(agentValue)) {}

    StepId stepId;
    GamePhase phase;
    std::vector<PlayerId> availableTargets;
    std::vector<ActionType> availableActions;
    AgentContext agent;
};

struct StrategyDecision {
    ActionType action = ActionType::Vote;
    PlayerId target = 0;
    std::string message;
    std::string reasoning;
};

struct Action {
    Action() = default;
    Action(
        StepId step,
        PlayerId actorValue,
        ActionType action,
        PlayerId targetValue,
        std::string messageValue = {},
        std::string reasoningValue = {}
    )
        : stepId(step),
          actor(actorValue),
          type(action),
          target(targetValue),
          message(std::move(messageValue)),
          reasoning(std::move(reasoningValue)) {}

    StepId stepId = 0;
    PlayerId actor = 0;
    ActionType type = ActionType::Vote;
    PlayerId target = 0;
    std::string message;
    std::string reasoning;
};

struct InvestigationResult {
    PlayerId investigator;
    PlayerId target;
    bool targetIsMafia;
};

struct EavesdropResult {
    PlayerId listener;
    PlayerId target;
    std::vector<ActionType> directedActions;
};

struct WitnessResult {
    PlayerId witness;
    PlayerId target;
    std::vector<PlayerId> attackers;
};

struct StepResult {
    std::vector<PlayerId> eliminated;
    std::vector<Action> actions;
    std::vector<Action> actionHistory;
    std::optional<PlayerId> mafiaTarget;
    bool mafiaConsensusRequired = false;
    std::vector<InvestigationResult> investigations;
    std::vector<EavesdropResult> eavesdropResults;
    std::vector<WitnessResult> witnessResults;
    std::optional<PlayerId> doctorTarget;
    std::chrono::system_clock::time_point completedAt;
};

}  // namespace mafia
