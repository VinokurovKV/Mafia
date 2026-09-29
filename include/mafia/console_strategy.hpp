#pragma once

#include <iosfwd>
#include <optional>
#include <string>

#include "mafia/decision_strategy.hpp"

namespace mafia {

class ConsoleStrategy final : public DecisionStrategy {
public:
    ConsoleStrategy(std::istream& input, std::ostream& output);

    PlayerId chooseTarget(const TurnContext& context) override;
    ActionType chooseActionType(const TurnContext& context) override;
    bool isInteractive() const noexcept override;
    void startDecision(const TurnContext& context) override;
    bool decisionReady() override;
    StrategyDecision takeDecision() override;

private:
    enum class InputStage {
        None,
        Action,
        Target,
        Ready,
    };

    void promptForTarget();
    std::optional<std::string> readAvailableLine();
    void useEndOfInputFallback();

    std::istream& input_;
    std::ostream& output_;
    std::optional<TurnContext> pendingContext_;
    std::optional<StrategyDecision> pendingDecision_;
    InputStage inputStage_ = InputStage::None;
    ActionType selectedAction_ = ActionType::Vote;
    bool inputClosed_ = false;
};

}  // namespace mafia
