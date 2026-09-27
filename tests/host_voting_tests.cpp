#include "mafia/host.hpp"
#include "mafia/roles.hpp"

#include <cassert>
#include <thread>

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

mafia::SharedPtr<mafia::DecisionStrategy> fixedTarget(
    mafia::PlayerId target
) {
    return mafia::SharedPtr<mafia::DecisionStrategy>(
        new FixedStrategy(target)
    );
}

void testPlayerWithMostVotesIsEliminated() {
    mafia::Host host;
    mafia::Civilian first(1, "First", fixedTarget(3), host);
    mafia::Civilian second(2, "Second", fixedTarget(3), host);
    mafia::Civilian third(3, "Third", fixedTarget(2), host);

    host.registerPlayer(first);
    host.registerPlayer(second);
    host.registerPlayer(third);

    std::thread firstThread(&mafia::Player::run, &first);
    std::thread secondThread(&mafia::Player::run, &second);
    std::thread thirdThread(&mafia::Player::run, &third);

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

    first.stop();
    second.stop();
    third.stop();
    firstThread.join();
    secondThread.join();
    thirdThread.join();
}

void testTieEliminatesNobody() {
    mafia::Host host;
    mafia::Civilian first(1, "First", fixedTarget(3), host);
    mafia::Civilian second(2, "Second", fixedTarget(4), host);
    mafia::Civilian third(3, "Third", fixedTarget(4), host);
    mafia::Civilian fourth(4, "Fourth", fixedTarget(3), host);

    host.registerPlayer(first);
    host.registerPlayer(second);
    host.registerPlayer(third);
    host.registerPlayer(fourth);

    std::thread firstThread(&mafia::Player::run, &first);
    std::thread secondThread(&mafia::Player::run, &second);
    std::thread thirdThread(&mafia::Player::run, &third);
    std::thread fourthThread(&mafia::Player::run, &fourth);

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

    first.stop();
    second.stop();
    third.stop();
    fourth.stop();
    firstThread.join();
    secondThread.join();
    thirdThread.join();
    fourthThread.join();
}

}  // namespace

int main() {
    testPlayerWithMostVotesIsEliminated();
    testTieEliminatesNobody();
}
