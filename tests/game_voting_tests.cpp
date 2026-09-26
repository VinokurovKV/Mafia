#include "mafia/game.hpp"
#include "mafia/roles.hpp"

#include <algorithm>
#include <cassert>

namespace {

class FixedStrategy final : public mafia::DecisionStrategy {
public:
    explicit FixedStrategy(mafia::PlayerId target)
        : target_(target) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext& context) override {
        const auto target = std::find(
            context.availableTargets.begin(),
            context.availableTargets.end(),
            target_
        );
        assert(target != context.availableTargets.end());
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

bool isAlive(const mafia::GameSnapshot& snapshot, mafia::PlayerId id) {
    const auto player = std::find_if(
        snapshot.players.begin(),
        snapshot.players.end(),
        [id](const mafia::PlayerState& state) {
            return state.id == id;
        }
    );

    assert(player != snapshot.players.end());
    return player->alive;
}

}  // namespace

int main() {
    mafia::Game game;
    game.addPlayer<mafia::Civilian>(1, "First", fixedTarget(3));
    game.addPlayer<mafia::Doctor>(2, "Second", fixedTarget(3));
    game.addPlayer<mafia::Mafia>(3, "Third", fixedTarget(2));

    game.run();

    const mafia::GameSnapshot snapshot = game.snapshot();
    assert(snapshot.phase == mafia::GamePhase::Voting);
    assert(isAlive(snapshot, 1));
    assert(isAlive(snapshot, 2));
    assert(!isAlive(snapshot, 3));
}
