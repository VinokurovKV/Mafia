#include "mafia/command_line.hpp"

#include <charconv>
#include <stdexcept>

namespace mafia {
namespace {

std::size_t parseNumber(
    std::string_view value,
    std::string_view optionName
) {
    std::size_t number = 0;
    const auto [end, error] = std::from_chars(
        value.data(),
        value.data() + value.size(),
        number
    );

    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument(
            std::string(optionName) + " requires an integer value"
        );
    }
    return number;
}

}  // namespace

CommandLineOptions parseCommandLine(
    std::span<const std::string_view> arguments
) {
    CommandLineOptions options;
    bool playersSpecified = false;
    bool announcementModeSpecified = false;
    bool logLevelSpecified = false;

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];

        if (argument == "--help" || argument == "-h") {
            options.showHelp = true;
            continue;
        }
        if (argument == "--players") {
            if (++index == arguments.size()) {
                throw std::invalid_argument("--players requires a value");
            }
            options.config.playerCount = parseNumber(
                arguments[index],
                "--players"
            );
            playersSpecified = true;
            continue;
        }
        if (argument == "--mafia-divisor") {
            if (++index == arguments.size()) {
                throw std::invalid_argument(
                    "--mafia-divisor requires a value"
                );
            }
            options.config.mafiaDivisor = parseNumber(
                arguments[index],
                "--mafia-divisor"
            );
            continue;
        }
        if (argument == "--interactive") {
            options.config.interactive = true;
            continue;
        }
        if (argument == "--log-directory") {
            if (++index == arguments.size() || arguments[index].empty()) {
                throw std::invalid_argument(
                    "--log-directory requires a value"
                );
            }
            options.config.logDirectory = arguments[index];
            continue;
        }
        if (argument == "--role-config") {
            if (++index == arguments.size() || arguments[index].empty()) {
                throw std::invalid_argument(
                    "--role-config requires a value"
                );
            }
            options.config.roleConfigFile = arguments[index];
            continue;
        }
        if (
            argument == "--open-announcements" ||
            argument == "--closed-announcements"
        ) {
            if (announcementModeSpecified) {
                throw std::invalid_argument(
                    "Announcement mode may be specified only once"
                );
            }
            options.config.announcementMode =
                argument == "--open-announcements"
                ? AnnouncementMode::Open
                : AnnouncementMode::Closed;
            announcementModeSpecified = true;
            continue;
        }
        if (argument == "--full-log" || argument == "--brief-log") {
            if (logLevelSpecified) {
                throw std::invalid_argument(
                    "Log level may be specified only once"
                );
            }
            options.config.logLevel = argument == "--full-log"
                ? LogLevel::Full
                : LogLevel::Brief;
            logLevelSpecified = true;
            continue;
        }

        throw std::invalid_argument(
            "Unknown command line option: " + std::string(argument)
        );
    }

    if (!options.showHelp && !playersSpecified) {
        throw std::invalid_argument("Required option --players is missing");
    }
    return options;
}

std::string commandLineUsage(std::string_view executableName) {
    return
        "Usage: " + std::string(executableName) +
        " --players N [options]\n\n"
        "Options:\n"
        "  --players N              Number of players, N > 4\n"
        "  --mafia-divisor K        Mafia divisor, K >= 3 (default: 3)\n"
        "  --interactive            Use one human-controlled player\n"
        "  --open-announcements     Reveal full roles and night details\n"
        "  --closed-announcements   Hide roles and night details (default)\n"
        "  --full-log               Print every accepted action\n"
        "  --brief-log              Print only main events (default)\n"
        "  --log-directory PATH     File log directory (default: logs)\n"
        "  --role-config PATH       YAML file with additional roles\n"
        "  --help, -h               Show this help\n";
}

}  // namespace mafia
