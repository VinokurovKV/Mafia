#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "mafia/decision_strategy.hpp"
#include "mafia/llm_client.hpp"
#include "mafia/shared_ptr.hpp"

namespace mafia {

class AiRequestBudget {
public:
    explicit AiRequestBudget(std::size_t maximumPerRound = 10);
    bool tryConsume(int round) noexcept;
    std::size_t used() const noexcept;

private:
    std::size_t maximumPerRound_;
    int currentRound_ = -1;
    std::size_t used_ = 0;
};

class AiStrategy final : public DecisionStrategy {
public:
    AiStrategy(
        SharedPtr<LlmClient> client,
        SharedPtr<DecisionStrategy> fallback,
        SharedPtr<AiRequestBudget> budget,
        std::string personality
    );

    StrategyDecision decide(const TurnContext& context) override;
    void startDecision(const TurnContext& context) override;
    bool decisionReady() override;
    StrategyDecision takeDecision() override;
    PlayerId chooseTarget(const TurnContext& context) override;
    ActionType chooseActionType(const TurnContext& context) override;

private:
    std::string buildPrompt(const TurnContext& context) const;
    StrategyDecision parseResponse(std::string_view response) const;
    bool isValid(
        const StrategyDecision& decision,
        const TurnContext& context
    ) const noexcept;
    void startFallback(const TurnContext& context, std::string reason);

    enum class PendingMode {
        None,
        Llm,
        Fallback,
    };

    SharedPtr<LlmClient> client_;
    SharedPtr<DecisionStrategy> fallback_;
    SharedPtr<AiRequestBudget> budget_;
    std::string personality_;
    PendingMode pendingMode_ = PendingMode::None;
    std::optional<TurnContext> pendingContext_;
    std::optional<StrategyDecision> completedDecision_;
    std::string fallbackReason_;
};

}  // namespace mafia
