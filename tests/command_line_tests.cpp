#include "mafia/command_line.hpp"

#include <cassert>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

mafia::CommandLineOptions parse(
    std::initializer_list<std::string_view> arguments
) {
    const std::vector<std::string_view> values(arguments);
    return mafia::parseCommandLine(values);
}

template <typename Function>
bool throwsInvalidArgument(Function function) {
    try {
        function();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

}  // namespace

int main() {
    const mafia::CommandLineOptions full = parse({
        "--players",
        "12",
        "--mafia-divisor",
        "4",
        "--interactive",
        "--open-announcements",
        "--full-log",
        "--log-directory",
        "test-logs",
    });
    assert(full.config.playerCount == 12);
    assert(full.config.mafiaDivisor == 4);
    assert(full.config.interactive);
    assert(
        full.config.announcementMode == mafia::AnnouncementMode::Open
    );
    assert(full.config.logLevel == mafia::LogLevel::Full);
    assert(full.config.logDirectory == "test-logs");

    const mafia::CommandLineOptions defaults = parse({"--players", "5"});
    assert(!defaults.config.interactive);
    assert(
        defaults.config.announcementMode == mafia::AnnouncementMode::Closed
    );
    assert(defaults.config.logLevel == mafia::LogLevel::Brief);
    assert(defaults.config.logDirectory == "logs");

    assert(parse({"--help"}).showHelp);
    assert(throwsInvalidArgument([] { parse({}); }));
    assert(throwsInvalidArgument([] { parse({"--players"}); }));
    assert(throwsInvalidArgument([] {
        parse({"--players", "ten"});
    }));
    assert(throwsInvalidArgument([] {
        parse({"--players", "10", "--unknown"});
    }));
    assert(throwsInvalidArgument([] {
        parse({"--players", "10", "--log-directory"});
    }));
    assert(throwsInvalidArgument([] {
        parse({
            "--players",
            "10",
            "--open-announcements",
            "--closed-announcements",
        });
    }));
}
