#include "mafia/game.hpp"
#include "mafia/roles.hpp"

#include <algorithm>
#include <cassert>
#include <optional>

namespace {

class RoundStrategy final : public mafia::DecisionStrategy {
public:
    RoundStrategy(
        mafia::PlayerId votingTarget,
        mafia::PlayerId nightTarget,
        std::optional<mafia::ActionType> nightAction = std::nullopt
    )
        : votingTarget_(votingTarget),
          nightTarget_(nightTarget),
          nightAction_(nightAction) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext& context) override {
        const mafia::PlayerId target =
            context.phase == mafia::GamePhase::Voting
            ? votingTarget_
            : nightTarget_;

        assert(std::find(
            context.availableTargets.begin(),
            context.availableTargets.end(),
            target
        ) != context.availableTargets.end());
        return target;
    }

    mafia::ActionType chooseActionType(
        const mafia::TurnContext& context
    ) override {
        if (nightAction_.has_value()) {
            return *nightAction_;
        }
        return DecisionStrategy::chooseActionType(context);
    }

private:
    mafia::PlayerId votingTarget_;
    mafia::PlayerId nightTarget_;
    std::optional<mafia::ActionType> nightAction_;
};

mafia::SharedPtr<mafia::DecisionStrategy> roundStrategy(
    mafia::PlayerId votingTarget,
    mafia::PlayerId nightTarget,
    std::optional<mafia::ActionType> nightAction = std::nullopt
) {
    return mafia::SharedPtr<mafia::DecisionStrategy>(
        new RoundStrategy(votingTarget, nightTarget, nightAction)
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
    game.addPlayer<mafia::Mafia>(1, "Mafia", roundStrategy(5, 5));
    game.addPlayer<mafia::Doctor>(2, "Doctor", roundStrategy(1, 2));
    game.addPlayer<mafia::Commissioner>(
        3,
        "Commissioner",
        roundStrategy(1, 4, mafia::ActionType::Shoot)
    );
    game.addPlayer<mafia::Maniac>(4, "Maniac", roundStrategy(5, 1));
    game.addPlayer<mafia::Civilian>(5, "Civilian", roundStrategy(4, 1));

    game.run();

    const mafia::GameSnapshot snapshot = game.snapshot();
    assert(snapshot.phase == mafia::GamePhase::Finished);
    assert(snapshot.winner == mafia::Winner::Civilians);
    assert(snapshot.round == 1);
    assert(!isAlive(snapshot, 1));
    assert(isAlive(snapshot, 2));
    assert(isAlive(snapshot, 3));
    assert(!isAlive(snapshot, 4));
    assert(!isAlive(snapshot, 5));
    assert(snapshot.lastDoctorTarget == 2);
    assert(!snapshot.publicHistory.empty());
}
