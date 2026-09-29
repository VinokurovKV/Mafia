#include "mafia/player.hpp"

#include <cassert>
#include <stdexcept>

namespace {

class FixedStrategy final : public mafia::DecisionStrategy {
public:
    explicit FixedStrategy(mafia::PlayerId target)
        : target_(target) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext&) override {
        return target_;
    }

private:
    mafia::PlayerId target_;
};

class DelayedStrategy final : public mafia::DecisionStrategy {
public:
    mafia::PlayerId chooseTarget(const mafia::TurnContext&) override {
        return 3;
    }

    void startDecision(const mafia::TurnContext&) override {
        polls_ = 0;
    }

    bool decisionReady() override {
        return polls_++ > 0;
    }

    mafia::StrategyDecision takeDecision() override {
        return {
            mafia::ActionType::Vote,
            3,
            {},
            {},
        };
    }

private:
    int polls_ = 0;
};

class TestPlayer final : public mafia::Player {
public:
    using Player::Player;

    mafia::Action formAction(
        const mafia::TurnContext& context,
        mafia::StrategyDecision decision
    ) override {
        return {
            context.stepId,
            id(),
            mafia::ActionType::Vote,
            decision.target,
        };
    }

    mafia::RoleType role() const noexcept override {
        return mafia::RoleType::Civilian;
    }

};

}  // namespace

int main() {
    mafia::SharedPtr<mafia::DecisionStrategy> strategy(
        new FixedStrategy(2)
    );
    TestPlayer player(1, "Player 1", strategy);

    assert(player.id() == 1);
    assert(player.name() == "Player 1");

    player.requestTurn({
        10,
        mafia::GamePhase::Voting,
        {2, 3},
        {mafia::ActionType::Vote},
    });

    const std::optional<mafia::Action> polledAction = player.pollTurn();
    assert(polledAction.has_value());
    const mafia::Action& action = *polledAction;
    assert(action.stepId == 10);
    assert(action.actor == 1);
    assert(action.type == mafia::ActionType::Vote);
    assert(action.target == 2);

    player.requestTurn({
        11,
        mafia::GamePhase::Voting,
        {2, 3},
        {mafia::ActionType::Vote},
    });
    const std::optional<mafia::Action> polledNextAction = player.pollTurn();
    assert(polledNextAction.has_value());
    const mafia::Action& nextAction = *polledNextAction;
    assert(nextAction.stepId == 11);
    assert(nextAction.target == 2);

    bool missingTurnRejected = false;
    try {
        static_cast<void>(player.pollTurn());
    } catch (const std::logic_error&) {
        missingTurnRejected = true;
    }
    assert(missingTurnRejected);

    mafia::SharedPtr<mafia::DecisionStrategy> delayedStrategy(
        new DelayedStrategy
    );
    TestPlayer delayedPlayer(2, "Delayed", delayedStrategy);
    delayedPlayer.requestTurn({
        12,
        mafia::GamePhase::Voting,
        {1, 3},
        {mafia::ActionType::Vote},
    });

    assert(!delayedPlayer.pollTurn().has_value());
    const std::optional<mafia::Action> delayedAction =
        delayedPlayer.pollTurn();
    assert(delayedAction.has_value());
    assert(delayedAction->target == 3);
}
