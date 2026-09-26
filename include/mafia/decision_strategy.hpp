#pragma once

#include "mafia/types.hpp"

namespace mafia {

class DecisionStrategy {
public:
    virtual ~DecisionStrategy() = default;
    virtual PlayerId chooseTarget(const TurnContext& context) = 0;
};

}  // namespace mafia
