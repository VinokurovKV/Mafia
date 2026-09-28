#pragma once

#include <iosfwd>

#include "mafia/decision_strategy.hpp"

namespace mafia {

class ConsoleStrategy final : public DecisionStrategy {
public:
    ConsoleStrategy(std::istream& input, std::ostream& output);

    PlayerId chooseTarget(const TurnContext& context) override;
    ActionType chooseActionType(const TurnContext& context) override;
    bool isInteractive() const noexcept override;

private:
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace mafia
