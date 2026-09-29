#include "mafia/roles.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "mafia/role_concepts.hpp"

namespace mafia {
namespace {

ActionType actionTypeForPhase(
    GamePhase phase,
    ActionType nightAction,
    std::string_view roleName
) {
    if (phase == GamePhase::Voting) {
        return ActionType::Vote;
    }
    if (phase == GamePhase::Night) {
        return nightAction;
    }

    throw std::logic_error(
        std::string(roleName) + " cannot act during the current phase"
    );
}

template <PlayerRole Role>
Action makeTargetedAction(
    Role& player,
    DecisionStrategy& strategy,
    const TurnContext& context,
    ActionType type
) {
    StrategyDecision decision = strategy.decide(context);
    if (decision.action != type) {
        throw std::logic_error("Strategy selected an unavailable action");
    }
    return Action{
        context.stepId,
        player.id(),
        type,
        decision.target,
        std::move(decision.message),
        std::move(decision.reasoning),
    };
}

}  // namespace

Action Mafia::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::MafiaKill,
            "Mafia"
        )
    );
}

RoleType Mafia::role() const noexcept {
    return RoleType::Mafia;
}

Action Civilian::makeAction(const TurnContext& context) {
    if (context.phase != GamePhase::Voting) {
        throw std::logic_error("Civilian can act only during voting");
    }

    return makeTargetedAction(
        *this,
        strategy(),
        context,
        ActionType::Vote
    );
}

RoleType Civilian::role() const noexcept {
    return RoleType::Civilian;
}

Action Doctor::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(context.phase, ActionType::Heal, "Doctor")
    );
}

RoleType Doctor::role() const noexcept {
    return RoleType::Doctor;
}

Action Commissioner::makeAction(const TurnContext& context) {
    ActionType actionType = ActionType::Vote;
    if (context.phase == GamePhase::Night) {
        StrategyDecision decision = strategy().decide(context);
        actionType = decision.action;
        if (
            actionType != ActionType::Check &&
            actionType != ActionType::Shoot
        ) {
            throw std::logic_error(
                "Commissioner must choose Check or Shoot at night"
            );
        }
        if (
            std::ranges::find(context.availableActions, actionType) ==
                context.availableActions.end()
        ) {
            throw std::logic_error(
                "Commissioner selected an unavailable action"
            );
        }
        return Action{
            context.stepId,
            id(),
            actionType,
            decision.target,
            std::move(decision.message),
            std::move(decision.reasoning),
        };
    } else if (context.phase != GamePhase::Voting) {
        throw std::logic_error(
            "Commissioner cannot act during the current phase"
        );
    }

    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionType
    );
}

RoleType Commissioner::role() const noexcept {
    return RoleType::Commissioner;
}

Action Maniac::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::ManiacKill,
            "Maniac"
        )
    );
}

RoleType Maniac::role() const noexcept {
    return RoleType::Maniac;
}

Action Eavesdropper::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::Listen,
            "Eavesdropper"
        )
    );
}

RoleType Eavesdropper::role() const noexcept {
    return RoleType::Eavesdropper;
}

Action Witness::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::Observe,
            "Witness"
        )
    );
}

RoleType Witness::role() const noexcept {
    return RoleType::Witness;
}

Action Bull::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::MafiaKill,
            "Bull"
        )
    );
}

RoleType Bull::role() const noexcept {
    return RoleType::Bull;
}

}  // namespace mafia
