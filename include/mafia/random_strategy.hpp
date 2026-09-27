#pragma once

#include <random>

#include "mafia/decision_strategy.hpp"

namespace mafia {

class RandomStrategy final : public DecisionStrategy {
public:
    RandomStrategy();
    explicit RandomStrategy(std::mt19937::result_type seed);

    PlayerId chooseTarget(const TurnContext& context) override;
    ActionType chooseActionType(const TurnContext& context) override;

private:
    std::mt19937 generator_;
};

}  // namespace mafia
