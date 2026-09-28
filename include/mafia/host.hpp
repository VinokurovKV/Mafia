#pragma once

#include <chrono>
#include <unordered_map>
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
};

}  // namespace mafia
