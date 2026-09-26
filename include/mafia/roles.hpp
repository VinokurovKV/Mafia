#pragma once

#include "mafia/player.hpp"

namespace mafia {

class Mafia : public Player {
public:
    using Player::Player;

    Action makeAction(const TurnContext& context) override;
};

class Civilian : public Player {
public:
    using Player::Player;

    Action makeAction(const TurnContext& context) override;
};

class Doctor : public Player {
public:
    using Player::Player;

    Action makeAction(const TurnContext& context) override;
};

class Commissioner : public Player {
public:
    using Player::Player;

    Action makeAction(const TurnContext& context) override;
};

class Maniac : public Player {
public:
    using Player::Player;

    Action makeAction(const TurnContext& context) override;
};

}  // namespace mafia
