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
    if (pendingTurn_.has_value() || turnInProgress_) {
        throw std::logic_error("Player already has a pending turn");
    }
    pendingTurn_ = std::move(context);
}

Action Player::makeAction(const TurnContext& context) {
    return formAction(context, strategy_->decide(context));
}

std::optional<Action> Player::pollTurn() {
    if (!pendingTurn_.has_value() && !turnInProgress_) {
        throw std::logic_error("Player has no pending turn");
    }

    actionTask_.resume();
    if (!actionTask_.hasAction()) {
        return std::nullopt;
    }
    return actionTask_.takeAction();
}

PlayerTask Player::actionLoop() {
    while (true) {
        if (!pendingTurn_.has_value()) {
            throw std::logic_error("Player coroutine resumed without a turn");
        }

        TurnContext context = std::move(*pendingTurn_);
        pendingTurn_.reset();
        turnInProgress_ = true;
        strategy_->startDecision(context);
        while (!strategy_->decisionReady()) {
            co_await std::suspend_always{};
        }
        Action action = formAction(
            context,
            strategy_->takeDecision()
        );
        turnInProgress_ = false;
        co_yield action;
    }
}

DecisionStrategy& Player::strategy() noexcept {
    return *strategy_;
}

const DecisionStrategy& Player::strategy() const noexcept {
    return *strategy_;
}

}  // namespace mafia
