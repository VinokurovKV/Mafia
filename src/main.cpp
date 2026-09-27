#include "mafia/command_line.hpp"
#include "mafia/game.hpp"

#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string_view> arguments;
    arguments.reserve(static_cast<std::size_t>(argc - 1));
    for (int index = 1; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }

    try {
        const mafia::CommandLineOptions options =
            mafia::parseCommandLine(arguments);

        if (options.showHelp) {
            std::cout << mafia::commandLineUsage(argv[0]);
            return 0;
        }
        mafia::Game game(options.config);
        game.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        std::cerr << mafia::commandLineUsage(argv[0]);
        return 1;
    }
}
