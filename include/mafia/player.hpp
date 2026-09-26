#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

#include "mafia/decision_strategy.hpp"
#include "mafia/shared_ptr.hpp"
#include "mafia/types.hpp"

namespace mafia {

class Host;

class Player {
public:
    Player(
        PlayerId id,
        std::string name,
        SharedPtr<DecisionStrategy> strategy,
        Host& host
    );

    virtual ~Player() = default;

    virtual Action makeAction(const TurnContext& context) = 0;

    PlayerId id() const noexcept;
    const std::string& name() const noexcept;

    void requestTurn(TurnContext context);
    void run();
    void stop();

protected:
    DecisionStrategy& strategy() noexcept;
    const DecisionStrategy& strategy() const noexcept;

private:
    PlayerId id_;
    std::string name_;
    SharedPtr<DecisionStrategy> strategy_;
    Host& host_;

    std::mutex turnMutex_;
    std::condition_variable turnAvailable_;
    std::optional<TurnContext> pendingTurn_;
    bool stopped_ = false;
};

}  // namespace mafia
