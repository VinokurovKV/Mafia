#include "mafia/host.hpp"
#include "mafia/player.hpp"

#include <cassert>
#include <chrono>
#include <future>
#include <thread>

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

    std::future<mafia::Action> actionFuture() {
        return actionPromise_.get_future();
    }

    mafia::Action makeAction(const mafia::TurnContext& context) override {
        mafia::Action action{
            context.stepId,
            id(),
            mafia::ActionType::Vote,
            strategy().chooseTarget(context),
        };
        actionPromise_.set_value(action);
        return action;
    }

    mafia::RoleType role() const noexcept override {
        return mafia::RoleType::Civilian;
    }

private:
    std::promise<mafia::Action> actionPromise_;
};

}  // namespace

int main() {
    mafia::Host host;
    mafia::SharedPtr<mafia::DecisionStrategy> strategy(
        new FixedStrategy(2)
    );
    TestPlayer player(1, "Player 1", strategy, host);

    assert(player.id() == 1);
    assert(player.name() == "Player 1");

    auto actionFuture = player.actionFuture();
    std::thread playerThread(&mafia::Player::run, &player);

    player.requestTurn({
        10,
        mafia::GamePhase::Voting,
        {2, 3},
        {mafia::ActionType::Vote},
    });

    assert(
        actionFuture.wait_for(std::chrono::seconds(1)) ==
        std::future_status::ready
    );

    const mafia::Action action = actionFuture.get();
    assert(action.stepId == 10);
    assert(action.actor == 1);
    assert(action.type == mafia::ActionType::Vote);
    assert(action.target == 2);

    player.stop();
    playerThread.join();
}
