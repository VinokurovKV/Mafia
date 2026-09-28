#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "mafia/types.hpp"

namespace mafia {

struct LoggedPlayer {
    PlayerId id;
    std::string name;
    RoleType role;
};

class GameLogger {
public:
    explicit GameLogger(std::filesystem::path rootDirectory);

    void startGame(std::vector<LoggedPlayer> players);
    void logStep(int round, GamePhase phase, const StepResult& result);
    void finishGame(Winner winner, const std::vector<PlayerState>& states);

    const std::filesystem::path& sessionDirectory() const noexcept;

private:
    struct PlayerStatistics {
        std::size_t submittedActions = 0;
        std::size_t votesCast = 0;
        std::size_t votesReceived = 0;
        std::size_t mafiaTargets = 0;
        std::size_t heals = 0;
        std::size_t checks = 0;
        std::size_t shots = 0;
        std::size_t maniacTargets = 0;
        std::size_t listens = 0;
        std::size_t observations = 0;
        std::optional<int> eliminatedRound;
        std::optional<GamePhase> eliminatedPhase;
    };

    std::string playerLabel(PlayerId id) const;
    void updateStatistics(
        int round,
        GamePhase phase,
        const StepResult& result
    );

    std::filesystem::path rootDirectory_;
    std::filesystem::path sessionDirectory_;
    std::vector<LoggedPlayer> players_;
    std::unordered_map<PlayerId, PlayerStatistics> statistics_;
    bool started_ = false;
};

}  // namespace mafia
