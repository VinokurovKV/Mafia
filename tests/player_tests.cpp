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

class TestPlayer final : public mafia::Player {
public:
    using Player::Player;

    mafia::Action makeAction(const mafia::TurnContext& context) override {
        return {
            context.stepId,
            id(),
            mafia::ActionType::Vote,
            strategy().chooseTarget(context),
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

    const mafia::Action action = player.performTurn();
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
    const mafia::Action nextAction = player.performTurn();
    assert(nextAction.stepId == 11);
    assert(nextAction.target == 2);

    bool missingTurnRejected = false;
    try {
        static_cast<void>(player.performTurn());
    } catch (const std::logic_error&) {
        missingTurnRejected = true;
    }
    assert(missingTurnRejected);
}
