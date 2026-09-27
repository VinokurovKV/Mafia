#include "mafia/host.hpp"
#include "mafia/roles.hpp"

#include <cassert>
#include <stdexcept>

namespace {

class FixedStrategy final : public mafia::DecisionStrategy {
public:
    explicit FixedStrategy(mafia::PlayerId target)
        : target_(target) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext& context) override {
        ++callCount_;
        assert(!context.availableTargets.empty());
        return target_;
    }

    int callCount() const noexcept {
        return callCount_;
    }

private:
    mafia::PlayerId target_;
    int callCount_ = 0;
};

template <typename Role>
void expectAction(
    mafia::PlayerId actor,
    mafia::GamePhase phase,
    mafia::ActionType expectedType
) {
    mafia::Host host;
    auto* strategyValue = new FixedStrategy(7);
    mafia::SharedPtr<mafia::DecisionStrategy> strategy(strategyValue);
    Role role(actor, "Player", strategy, host);

    const mafia::Action action = role.makeAction({
        42,
        phase,
        {7, 8},
        {expectedType},
    });

    assert(action.stepId == 42);
    assert(action.actor == actor);
    assert(action.type == expectedType);
    assert(action.target == 7);
    assert(strategyValue->callCount() == 1);
}

void testVotingActions() {
    expectAction<mafia::Mafia>(1, mafia::GamePhase::Voting, mafia::ActionType::Vote);
    expectAction<mafia::Civilian>(2, mafia::GamePhase::Voting, mafia::ActionType::Vote);
    expectAction<mafia::Doctor>(3, mafia::GamePhase::Voting, mafia::ActionType::Vote);
    expectAction<mafia::Commissioner>(4, mafia::GamePhase::Voting, mafia::ActionType::Vote);
    expectAction<mafia::Maniac>(5, mafia::GamePhase::Voting, mafia::ActionType::Vote);
}

void testNightActions() {
    expectAction<mafia::Mafia>(1, mafia::GamePhase::Night, mafia::ActionType::MafiaKill);
    expectAction<mafia::Doctor>(3, mafia::GamePhase::Night, mafia::ActionType::Heal);
    expectAction<mafia::Commissioner>(4, mafia::GamePhase::Night, mafia::ActionType::Check);
    expectAction<mafia::Maniac>(5, mafia::GamePhase::Night, mafia::ActionType::ManiacKill);
}

void testCivilianCannotActAtNight() {
    mafia::Host host;
    mafia::SharedPtr<mafia::DecisionStrategy> strategy(new FixedStrategy(7));
    mafia::Civilian civilian(2, "Civilian", strategy, host);

    bool exceptionThrown = false;
    try {
        static_cast<void>(civilian.makeAction({
            42,
            mafia::GamePhase::Night,
            {7},
            {},
        }));
    } catch (const std::logic_error&) {
        exceptionThrown = true;
    }

    assert(exceptionThrown);
}

}  // namespace

int main() {
    testVotingActions();
    testNightActions();
    testCivilianCannotActAtNight();
}
