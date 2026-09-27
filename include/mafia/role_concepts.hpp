#pragma once

#include <concepts>
#include <string>
#include <type_traits>

#include "mafia/player.hpp"

namespace mafia {

template <typename Role>
concept PlayerRole =
    std::derived_from<Role, Player> &&
    !std::is_abstract_v<Role> &&
    requires(Role& player, const Role& constPlayer, const TurnContext& context) {
        { player.makeAction(context) } -> std::same_as<Action>;
        { constPlayer.role() } noexcept -> std::same_as<RoleType>;
        { constPlayer.id() } noexcept -> std::same_as<PlayerId>;
        { constPlayer.name() } noexcept -> std::same_as<const std::string&>;
        { Role::participatesInVoting } -> std::convertible_to<bool>;
        { Role::actsAtNight } -> std::convertible_to<bool>;
    };

template <typename Role>
concept VotingRole = PlayerRole<Role> && Role::participatesInVoting;

template <typename Role>
concept NightActingRole = PlayerRole<Role> && Role::actsAtNight;

}  // namespace mafia
