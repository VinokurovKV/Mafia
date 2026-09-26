#include "mafia/host.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "mafia/player.hpp"

namespace mafia {

void Host::registerPlayer(Player& player) {
    const auto registered = std::find_if(
        players_.begin(),
        players_.end(),
        [&player](const Player* current) {
            return current->id() == player.id();
        }
    );

    if (registered != players_.end()) {
        throw std::logic_error("Player id is already registered");
    }

    players_.push_back(&player);
}

StepResult Host::conductStep(
    const StepRequest& request,
    const GameSnapshot& state
) {
    if (request.phase != GamePhase::Voting) {
        throw std::logic_error("Host currently supports only voting");
    }
    if (state.phase != request.phase) {
        throw std::logic_error("Step phase does not match game state");
    }

    std::vector<Player*> voters;
    std::vector<PlayerId> alivePlayers;

    for (const PlayerState& playerState : state.players) {
        if (!playerState.alive) {
            continue;
        }

        const auto player = std::find_if(
            players_.begin(),
            players_.end(),
            [&playerState](const Player* current) {
                return current->id() == playerState.id;
            }
        );

        if (player == players_.end()) {
            throw std::logic_error("Alive player is not registered in Host");
        }

        voters.push_back(*player);
        alivePlayers.push_back(playerState.id);
    }

    {
        std::lock_guard lock(actionsMutex_);
        activeStep_ = request.id;
        expectedPlayers_.clear();
        actions_.clear();
        expectedPlayers_.insert(alivePlayers.begin(), alivePlayers.end());
    }

    for (Player* voter : voters) {
        std::vector<PlayerId> availableTargets;
        for (const PlayerId candidate : alivePlayers) {
            if (candidate != voter->id()) {
                availableTargets.push_back(candidate);
            }
        }

        voter->requestTurn({
            request.id,
            GamePhase::Voting,
            std::move(availableTargets),
        });
    }

    std::unordered_map<PlayerId, Action> receivedActions;
    {
        std::unique_lock lock(actionsMutex_);
        actionAvailable_.wait(lock, [this] {
            return actions_.size() == expectedPlayers_.size();
        });

        receivedActions = std::move(actions_);
        actions_.clear();
        expectedPlayers_.clear();
        activeStep_.reset();
    }

    std::unordered_map<PlayerId, std::size_t> voteCounts;
    for (const auto& [actor, action] : receivedActions) {
        const bool targetIsAlive = std::find(
            alivePlayers.begin(),
            alivePlayers.end(),
            action.target
        ) != alivePlayers.end();

        if (
            action.type == ActionType::Vote &&
            action.target != actor &&
            targetIsAlive
        ) {
            ++voteCounts[action.target];
        }
    }

    std::optional<PlayerId> selectedPlayer;
    std::size_t maximumVotes = 0;
    bool tied = false;

    for (const auto& [candidate, votes] : voteCounts) {
        if (votes > maximumVotes) {
            maximumVotes = votes;
            selectedPlayer = candidate;
            tied = false;
        } else if (votes == maximumVotes) {
            tied = true;
        }
    }

    StepResult result;
    if (selectedPlayer.has_value() && !tied) {
        result.eliminated.push_back(*selectedPlayer);
    }
    return result;
}

void Host::submitAction(const Action& action) {
    {
        std::lock_guard lock(actionsMutex_);

        if (!activeStep_.has_value() || action.stepId != *activeStep_) {
            return;
        }
        if (!expectedPlayers_.contains(action.actor)) {
            return;
        }

        actions_.try_emplace(action.actor, action);
    }
    actionAvailable_.notify_one();
}

}  // namespace mafia
