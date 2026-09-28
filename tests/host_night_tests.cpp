#include "mafia/host.hpp"
#include "mafia/roles.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <optional>
#include <vector>

namespace {

class NightStrategy final : public mafia::DecisionStrategy {
public:
    NightStrategy(
        mafia::PlayerId target,
        std::optional<mafia::ActionType> actionType = std::nullopt,
        std::optional<mafia::PlayerId> excludedTarget = std::nullopt
    )
        : target_(target),
          actionType_(actionType),
          excludedTarget_(excludedTarget) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext& context) override {
        const auto configuredTarget = std::find(
            context.availableTargets.begin(),
            context.availableTargets.end(),
            target_
        );

        if (
            configuredTarget == context.availableTargets.end() &&
            context.availableTargets.size() == 1 &&
            context.availableActions ==
                std::vector<mafia::ActionType>{mafia::ActionType::MafiaKill}
        ) {
            return context.availableTargets.front();
        }
        assert(configuredTarget != context.availableTargets.end());

        if (excludedTarget_.has_value()) {
            assert(std::find(
                context.availableTargets.begin(),
                context.availableTargets.end(),
                *excludedTarget_
            ) == context.availableTargets.end());
        }
        return target_;
    }

    mafia::ActionType chooseActionType(
        const mafia::TurnContext& context
    ) override {
        if (!actionType_.has_value()) {
            return DecisionStrategy::chooseActionType(context);
        }

        assert(std::find(
            context.availableActions.begin(),
            context.availableActions.end(),
            *actionType_
        ) != context.availableActions.end());
        return *actionType_;
    }

private:
    mafia::PlayerId target_;
    std::optional<mafia::ActionType> actionType_;
    std::optional<mafia::PlayerId> excludedTarget_;
};

class DiscussionStrategy final : public mafia::DecisionStrategy {
public:
    explicit DiscussionStrategy(mafia::PlayerId initialTarget)
        : initialTarget_(initialTarget) {}

    mafia::PlayerId chooseTarget(const mafia::TurnContext& context) override {
        if (firstTurn_) {
            firstTurn_ = false;
            assert(std::find(
                context.availableTargets.begin(),
                context.availableTargets.end(),
                initialTarget_
            ) != context.availableTargets.end());
            return initialTarget_;
        }

        discussionOptions_.push_back(context.availableTargets);
        assert(!context.availableTargets.empty());
        return *std::min_element(
            context.availableTargets.begin(),
            context.availableTargets.end()
        );
    }

    const std::vector<std::vector<mafia::PlayerId>>& discussionOptions()
        const noexcept {
        return discussionOptions_;
    }

private:
    mafia::PlayerId initialTarget_;
    bool firstTurn_ = true;
    std::vector<std::vector<mafia::PlayerId>> discussionOptions_;
};

mafia::SharedPtr<mafia::DecisionStrategy> nightStrategy(
    mafia::PlayerId target,
    std::optional<mafia::ActionType> actionType = std::nullopt,
    std::optional<mafia::PlayerId> excludedTarget = std::nullopt
) {
    return mafia::SharedPtr<mafia::DecisionStrategy>(
        new NightStrategy(target, actionType, excludedTarget)
    );
}

void registerPlayers(
    mafia::Host& host,
    const std::vector<mafia::Player*>& players
) {
    for (mafia::Player* player : players) {
        host.registerPlayer(*player);
    }
}

void testHealingAndInvestigation() {
    mafia::Host host;
    mafia::Mafia mafiaPlayer(1, "Mafia", nightStrategy(4));
    mafia::Doctor doctor(
        2,
        "Doctor",
        nightStrategy(4, std::nullopt, 2)
    );
    mafia::Commissioner commissioner(
        3,
        "Commissioner",
        nightStrategy(1, mafia::ActionType::Check)
    );
    mafia::Maniac maniac(4, "Maniac", nightStrategy(5));
    mafia::Civilian civilian(5, "Civilian", nightStrategy(1));

    const std::vector<mafia::Player*> players{
        &mafiaPlayer,
        &doctor,
        &commissioner,
        &maniac,
        &civilian,
    };
    registerPlayers(host, players);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Night,
        {{1, true}, {2, true}, {3, true}, {4, true}, {5, true}},
        2,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {200, mafia::GamePhase::Night},
        snapshot
    );
    assert(result.doctorTarget == 4);
    assert(result.eliminated == std::vector<mafia::PlayerId>{5});
    assert(result.investigations.size() == 1);
    assert(result.investigations.front().investigator == 3);
    assert(result.investigations.front().target == 1);
    assert(result.investigations.front().targetIsMafia);
    assert(
        result.completedAt != std::chrono::system_clock::time_point{}
    );
}

void testCommissionerCanShoot() {
    mafia::Host host;
    mafia::Mafia mafiaPlayer(1, "Mafia", nightStrategy(5));
    mafia::Doctor doctor(2, "Doctor", nightStrategy(4));
    mafia::Commissioner commissioner(
        3,
        "Commissioner",
        nightStrategy(1, mafia::ActionType::Shoot)
    );
    mafia::Maniac maniac(4, "Maniac", nightStrategy(5));
    mafia::Civilian civilian(5, "Civilian", nightStrategy(1));

    const std::vector<mafia::Player*> players{
        &mafiaPlayer,
        &doctor,
        &commissioner,
        &maniac,
        &civilian,
    };
    registerPlayers(host, players);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Night,
        {{1, true}, {2, true}, {3, true}, {4, true}, {5, true}},
        std::nullopt,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {201, mafia::GamePhase::Night},
        snapshot
    );
    assert((result.eliminated == std::vector<mafia::PlayerId>{1, 5}));
    assert(result.investigations.empty());
}

void testMafiaAlwaysReachesACommonTarget() {
    mafia::Host host;
    mafia::Mafia firstMafia(1, "First Mafia", nightStrategy(4));
    mafia::Mafia secondMafia(2, "Second Mafia", nightStrategy(5));
    mafia::Civilian firstCivilian(3, "First", nightStrategy(1));
    mafia::Civilian secondCivilian(4, "Second", nightStrategy(1));
    mafia::Civilian thirdCivilian(5, "Third", nightStrategy(1));

    const std::vector<mafia::Player*> players{
        &firstMafia,
        &secondMafia,
        &firstCivilian,
        &secondCivilian,
        &thirdCivilian,
    };
    registerPlayers(host, players);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Night,
        {{1, true}, {2, true}, {3, true}, {4, true}, {5, true}},
        std::nullopt,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {202, mafia::GamePhase::Night},
        snapshot
    );
    assert(result.eliminated.size() == 1);
    assert(
        result.eliminated.front() == 4 ||
        result.eliminated.front() == 5
    );
    assert(result.mafiaTarget == result.eliminated.front());
    assert(result.mafiaConsensusRequired);

    std::size_t confirmedMafiaActions = 0;
    for (const mafia::Action& action : result.actions) {
        if (action.type == mafia::ActionType::MafiaKill) {
            ++confirmedMafiaActions;
            assert(action.target == *result.mafiaTarget);
        }
    }
    assert(confirmedMafiaActions == 2);
    assert(result.actionHistory.size() == result.actions.size() + 1);
}

void testMafiaReceivesOtherPlayersProposals() {
    mafia::Host host;
    auto* firstStrategy = new DiscussionStrategy(4);
    auto* secondStrategy = new DiscussionStrategy(5);
    auto* thirdStrategy = new DiscussionStrategy(6);

    mafia::Mafia firstMafia(
        1,
        "First Mafia",
        mafia::SharedPtr<mafia::DecisionStrategy>(firstStrategy)
    );
    mafia::Mafia secondMafia(
        2,
        "Second Mafia",
        mafia::SharedPtr<mafia::DecisionStrategy>(secondStrategy)
    );
    mafia::Mafia thirdMafia(
        3,
        "Third Mafia",
        mafia::SharedPtr<mafia::DecisionStrategy>(thirdStrategy)
    );
    mafia::Civilian firstCivilian(4, "First", nightStrategy(1));
    mafia::Civilian secondCivilian(5, "Second", nightStrategy(1));
    mafia::Civilian thirdCivilian(6, "Third", nightStrategy(1));
    mafia::Civilian fourthCivilian(7, "Fourth", nightStrategy(1));

    const std::vector<mafia::Player*> players{
        &firstMafia,
        &secondMafia,
        &thirdMafia,
        &firstCivilian,
        &secondCivilian,
        &thirdCivilian,
        &fourthCivilian,
    };
    registerPlayers(host, players);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Night,
        {
            {1, true}, {2, true}, {3, true}, {4, true},
            {5, true}, {6, true}, {7, true},
        },
        std::nullopt,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {203, mafia::GamePhase::Night},
        snapshot
    );
    assert(result.mafiaTarget == 5);
    assert(result.eliminated == std::vector<mafia::PlayerId>{5});
    assert(result.mafiaConsensusRequired);
    const std::vector<std::vector<mafia::PlayerId>> twoOptions{{5, 6}};
    const std::vector<std::vector<mafia::PlayerId>> oneOption{{5}};
    assert(firstStrategy->discussionOptions() == twoOptions);
    assert(secondStrategy->discussionOptions() == twoOptions);
    assert(thirdStrategy->discussionOptions() == oneOption);
    assert(result.actionHistory.size() == result.actions.size() + 3);
}

void testAdditionalRoleNightRules() {
    mafia::Host host;
    mafia::Mafia mafiaPlayer(1, "Mafia", nightStrategy(4));
    mafia::Maniac maniac(2, "Maniac", nightStrategy(3));
    mafia::Bull bull(3, "Bull", nightStrategy(4));
    mafia::Doctor doctor(4, "Doctor", nightStrategy(4));
    mafia::Eavesdropper eavesdropper(
        5,
        "Eavesdropper",
        nightStrategy(3)
    );
    mafia::Witness witness(6, "Witness", nightStrategy(3));
    mafia::Civilian civilian(7, "Civilian", nightStrategy(1));
    mafia::Commissioner commissioner(
        8,
        "Commissioner",
        nightStrategy(3, mafia::ActionType::Check)
    );

    const std::vector<mafia::Player*> players{
        &mafiaPlayer,
        &maniac,
        &bull,
        &doctor,
        &eavesdropper,
        &witness,
        &civilian,
        &commissioner,
    };
    registerPlayers(host, players);

    const mafia::GameSnapshot snapshot{
        1,
        mafia::GamePhase::Night,
        {
            {1, true}, {2, true}, {3, true}, {4, true},
            {5, true}, {6, true}, {7, true}, {8, true},
        },
        std::nullopt,
        std::nullopt,
    };
    const mafia::StepResult result = host.conductStep(
        {204, mafia::GamePhase::Night},
        snapshot
    );

    assert(result.mafiaTarget == 4);
    assert(result.eliminated.empty());
    assert(result.investigations.size() == 1);
    assert(result.investigations.front().target == 3);
    assert(result.investigations.front().targetIsMafia);

    assert(result.eavesdropResults.size() == 1);
    const auto& heard = result.eavesdropResults.front();
    assert(heard.listener == 5);
    assert(heard.target == 3);
    assert(std::find(
        heard.directedActions.begin(),
        heard.directedActions.end(),
        mafia::ActionType::ManiacKill
    ) != heard.directedActions.end());
    assert(std::find(
        heard.directedActions.begin(),
        heard.directedActions.end(),
        mafia::ActionType::Check
    ) != heard.directedActions.end());

    assert(result.witnessResults.size() == 1);
    const auto& observed = result.witnessResults.front();
    assert(observed.witness == 6);
    assert(observed.target == 3);
    assert(observed.attackers == std::vector<mafia::PlayerId>{2});
}

}  // namespace

int main() {
    testHealingAndInvestigation();
    testCommissionerCanShoot();
    testMafiaAlwaysReachesACommonTarget();
    testMafiaReceivesOtherPlayersProposals();
    testAdditionalRoleNightRules();
}
