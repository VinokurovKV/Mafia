#include "mafia/console_strategy.hpp"

#include <cassert>
#include <sstream>
#include <string>

namespace {

void testTargetInputIsValidated() {
    std::istringstream input("text\n3\n4\n");
    std::ostringstream output;
    mafia::ConsoleStrategy strategy(input, output);

    const mafia::TurnContext context{
        1,
        mafia::GamePhase::Voting,
        {2, 4, 6},
        {mafia::ActionType::Vote},
    };

    assert(strategy.chooseTarget(context) == 4);
    assert(output.str().find("Invalid target") != std::string::npos);
}

void testCommissionerActionCanBeSelected() {
    std::istringstream input("2\n");
    std::ostringstream output;
    mafia::ConsoleStrategy strategy(input, output);

    const mafia::TurnContext context{
        2,
        mafia::GamePhase::Night,
        {2, 3},
        {mafia::ActionType::Check, mafia::ActionType::Shoot},
    };

    assert(
        strategy.chooseActionType(context) == mafia::ActionType::Shoot
    );
}

void testSingleTargetNeedsNoInput() {
    std::istringstream input;
    std::ostringstream output;
    mafia::ConsoleStrategy strategy(input, output);

    const mafia::TurnContext context{
        3,
        mafia::GamePhase::Night,
        {7},
        {mafia::ActionType::MafiaKill},
    };

    assert(strategy.chooseTarget(context) == 7);
    assert(output.str().find("Only available target") != std::string::npos);
}

void testClosedInputUsesSafeFallback() {
    std::istringstream input;
    std::ostringstream output;
    mafia::ConsoleStrategy strategy(input, output);

    const mafia::TurnContext context{
        4,
        mafia::GamePhase::Voting,
        {8, 9},
        {mafia::ActionType::Vote},
    };

    assert(strategy.chooseTarget(context) == 8);
    assert(output.str().find("Input closed") != std::string::npos);
}

void testAsynchronousDecisionIsPolledInStages() {
    std::istringstream input("2\n3\n");
    std::ostringstream output;
    mafia::ConsoleStrategy strategy(input, output);
    const mafia::TurnContext context{
        5,
        mafia::GamePhase::Night,
        {2, 3},
        {mafia::ActionType::Check, mafia::ActionType::Shoot},
    };

    strategy.startDecision(context);
    assert(!strategy.decisionReady());
    assert(strategy.decisionReady());
    const mafia::StrategyDecision decision = strategy.takeDecision();
    assert(decision.action == mafia::ActionType::Shoot);
    assert(decision.target == 3);
}

}  // namespace

int main() {
    testTargetInputIsValidated();
    testCommissionerActionCanBeSelected();
    testSingleTargetNeedsNoInput();
    testClosedInputUsesSafeFallback();
    testAsynchronousDecisionIsPolledInStages();
}
