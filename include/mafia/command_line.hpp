#pragma once

#include <span>
#include <string>
#include <string_view>

#include "mafia/types.hpp"

namespace mafia {

struct CommandLineOptions {
    GameConfig config;
    bool showHelp = false;
};

CommandLineOptions parseCommandLine(
    std::span<const std::string_view> arguments
);

std::string commandLineUsage(std::string_view executableName);

}  // namespace mafia
