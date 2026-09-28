#include "mafia/player.hpp"

#include <stdexcept>
#include <utility>

namespace mafia {

Player::Player(
    PlayerId id,
    std::string name,
    SharedPtr<DecisionStrategy> strategy
)
    : id_(id),
      name_(std::move(name)),
      strategy_(std::move(strategy)),
      actionTask_(actionLoop()) {}

PlayerId Player::id() const noexcept {
    return id_;
}

const std::string& Player::name() const noexcept {
    return name_;
}

bool Player::isInteractive() const noexcept {
    return strategy_->isInteractive();
}

void Player::requestTurn(TurnContext context) {
    if (pendingTurn_.has_value()) {
        throw std::logic_error("Player already has a pending turn");
    }
    pendingTurn_ = std::move(context);
}

Action Player::performTurn() {
    if (!pendingTurn_.has_value()) {
        throw std::logic_error("Player has no pending turn");
    }

    actionTask_.resume();
    return actionTask_.takeAction();
}

PlayerTask Player::actionLoop() {
    while (true) {
        if (!pendingTurn_.has_value()) {
            throw std::logic_error("Player coroutine resumed without a turn");
        }

        TurnContext context = std::move(*pendingTurn_);
        pendingTurn_.reset();
        co_yield makeAction(context);
    }
}

DecisionStrategy& Player::strategy() noexcept {
    return *strategy_;
}

const DecisionStrategy& Player::strategy() const noexcept {
    return *strategy_;
}

}  // namespace mafia
