#pragma once

#include "mafia/player.hpp"

namespace mafia {

class Mafia : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action makeAction(const TurnContext& context) override;
    RoleType role() const noexcept override;
};

class Civilian : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = false;

    Action makeAction(const TurnContext& context) override;
    RoleType role() const noexcept override;
};

class Doctor : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action makeAction(const TurnContext& context) override;
    RoleType role() const noexcept override;
};

class Commissioner : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action makeAction(const TurnContext& context) override;
    RoleType role() const noexcept override;
};

class Maniac : public Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;

    Action makeAction(const TurnContext& context) override;
    RoleType role() const noexcept override;
};

}  // namespace mafia
