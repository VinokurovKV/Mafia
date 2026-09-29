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
    const TurnContext& context,
    ActionType type,
    StrategyDecision decision
) {
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

Action Mafia::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    return makeTargetedAction(
        *this,
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::MafiaKill,
            "Mafia"
        ),
        std::move(decision)
    );
}

RoleType Mafia::role() const noexcept {
    return RoleType::Mafia;
}

Action Civilian::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    if (context.phase != GamePhase::Voting) {
        throw std::logic_error("Civilian can act only during voting");
    }

    return makeTargetedAction(
        *this,
        context,
        ActionType::Vote,
        std::move(decision)
    );
}

RoleType Civilian::role() const noexcept {
    return RoleType::Civilian;
}

Action Doctor::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    return makeTargetedAction(
        *this,
        context,
        actionTypeForPhase(context.phase, ActionType::Heal, "Doctor"),
        std::move(decision)
    );
}

RoleType Doctor::role() const noexcept {
    return RoleType::Doctor;
}

Action Commissioner::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    ActionType actionType = ActionType::Vote;
    if (context.phase == GamePhase::Night) {
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
        context,
        actionType,
        std::move(decision)
    );
}

RoleType Commissioner::role() const noexcept {
    return RoleType::Commissioner;
}

Action Maniac::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    return makeTargetedAction(
        *this,
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::ManiacKill,
            "Maniac"
        ),
        std::move(decision)
    );
}

RoleType Maniac::role() const noexcept {
    return RoleType::Maniac;
}

Action Eavesdropper::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    return makeTargetedAction(
        *this,
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::Listen,
            "Eavesdropper"
        ),
        std::move(decision)
    );
}

RoleType Eavesdropper::role() const noexcept {
    return RoleType::Eavesdropper;
}

Action Witness::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    return makeTargetedAction(
        *this,
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::Observe,
            "Witness"
        ),
        std::move(decision)
    );
}

RoleType Witness::role() const noexcept {
    return RoleType::Witness;
}

Action Bull::formAction(
    const TurnContext& context,
    StrategyDecision decision
) {
    return makeTargetedAction(
        *this,
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::MafiaKill,
            "Bull"
        ),
        std::move(decision)
    );
}

RoleType Bull::role() const noexcept {
    return RoleType::Bull;
}

}  // namespace mafia
