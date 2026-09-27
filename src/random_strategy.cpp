#include "mafia/random_strategy.hpp"

#include <stdexcept>

namespace mafia {

RandomStrategy::RandomStrategy()
    : RandomStrategy(std::random_device{}()) {}

RandomStrategy::RandomStrategy(std::mt19937::result_type seed)
    : generator_(seed) {}

PlayerId RandomStrategy::chooseTarget(const TurnContext& context) {
    if (context.availableTargets.empty()) {
        throw std::logic_error("No targets are available");
    }

    std::uniform_int_distribution<std::size_t> distribution(
        0,
        context.availableTargets.size() - 1
    );
    return context.availableTargets[distribution(generator_)];
}

ActionType RandomStrategy::chooseActionType(const TurnContext& context) {
    if (context.availableActions.empty()) {
        throw std::logic_error("No action types are available");
    }

    std::uniform_int_distribution<std::size_t> distribution(
        0,
        context.availableActions.size() - 1
    );
    return context.availableActions[distribution(generator_)];
}

}  // namespace mafia
