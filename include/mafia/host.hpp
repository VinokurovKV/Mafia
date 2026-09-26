#pragma once

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
    std::vector<Player*> players_;

    std::mutex actionsMutex_;
    std::condition_variable actionAvailable_;
    std::optional<StepId> activeStep_;
    std::unordered_set<PlayerId> expectedPlayers_;
    std::unordered_map<PlayerId, Action> actions_;
};

}  // namespace mafia
