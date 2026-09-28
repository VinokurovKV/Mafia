#pragma once

#include <filesystem>
#include <vector>

#include "mafia/types.hpp"

namespace mafia {

std::vector<RoleType> loadAdditionalRoles(
    const std::filesystem::path& path
);

}  // namespace mafia
