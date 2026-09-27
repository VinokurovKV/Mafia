#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mafia/types.hpp"

namespace mafia {

class Player;

class Host {
public:
    void registerPlayer(Player& player);

    StepResult conductStep(
        const StepRequest& request,
        const GameSnapshot& state
    );

    void submitAction(const Action& action);

private:
    struct TurnAssignment {
        Player* player;
        TurnContext context;
    };

    struct CollectedActions {
        std::unordered_map<PlayerId, Action> actions;
        std::chrono::system_clock::time_point completedAt;
    };

    StepResult conductVoting(
        const StepRequest& request,
        const GameSnapshot& state
    );
    StepResult conductNight(
        const StepRequest& request,
        const GameSnapshot& state
    );
    CollectedActions collectActions(
        const StepRequest& request,
        const std::vector<TurnAssignment>& assignments
    );
    std::vector<Player*> livingPlayers(const GameSnapshot& state) const;
    Player* findPlayer(PlayerId id) const noexcept;

    std::vector<Player*> players_;

    std::mutex actionsMutex_;
    std::condition_variable actionAvailable_;
    std::optional<StepId> activeStep_;
    std::optional<std::chrono::system_clock::time_point> lastActionReceived_;
    std::unordered_set<PlayerId> expectedPlayers_;
    std::unordered_map<PlayerId, Action> actions_;
};

}  // namespace mafia
