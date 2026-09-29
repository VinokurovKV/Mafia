#pragma once

#include <optional>
#include <stdexcept>

#include "mafia/types.hpp"

namespace mafia {

class DecisionStrategy {
public:
    virtual ~DecisionStrategy() = default;
    virtual PlayerId chooseTarget(const TurnContext& context) = 0;

    virtual void startDecision(const TurnContext& context) {
        if (pendingDecision_.has_value()) {
            throw std::logic_error("Strategy already has a pending decision");
        }
        pendingDecision_ = decide(context);
    }

    virtual bool decisionReady() {
        return pendingDecision_.has_value();
    }

    virtual StrategyDecision takeDecision() {
        if (!pendingDecision_.has_value()) {
            throw std::logic_error("Strategy decision is not ready");
        }
        StrategyDecision decision = std::move(*pendingDecision_);
        pendingDecision_.reset();
        return decision;
    }

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

private:
    std::optional<StrategyDecision> pendingDecision_;
};

}  // namespace mafia
