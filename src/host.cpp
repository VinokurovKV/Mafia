#include "mafia/host.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "mafia/player.hpp"

namespace mafia {
namespace {

template <typename T>
bool contains(const std::vector<T>& values, const T& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool isValidAction(
    const Action& action,
    const TurnContext& context
) {
    return contains(context.availableActions, action.type) &&
        contains(context.availableTargets, action.target);
}

}  // namespace

void Host::registerPlayer(Player& player) {
    if (findPlayer(player.id()) != nullptr) {
        throw std::logic_error("Player id is already registered");
    }

    players_.push_back(&player);
}

StepResult Host::conductStep(
    const StepRequest& request,
    const GameSnapshot& state
) {
    if (state.phase != request.phase) {
        throw std::logic_error("Step phase does not match game state");
    }

    switch (request.phase) {
        case GamePhase::Voting:
            return conductVoting(request, state);
        case GamePhase::Night:
            return conductNight(request, state);
        default:
            throw std::logic_error("Host does not support this phase yet");
    }
}

StepResult Host::conductVoting(
    const StepRequest& request,
    const GameSnapshot& state
) {
    const std::vector<Player*> voters = livingPlayers(state);
    std::vector<TurnAssignment> assignments;
    assignments.reserve(voters.size());

    for (Player* voter : voters) {
        std::vector<PlayerId> targets;
        for (Player* candidate : voters) {
            if (candidate->id() != voter->id()) {
                targets.push_back(candidate->id());
            }
        }

        assignments.push_back({
            voter,
            {
                request.id,
                GamePhase::Voting,
                std::move(targets),
                {ActionType::Vote},
            },
        });
    }

    const CollectedActions collected = collectActions(request, assignments);
    std::unordered_map<PlayerId, std::size_t> voteCounts;

    for (const TurnAssignment& assignment : assignments) {
        const auto action = collected.actions.find(assignment.player->id());
        if (
            action != collected.actions.end() &&
            isValidAction(action->second, assignment.context)
        ) {
            ++voteCounts[action->second.target];
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
    result.completedAt = collected.completedAt;
    for (const auto& [actor, action] : collected.actions) {
        static_cast<void>(actor);
        result.actions.push_back(action);
    }
    std::sort(
        result.actions.begin(),
        result.actions.end(),
        [](const Action& left, const Action& right) {
            return left.actor < right.actor;
        }
    );
    result.actionHistory = result.actions;
    if (selectedPlayer.has_value() && !tied) {
        result.eliminated.push_back(*selectedPlayer);
    }
    return result;
}

StepResult Host::conductNight(
    const StepRequest& request,
    const GameSnapshot& state
) {
    const std::vector<Player*> living = livingPlayers(state);
    std::vector<TurnAssignment> assignments;

    for (Player* actor : living) {
        std::vector<PlayerId> targets;
        std::vector<ActionType> actionTypes;

        switch (actor->role()) {
            case RoleType::Mafia:
                actionTypes = {ActionType::MafiaKill};
                for (Player* candidate : living) {
                    if (candidate->role() != RoleType::Mafia) {
                        targets.push_back(candidate->id());
                    }
                }
                break;

            case RoleType::Doctor:
                actionTypes = {ActionType::Heal};
                for (Player* candidate : living) {
                    if (
                        !state.lastDoctorTarget.has_value() ||
                        candidate->id() != *state.lastDoctorTarget
                    ) {
                        targets.push_back(candidate->id());
                    }
                }
                break;

            case RoleType::Commissioner:
                actionTypes = {ActionType::Check, ActionType::Shoot};
                for (Player* candidate : living) {
                    if (candidate->id() != actor->id()) {
                        targets.push_back(candidate->id());
                    }
                }
                break;

            case RoleType::Maniac:
                actionTypes = {ActionType::ManiacKill};
                for (Player* candidate : living) {
                    if (candidate->id() != actor->id()) {
                        targets.push_back(candidate->id());
                    }
                }
                break;

            case RoleType::Civilian:
                continue;
        }

        assignments.push_back({
            actor,
            {
                request.id,
                GamePhase::Night,
                std::move(targets),
                std::move(actionTypes),
            },
        });
    }

    CollectedActions collected = collectActions(request, assignments);
    std::vector<Action> actionHistory;
    actionHistory.reserve(collected.actions.size());
    for (const auto& [actor, action] : collected.actions) {
        static_cast<void>(actor);
        actionHistory.push_back(action);
    }
    std::sort(
        actionHistory.begin(),
        actionHistory.end(),
        [](const Action& left, const Action& right) {
            return left.actor < right.actor;
        }
    );

    std::vector<TurnAssignment> mafiaAssignments;
    for (const TurnAssignment& assignment : assignments) {
        if (assignment.player->role() != RoleType::Mafia) {
            continue;
        }

        mafiaAssignments.push_back(assignment);
        const auto proposal = collected.actions.find(assignment.player->id());
        if (
            proposal == collected.actions.end() ||
            !isValidAction(proposal->second, assignment.context) ||
            proposal->second.type != ActionType::MafiaKill
        ) {
            throw std::logic_error("Mafia player did not submit a valid target");
        }
    }

    std::optional<PlayerId> mafiaTarget;
    bool mafiaConsensusRequired = false;
    if (!mafiaAssignments.empty()) {
        const auto commonMafiaTarget = [&]() -> std::optional<PlayerId> {
            const PlayerId firstTarget = collected.actions.at(
                mafiaAssignments.front().player->id()
            ).target;
            const bool unanimous = std::all_of(
                mafiaAssignments.begin(),
                mafiaAssignments.end(),
                [&collected, firstTarget](const TurnAssignment& assignment) {
                    return collected.actions.at(
                        assignment.player->id()
                    ).target == firstTarget;
                }
            );
            return unanimous
                ? std::optional<PlayerId>{firstTarget}
                : std::nullopt;
        };

        mafiaTarget = commonMafiaTarget();
        while (!mafiaTarget.has_value()) {
            mafiaConsensusRequired = true;

            for (const TurnAssignment& assignment : mafiaAssignments) {
                std::vector<PlayerId> otherProposals;
                for (const TurnAssignment& other : mafiaAssignments) {
                    if (other.player->id() == assignment.player->id()) {
                        continue;
                    }

                    const PlayerId proposedTarget = collected.actions.at(
                        other.player->id()
                    ).target;
                    if (!contains(otherProposals, proposedTarget)) {
                        otherProposals.push_back(proposedTarget);
                    }
                }

                const TurnAssignment discussionTurn{
                    assignment.player,
                    {
                        request.id,
                        GamePhase::Night,
                        std::move(otherProposals),
                        {ActionType::MafiaKill},
                    },
                };
                const CollectedActions discussed = collectActions(
                    request,
                    {discussionTurn}
                );
                const auto action = discussed.actions.find(
                    assignment.player->id()
                );
                if (
                    action == discussed.actions.end() ||
                    !isValidAction(action->second, discussionTurn.context)
                ) {
                    throw std::logic_error(
                        "Mafia player did not submit a valid discussion vote"
                    );
                }

                actionHistory.push_back(action->second);
                collected.actions[assignment.player->id()] = action->second;
                collected.completedAt = discussed.completedAt;
                mafiaTarget = commonMafiaTarget();
                if (mafiaTarget.has_value()) {
                    break;
                }
            }
        }
    }

    StepResult result;
    result.completedAt = collected.completedAt;
    result.actionHistory = std::move(actionHistory);
    result.mafiaTarget = mafiaTarget;
    result.mafiaConsensusRequired = mafiaConsensusRequired;
    for (const auto& [actor, action] : collected.actions) {
        static_cast<void>(actor);
        result.actions.push_back(action);
    }
    std::sort(
        result.actions.begin(),
        result.actions.end(),
        [](const Action& left, const Action& right) {
            return left.actor < right.actor;
        }
    );

    std::unordered_set<PlayerId> attackedPlayers;
    std::vector<PlayerId> mafiaTargets;
    std::size_t validMafiaActions = 0;

    for (const TurnAssignment& assignment : assignments) {
        const auto received = collected.actions.find(assignment.player->id());
        if (
            received == collected.actions.end() ||
            !isValidAction(received->second, assignment.context)
        ) {
            continue;
        }

        const Action& action = received->second;
        switch (action.type) {
            case ActionType::MafiaKill:
                ++validMafiaActions;
                mafiaTargets.push_back(action.target);
                break;

            case ActionType::Heal:
                result.doctorTarget = action.target;
                break;

            case ActionType::Check: {
                const Player* target = findPlayer(action.target);
                result.investigations.push_back({
                    action.actor,
                    action.target,
                    target != nullptr && target->role() == RoleType::Mafia,
                });
                break;
            }

            case ActionType::Shoot:
            case ActionType::ManiacKill:
                attackedPlayers.insert(action.target);
                break;

            case ActionType::Vote:
                break;
        }
    }

    const std::size_t livingMafia = static_cast<std::size_t>(std::count_if(
        assignments.begin(),
        assignments.end(),
        [](const TurnAssignment& assignment) {
            return assignment.player->role() == RoleType::Mafia;
        }
    ));

    if (
        validMafiaActions == livingMafia &&
        !mafiaTargets.empty() &&
        std::all_of(
            mafiaTargets.begin(),
            mafiaTargets.end(),
            [&mafiaTargets](PlayerId target) {
                return target == mafiaTargets.front();
            }
        )
    ) {
        attackedPlayers.insert(mafiaTargets.front());
    }

    if (result.doctorTarget.has_value()) {
        attackedPlayers.erase(*result.doctorTarget);
    }

    result.eliminated.assign(
        attackedPlayers.begin(),
        attackedPlayers.end()
    );
    std::sort(result.eliminated.begin(), result.eliminated.end());
    return result;
}

Host::CollectedActions Host::collectActions(
    const StepRequest& request,
    const std::vector<TurnAssignment>& assignments
) {
    {
        std::lock_guard lock(actionsMutex_);
        if (activeStep_.has_value()) {
            throw std::logic_error("Another step is already active");
        }

        activeStep_ = request.id;
        lastActionReceived_.reset();
        expectedPlayers_.clear();
        actions_.clear();
        for (const TurnAssignment& assignment : assignments) {
            expectedPlayers_.insert(assignment.player->id());
        }
    }

    for (const TurnAssignment& assignment : assignments) {
        assignment.player->requestTurn(assignment.context);
    }

    CollectedActions collected;
    {
        std::unique_lock lock(actionsMutex_);
        actionAvailable_.wait(lock, [this] {
            return actions_.size() == expectedPlayers_.size();
        });

        collected.actions = std::move(actions_);
        collected.completedAt = lastActionReceived_.value_or(
            std::chrono::system_clock::now()
        );

        actions_.clear();
        expectedPlayers_.clear();
        activeStep_.reset();
        lastActionReceived_.reset();
    }
    return collected;
}

std::vector<Player*> Host::livingPlayers(const GameSnapshot& state) const {
    std::vector<Player*> living;

    for (const PlayerState& playerState : state.players) {
        if (!playerState.alive) {
            continue;
        }

        Player* player = findPlayer(playerState.id);
        if (player == nullptr) {
            throw std::logic_error("Alive player is not registered in Host");
        }
        living.push_back(player);
    }
    return living;
}

Player* Host::findPlayer(PlayerId id) const noexcept {
    const auto player = std::find_if(
        players_.begin(),
        players_.end(),
        [id](const Player* current) {
            return current->id() == id;
        }
    );
    return player == players_.end() ? nullptr : *player;
}

void Host::submitAction(const Action& action) {
    bool actionAccepted = false;
    {
        std::lock_guard lock(actionsMutex_);

        if (!activeStep_.has_value() || action.stepId != *activeStep_) {
            return;
        }
        if (!expectedPlayers_.contains(action.actor)) {
            return;
        }

        const auto [position, inserted] = actions_.try_emplace(
            action.actor,
            action
        );
        static_cast<void>(position);
        if (inserted) {
            lastActionReceived_ = std::chrono::system_clock::now();
            actionAccepted = true;
        }
    }

    if (actionAccepted) {
        actionAvailable_.notify_one();
    }
}

}  // namespace mafia
