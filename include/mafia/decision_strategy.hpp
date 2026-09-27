#pragma once

#include <stdexcept>

#include "mafia/types.hpp"

namespace mafia {

class DecisionStrategy {
public:
    virtual ~DecisionStrategy() = default;
    virtual PlayerId chooseTarget(const TurnContext& context) = 0;

    virtual ActionType chooseActionType(const TurnContext& context) {
        if (context.availableActions.empty()) {
            throw std::logic_error("No action types are available");
        }
        return context.availableActions.front();
    }
};

}  // namespace mafia
