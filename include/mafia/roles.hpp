#pragma once

#include "mafia/player.hpp"

namespace mafia {

class Mafia : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Civilian : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = false;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Doctor : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Commissioner : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Maniac : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Eavesdropper : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Witness : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

class Bull : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action formAction(
        const TurnContext& context,
        StrategyDecision decision
    ) override;
    RoleType role() const noexcept override;
};

}  // namespace mafia
