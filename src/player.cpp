#include "mafia/player.hpp"

#include <utility>

#include "mafia/host.hpp"

namespace mafia {

Player::Player(
    PlayerId id,
    std::string name,
    SharedPtr<DecisionStrategy> strategy,
    Host& host
)
    : id_(id),
      name_(std::move(name)),
      strategy_(std::move(strategy)),
      host_(host) {}

PlayerId Player::id() const noexcept {
    return id_;
}

const std::string& Player::name() const noexcept {
    return name_;
}

void Player::requestTurn(TurnContext context) {
    {
        std::lock_guard lock(turnMutex_);
        pendingTurn_ = std::move(context);
    }
    turnAvailable_.notify_one();
}

void Player::run() {
    while (true) {
        std::unique_lock lock(turnMutex_);
        turnAvailable_.wait(lock, [this] {
            return stopped_ || pendingTurn_.has_value();
        });

        if (stopped_) {
            return;
        }

        TurnContext context = std::move(*pendingTurn_);
        pendingTurn_.reset();
        lock.unlock();

        host_.submitAction(makeAction(context));
    }
}

void Player::stop() {
    {
        std::lock_guard lock(turnMutex_);
        stopped_ = true;
    }
    turnAvailable_.notify_one();
}

DecisionStrategy& Player::strategy() noexcept {
    return *strategy_;
}

const DecisionStrategy& Player::strategy() const noexcept {
    return *strategy_;
}

}  // namespace mafia
