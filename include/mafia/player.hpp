#pragma once

#include <optional>
#include <string>

#include "mafia/decision_strategy.hpp"
#include "mafia/player_task.hpp"
#include "mafia/shared_ptr.hpp"
#include "mafia/types.hpp"

namespace mafia {

class Player {
public:
    Player(
        PlayerId id,
        std::string name,
        SharedPtr<DecisionStrategy> strategy
    );

    virtual ~Player() = default;

    virtual Action makeAction(const TurnContext& context) = 0;
    virtual RoleType role() const noexcept = 0;

    PlayerId id() const noexcept;
    const std::string& name() const noexcept;
    bool isInteractive() const noexcept;

    void requestTurn(TurnContext context);
    Action performTurn();

protected:
    DecisionStrategy& strategy() noexcept;
    const DecisionStrategy& strategy() const noexcept;

private:
    PlayerTask actionLoop();

    PlayerId id_;
    std::string name_;
    SharedPtr<DecisionStrategy> strategy_;
    std::optional<TurnContext> pendingTurn_;
    PlayerTask actionTask_;
};

}  // namespace mafia
