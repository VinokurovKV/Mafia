#include "mafia/roles.hpp"

#include <stdexcept>
#include <string_view>

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

Action makeTargetedAction(
    Player& player,
    DecisionStrategy& strategy,
    const TurnContext& context,
    ActionType type
) {
    return Action{
        context.stepId,
        player.id(),
        type,
        strategy.chooseTarget(context),
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

Action Doctor::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(context.phase, ActionType::Heal, "Doctor")
    );
}

Action Commissioner::makeAction(const TurnContext& context) {
    return makeTargetedAction(
        *this,
        strategy(),
        context,
        actionTypeForPhase(
            context.phase,
            ActionType::Check,
            "Commissioner"
        )
    );
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

}  // namespace mafia
