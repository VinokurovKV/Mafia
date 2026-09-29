#include "mafia/ai_strategy.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

class FakeLlmClient final : public mafia::LlmClient {
public:
    explicit FakeLlmClient(std::string response)
        : response_(std::move(response)) {}

    std::string complete(std::string_view prompt) override {
        ++requestCount_;
        lastPrompt_ = prompt;
        return response_;
    }

    std::size_t requestCount() const noexcept {
        return requestCount_;
    }

    const std::string& lastPrompt() const noexcept {
        return lastPrompt_;
    }

private:
    std::string response_;
    std::string lastPrompt_;
    std::size_t requestCount_ = 0;
};

class ThrowingLlmClient final : public mafia::LlmClient {
public:
    std::string complete(std::string_view) override {
        ++requestCount_;
        throw std::runtime_error("simulated network failure");
    }

    std::size_t requestCount() const noexcept {
        return requestCount_;
    }

private:
    std::size_t requestCount_ = 0;
};

class FixedFallback final : public mafia::DecisionStrategy {
public:
    explicit FixedFallback(mafia::PlayerId target) : target_(target) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext&) override {
        return target_;
    }

private:
    mafia::PlayerId target_;
};

mafia::TurnContext votingContext(int round = 2) {
    return {
        12,
        mafia::GamePhase::Voting,
        {2, 4, 5},
        {mafia::ActionType::Vote},
        {
            round,
            3,
            mafia::RoleType::Commissioner,
            {},
            {2, 3, 4, 5},
            {"Player 2 voted for Player 5"},
            {"Investigation: Player 4 is mafia"},
        },
    };
}

void testValidResponseAndPrompt() {
    auto* fake = new FakeLlmClient(
        "```json\n{\"action\":\"vote\","
        "\"message\":\"Player 4 looks suspicious.\","
        "\"target\":4,"
        "\"reasoning\":\"The private check confirms it.\"}\n```"
    );
    mafia::AiStrategy strategy(
        mafia::SharedPtr<mafia::LlmClient>(fake),
        mafia::SharedPtr<mafia::DecisionStrategy>(new FixedFallback(2)),
        mafia::makeShared<mafia::AiRequestBudget>(10),
        "Careful and concise"
    );

    const mafia::StrategyDecision decision = strategy.decide(votingContext());
    assert(decision.action == mafia::ActionType::Vote);
    assert(decision.target == 4);
    assert(decision.message == "Player 4 looks suspicious.");
    assert(decision.reasoning == "The private check confirms it.");
    assert(fake->requestCount() == 1);
    assert(fake->lastPrompt().find("Your role: Commissioner") != std::string::npos);
    assert(fake->lastPrompt().find("Investigation: Player 4 is mafia") != std::string::npos);
    assert(fake->lastPrompt().find("Valid voting targets: 2 4 5") != std::string::npos);
}

void testInvalidResponseUsesFallback() {
    auto* fake = new FakeLlmClient(
        R"({"action":"vote","message":"Vote.","target":99,"reasoning":"Guess."})"
    );
    mafia::AiStrategy strategy(
        mafia::SharedPtr<mafia::LlmClient>(fake),
        mafia::SharedPtr<mafia::DecisionStrategy>(new FixedFallback(2)),
        mafia::makeShared<mafia::AiRequestBudget>(10),
        "Cautious"
    );

    const mafia::StrategyDecision decision = strategy.decide(votingContext());
    assert(decision.target == 2);
    assert(decision.reasoning.find("AI fallback:") == 0);
    assert(fake->requestCount() == 1);
}

void testExceptionAndBudgetUseFallback() {
    auto* throwing = new ThrowingLlmClient;
    mafia::AiStrategy strategy(
        mafia::SharedPtr<mafia::LlmClient>(throwing),
        mafia::SharedPtr<mafia::DecisionStrategy>(new FixedFallback(5)),
        mafia::makeShared<mafia::AiRequestBudget>(1),
        "Direct"
    );

    const mafia::StrategyDecision first = strategy.decide(votingContext());
    const mafia::StrategyDecision second = strategy.decide(votingContext());
    assert(first.target == 5);
    assert(second.target == 5);
    assert(throwing->requestCount() == 1);
    assert(second.reasoning.find("request limit reached") != std::string::npos);
}

void testNightActionNeverCallsLlm() {
    auto* fake = new FakeLlmClient("not used");
    mafia::AiStrategy strategy(
        mafia::SharedPtr<mafia::LlmClient>(fake),
        mafia::SharedPtr<mafia::DecisionStrategy>(new FixedFallback(2)),
        mafia::makeShared<mafia::AiRequestBudget>(10),
        "Quiet"
    );
    mafia::TurnContext context = votingContext();
    context.phase = mafia::GamePhase::Night;
    context.availableActions = {mafia::ActionType::Heal};

    const mafia::StrategyDecision decision = strategy.decide(context);
    assert(decision.action == mafia::ActionType::Heal);
    assert(decision.target == 2);
    assert(fake->requestCount() == 0);
}

}  // namespace

int main() {
    testValidResponseAndPrompt();
    testInvalidResponseUsesFallback();
    testExceptionAndBudgetUseFallback();
    testNightActionNeverCallsLlm();
}
