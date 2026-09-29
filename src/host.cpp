#include "mafia/host.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>

#include "mafia/player.hpp"

namespace mafia {
namespace {

template <typename T>
bool contains(const std::vector<T>& values, const T& value) {
    return std::ranges::find(values, value) != values.end();
}

template <typename Predicate>
std::vector<PlayerId> playerIds(
    const std::vector<Player*>& players,
    Predicate predicate
) {
    auto ids = players
        | std::views::filter(std::move(predicate))
        | std::views::transform([](const Player* player) {
              return player->id();
          });

    std::vector<PlayerId> result;
    result.reserve(players.size());
    for (const PlayerId id : ids) {
        result.push_back(id);
    }
    return result;
}

bool isValidAction(
    const Action& action,
    const TurnContext& context
) {
    return contains(context.availableActions, action.type) &&
        contains(context.availableTargets, action.target);
}

std::string_view actionName(ActionType action) {
    switch (action) {
        case ActionType::Vote: return "vote";
        case ActionType::MafiaKill: return "mafia kill";
        case ActionType::Heal: return "heal";
        case ActionType::Check: return "check";
        case ActionType::Shoot: return "commissioner shot";
        case ActionType::ManiacKill: return "maniac attack";
        case ActionType::Listen: return "eavesdropping";
        case ActionType::Observe: return "witness observation";
    }
    return "unknown action";
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
        std::vector<PlayerId> targets = playerIds(
            voters,
            [voter](const Player* candidate) {
                return candidate->id() != voter->id();
            }
        );

        assignments.push_back({
            voter,
            {
                request.id,
                GamePhase::Voting,
                std::move(targets),
                {ActionType::Vote},
                makeAgentContext(*voter, state),
            },
        });
    }

    const CollectedActions collected = collectActions(request, assignments);
    std::unordered_map<PlayerId, std::size_t> voteCounts;

    std::ranges::for_each(assignments, [&](const TurnAssignment& assignment) {
        const auto action = collected.actions.find(assignment.player->id());
        if (
            action != collected.actions.end() &&
            isValidAction(action->second, assignment.context)
        ) {
            ++voteCounts[action->second.target];
        }
    });

    std::optional<PlayerId> selectedPlayer;
    bool tied = false;
    const auto maximum = std::ranges::max_element(
        voteCounts,
        {},
        [](const auto& entry) {
            return entry.second;
        }
    );
    if (maximum != voteCounts.end()) {
        selectedPlayer = maximum->first;
        tied = std::ranges::count_if(
            voteCounts,
            [maximum](const auto& entry) {
                return entry.second == maximum->second;
            }
        ) > 1;
    }

    StepResult result;
    result.completedAt = collected.completedAt;
    for (const auto& [actor, action] : collected.actions) {
        static_cast<void>(actor);
        result.actions.push_back(action);
    }
    std::ranges::sort(
        result.actions,
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
            case RoleType::Bull:
                actionTypes = {ActionType::MafiaKill};
                targets = playerIds(
                    living,
                    [](const Player* candidate) {
                        return !isMafiaRole(candidate->role());
                    }
                );
                break;

            case RoleType::Doctor:
                actionTypes = {ActionType::Heal};
                targets = playerIds(
                    living,
                    [&state](const Player* candidate) {
                        return !state.lastDoctorTarget.has_value() ||
                            candidate->id() != *state.lastDoctorTarget;
                    }
                );
                break;

            case RoleType::Commissioner:
                actionTypes = {ActionType::Check, ActionType::Shoot};
                targets = playerIds(
                    living,
                    [actor](const Player* candidate) {
                        return candidate->id() != actor->id();
                    }
                );
                break;

            case RoleType::Maniac:
                actionTypes = {ActionType::ManiacKill};
                targets = playerIds(
                    living,
                    [actor](const Player* candidate) {
                        return candidate->id() != actor->id();
                    }
                );
                break;

            case RoleType::Eavesdropper:
                actionTypes = {ActionType::Listen};
                targets = playerIds(
                    living,
                    [actor](const Player* candidate) {
                        return candidate->id() != actor->id();
                    }
                );
                break;

            case RoleType::Witness:
                actionTypes = {ActionType::Observe};
                targets = playerIds(
                    living,
                    [actor](const Player* candidate) {
                        return candidate->id() != actor->id();
                    }
                );
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
                makeAgentContext(*actor, state),
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
    std::ranges::sort(
        actionHistory,
        [](const Action& left, const Action& right) {
            return left.actor < right.actor;
        }
    );

    std::vector<TurnAssignment> mafiaAssignments;
    auto mafiaTurns = assignments | std::views::filter(
        [](const TurnAssignment& assignment) {
            return isMafiaRole(assignment.player->role());
        }
    );
    for (const TurnAssignment& assignment : mafiaTurns) {
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
            const bool unanimous = std::ranges::all_of(
                mafiaAssignments,
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
                auto proposals = mafiaAssignments
                    | std::views::filter(
                          [&assignment](const TurnAssignment& other) {
                              return other.player->id() !=
                                  assignment.player->id();
                          }
                      )
                    | std::views::transform(
                          [&collected](const TurnAssignment& other) {
                              return collected.actions.at(
                                  other.player->id()
                              ).target;
                          }
                      );
                for (const PlayerId proposedTarget : proposals) {
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
                        makeAgentContext(*assignment.player, state),
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
    std::ranges::sort(
        result.actions,
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
                    target != nullptr && isMafiaRole(target->role()),
                });
                break;
            }

            case ActionType::Shoot:
                attackedPlayers.insert(action.target);
                break;

            case ActionType::ManiacKill: {
                const Player* target = findPlayer(action.target);
                if (target == nullptr || target->role() != RoleType::Bull) {
                    attackedPlayers.insert(action.target);
                }
                break;
            }

            case ActionType::Listen:
            case ActionType::Observe:
                break;

            case ActionType::Vote:
                break;
        }
    }

    const std::size_t livingMafia = static_cast<std::size_t>(
        std::ranges::count_if(
            assignments,
            [](const TurnAssignment& assignment) {
                return isMafiaRole(assignment.player->role());
            }
        )
    );

    if (
        validMafiaActions == livingMafia &&
        !mafiaTargets.empty() &&
        std::ranges::all_of(
            mafiaTargets,
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

    for (const Action& observerAction : result.actions) {
        if (observerAction.type == ActionType::Listen) {
            EavesdropResult observation{
                observerAction.actor,
                observerAction.target,
                {},
            };
            for (const Action& directed : result.actions) {
                if (
                    directed.actor != observerAction.actor &&
                    directed.target == observerAction.target &&
                    !contains(observation.directedActions, directed.type)
                ) {
                    observation.directedActions.push_back(directed.type);
                }
            }
            result.eavesdropResults.push_back(std::move(observation));
        } else if (observerAction.type == ActionType::Observe) {
            WitnessResult observation{
                observerAction.actor,
                observerAction.target,
                {},
            };
            for (const Action& directed : result.actions) {
                const bool isAttack =
                    directed.type == ActionType::Shoot ||
                    directed.type == ActionType::ManiacKill ||
                    (directed.type == ActionType::MafiaKill &&
                     result.mafiaTarget == directed.target);
                if (
                    isAttack &&
                    directed.target == observerAction.target &&
                    !contains(observation.attackers, directed.actor)
                ) {
                    observation.attackers.push_back(directed.actor);
                }
            }
            result.witnessResults.push_back(std::move(observation));
        }
    }

    result.eliminated.assign(
        attackedPlayers.begin(),
        attackedPlayers.end()
    );
    std::ranges::sort(result.eliminated);
    rememberPrivateResults(result);
    return result;
}

Host::CollectedActions Host::collectActions(
    const StepRequest& request,
    const std::vector<TurnAssignment>& assignments
) {
    for (const TurnAssignment& assignment : assignments) {
        assignment.player->requestTurn(assignment.context);
    }

    CollectedActions collected;
    collected.completedAt = std::chrono::system_clock::now();

    std::vector<bool> completed(assignments.size(), false);
    std::size_t remaining = assignments.size();
    while (remaining > 0) {
        bool madeProgress = false;
        for (const bool interactive : {false, true}) {
            for (std::size_t index = 0; index < assignments.size(); ++index) {
                if (completed[index]) {
                    continue;
                }
                const TurnAssignment& assignment = assignments[index];
                if (assignment.player->isInteractive() != interactive) {
                    continue;
                }

                std::optional<Action> polled = assignment.player->pollTurn();
                if (!polled.has_value()) {
                    continue;
                }
                Action action = std::move(*polled);
                if (
                    action.stepId != request.id ||
                    action.actor != assignment.player->id()
                ) {
                    throw std::logic_error(
                        "Player coroutine returned an action for another turn"
                    );
                }

                const auto [position, inserted] =
                    collected.actions.try_emplace(
                        action.actor,
                        std::move(action)
                    );
                static_cast<void>(position);
                if (!inserted) {
                    throw std::logic_error(
                        "Player coroutine submitted more than one action"
                    );
                }
                collected.completedAt = std::chrono::system_clock::now();
                completed[index] = true;
                --remaining;
                madeProgress = true;
            }
        }
        if (!madeProgress && remaining > 0) {
            std::this_thread::yield();
        }
    }
    return collected;
}

std::vector<Player*> Host::livingPlayers(const GameSnapshot& state) const {
    std::vector<Player*> living;

    auto livingStates = state.players | std::views::filter(
        [](const PlayerState& playerState) {
            return playerState.alive;
        }
    );
    for (const PlayerState& playerState : livingStates) {
        Player* player = findPlayer(playerState.id);
        if (player == nullptr) {
            throw std::logic_error("Alive player is not registered in Host");
        }
        living.push_back(player);
    }
    return living;
}

Player* Host::findPlayer(PlayerId id) const noexcept {
    const auto player = std::ranges::find_if(
        players_,
        [id](const Player* current) {
            return current->id() == id;
        }
    );
    return player == players_.end() ? nullptr : *player;
}

AgentContext Host::makeAgentContext(
    const Player& player,
    const GameSnapshot& state
) const {
    AgentContext context;
    context.round = state.round;
    context.selfId = player.id();
    context.role = player.role();
    context.publicHistory = state.publicHistory;
    for (const PlayerState& current : state.players) {
        if (current.alive) {
            context.livingPlayers.push_back(current.id);
        } else {
            context.eliminatedPlayers.push_back(current.id);
        }
    }

    const auto remembered = privateKnowledge_.find(player.id());
    if (remembered != privateKnowledge_.end()) {
        context.privateKnowledge = remembered->second;
    }
    if (isMafiaRole(player.role())) {
        std::string teammates = "Known mafia teammates:";
        for (const Player* candidate : players_) {
            if (
                candidate->id() != player.id() &&
                isMafiaRole(candidate->role())
            ) {
                teammates += " Player " + std::to_string(candidate->id());
            }
        }
        context.privateKnowledge.push_back(std::move(teammates));
    }
    return context;
}

void Host::rememberPrivateResults(const StepResult& result) {
    for (const InvestigationResult& investigation : result.investigations) {
        privateKnowledge_[investigation.investigator].push_back(
            "Investigation: Player " + std::to_string(investigation.target) +
            (investigation.targetIsMafia ? " is mafia" : " is not mafia")
        );
    }
    for (const EavesdropResult& observation : result.eavesdropResults) {
        std::string fact =
            "Eavesdropping at Player " + std::to_string(observation.target) +
            ":";
        if (observation.directedActions.empty()) {
            fact += " no directed actions";
        } else {
            for (const ActionType action : observation.directedActions) {
                fact += " " + std::string(actionName(action));
            }
        }
        privateKnowledge_[observation.listener].push_back(std::move(fact));
    }
    for (const WitnessResult& observation : result.witnessResults) {
        std::string fact =
            "Witnessed at Player " + std::to_string(observation.target) +
            ":";
        if (observation.attackers.empty()) {
            fact += " no attack";
        } else {
            fact += " attackers";
            for (const PlayerId attacker : observation.attackers) {
                fact += " Player " + std::to_string(attacker);
            }
        }
        privateKnowledge_[observation.witness].push_back(std::move(fact));
    }
}

}  // namespace mafia
