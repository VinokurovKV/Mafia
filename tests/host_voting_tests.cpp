#include "mafia/host.hpp"
#include "mafia/roles.hpp"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace {

class FixedStrategy final : public mafia::DecisionStrategy {
public:
    explicit FixedStrategy(mafia::PlayerId target)
        : target_(target) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext& context) override {
        bool targetIsAvailable = false;
        for (const mafia::PlayerId candidate : context.availableTargets) {
            if (candidate == target_) {
                targetIsAvailable = true;
            }
        }
        assert(targetIsAvailable);
        return target_;
    }

private:
    mafia::PlayerId target_;
};

class OrderedStrategy final : public mafia::DecisionStrategy {
public:
    OrderedStrategy(
        mafia::PlayerId target,
        std::string label,
        std::vector<std::string>& executionOrder,
        bool interactive
    )
        : target_(target),
          label_(std::move(label)),
          executionOrder_(executionOrder),
          interactive_(interactive) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext&) override {
        executionOrder_.push_back(label_);
        return target_;
    }

    bool isInteractive() const noexcept override {
        return interactive_;
    }

private:
    mafia::PlayerId target_;
    std::string label_;
    std::vector<std::string>& executionOrder_;
    bool interactive_;
};

class PolledStrategy final : public mafia::DecisionStrategy {
public:
    PolledStrategy(
        mafia::PlayerId target,
        int waits,
        std::string label,
        std::vector<std::string>& completionOrder
    )
        : target_(target),
          waits_(waits),
          label_(std::move(label)),
          completionOrder_(completionOrder) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext&) override {
        return target_;
    }

    void startDecision(const mafia::TurnContext&) override {
        polls_ = 0;
    }

    bool decisionReady() override {
        return polls_++ >= waits_;
    }

    mafia::StrategyDecision takeDecision() override {
        completionOrder_.push_back(label_);
        return {mafia::ActionType::Vote, target_, {}, {}};
    }

private:
    mafia::PlayerId target_;
    int waits_;
    int polls_ = 0;
    std::string label_;
    std::vector<std::string>& completionOrder_;
};

class ContextCapturingStrategy final : public mafia::DecisionStrategy {
public:
    explicit ContextCapturingStrategy(mafia::AgentContext& captured)
        : captured_(captured) {}

    mafia::PlayerId chooseTarget(
        const mafia::TurnContext& context
    ) override {
        captured_ = context.agent;
        return context.availableTargets.front();
    }

private:
    mafia::AgentContext& captured_;
};

mafia::SharedPtr<mafia::DecisionStrategy> fixedTarget(
    mafia::PlayerId target
) {
    return mafia::SharedPtr<mafia::DecisionStrategy>(
        new FixedStrategy(target)
    );
}

void testPlayerWithMostVotesIsEliminated() {
    mafia::Host host;
    mafia::Civilian first(1, "First", fixedTarget(3));
    mafia::Civilian second(2, "Second", fixedTarget(3));
    mafia::Civilian third(3, "Third", fixedTarget(2));

    host.registerPlayer(first);
    host.registerPlayer(second);
    host.registerPlayer(third);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Voting,
        {{1, true}, {2, true}, {3, true}, {4, false}},
        std::nullopt,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {100, mafia::GamePhase::Voting},
        snapshot
    );

    assert(result.eliminated.size() == 1);
    assert(result.eliminated.front() == 3);

}

void testTieEliminatesNobody() {
    mafia::Host host;
    mafia::Civilian first(1, "First", fixedTarget(3));
    mafia::Civilian second(2, "Second", fixedTarget(4));
    mafia::Civilian third(3, "Third", fixedTarget(4));
    mafia::Civilian fourth(4, "Fourth", fixedTarget(3));

    host.registerPlayer(first);
    host.registerPlayer(second);
    host.registerPlayer(third);
    host.registerPlayer(fourth);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Voting,
        {{1, true}, {2, true}, {3, true}, {4, true}},
        std::nullopt,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {101, mafia::GamePhase::Voting},
        snapshot
    );

    assert(result.eliminated.empty());
}

void testAutomaticCoroutineRunsBeforeInteractiveCoroutine() {
    mafia::Host host;
    std::vector<std::string> executionOrder;
    mafia::Civilian interactive(
        1,
        "Interactive",
        mafia::SharedPtr<mafia::DecisionStrategy>(new OrderedStrategy(
            2,
            "interactive",
            executionOrder,
            true
        ))
    );
    mafia::Civilian automatic(
        2,
        "Automatic",
        mafia::SharedPtr<mafia::DecisionStrategy>(new OrderedStrategy(
            1,
            "automatic",
            executionOrder,
            false
        ))
    );

    host.registerPlayer(interactive);
    host.registerPlayer(automatic);
    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Voting,
        {{1, true}, {2, true}},
        std::nullopt,
        std::nullopt,
    };

    static_cast<void>(host.conductStep(
        {102, mafia::GamePhase::Voting},
        snapshot
    ));

    assert((executionOrder == std::vector<std::string>{
        "automatic",
        "interactive",
    }));
}

void testWaitingCoroutineDoesNotBlockOtherPlayers() {
    mafia::Host host;
    std::vector<std::string> completionOrder;
    mafia::Civilian slow(
        1,
        "Slow",
        mafia::SharedPtr<mafia::DecisionStrategy>(new PolledStrategy(
            2, 2, "slow", completionOrder
        ))
    );
    mafia::Civilian ready(
        2,
        "Ready",
        mafia::SharedPtr<mafia::DecisionStrategy>(new PolledStrategy(
            1, 0, "ready", completionOrder
        ))
    );
    host.registerPlayer(slow);
    host.registerPlayer(ready);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Voting,
        {{1, true}, {2, true}},
        std::nullopt,
        std::nullopt,
    };
    static_cast<void>(host.conductStep(
        {103, mafia::GamePhase::Voting}, snapshot
    ));

    assert((completionOrder == std::vector<std::string>{"ready", "slow"}));
}

void testAgentContextContainsEliminatedPlayers() {
    mafia::Host host;
    mafia::AgentContext captured;
    mafia::Civilian first(
        1,
        "First",
        mafia::SharedPtr<mafia::DecisionStrategy>(
            new ContextCapturingStrategy(captured)
        )
    );
    mafia::Civilian second(2, "Second", fixedTarget(1));
    host.registerPlayer(first);
    host.registerPlayer(second);

    const mafia::GameSnapshot snapshot{
        2,
        mafia::GamePhase::Voting,
        {{1, true}, {2, true}, {3, false}, {4, false}},
        std::nullopt,
        std::nullopt,
    };
    static_cast<void>(host.conductStep(
        {104, mafia::GamePhase::Voting}, snapshot
    ));

    assert((captured.livingPlayers ==
            std::vector<mafia::PlayerId>{1, 2}));
    assert((captured.eliminatedPlayers ==
            std::vector<mafia::PlayerId>{3, 4}));
}

}  // namespace

int main() {
    testPlayerWithMostVotesIsEliminated();
    testTieEliminatesNobody();
    testAutomaticCoroutineRunsBeforeInteractiveCoroutine();
    testWaitingCoroutineDoesNotBlockOtherPlayers();
    testAgentContextContainsEliminatedPlayers();
}
