#pragma once

#include <stdexcept>

#include "mafia/types.hpp"

namespace mafia {

class DecisionStrategy {
public:
    virtual ~DecisionStrategy() = default;
    virtual PlayerId chooseTarget(const TurnContext& context) = 0;

    virtual StrategyDecision decide(const TurnContext& context) {
        if (context.availableActions.empty()) {
            throw std::logic_error("No action types are available");
        }
        const ActionType action = context.availableActions.size() == 1
            ? context.availableActions.front()
            : chooseActionType(context);
        return {
            action,
            chooseTarget(context),
            {},
            {},
        };
    }

    virtual bool isInteractive() const noexcept {
        return false;
    }

    virtual ActionType chooseActionType(const TurnContext& context) {
        if (context.availableActions.empty()) {
            throw std::logic_error("No action types are available");
        }
        return context.availableActions.front();
    }
};

}  // namespace mafia
