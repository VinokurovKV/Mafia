#pragma once

#include <stdexcept>

#include "mafia/types.hpp"

namespace mafia {

class DecisionStrategy {
public:
    virtual ~DecisionStrategy() = default;
    virtual PlayerId chooseTarget(const TurnContext& context) = 0;

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
