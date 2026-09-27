#include "mafia/game_logger.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    assert(input);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>(),
    };
}

}  // namespace

int main() {
    const auto uniqueValue = std::chrono::steady_clock::now()
        .time_since_epoch()
        .count();
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("mafia_logger_test_" + std::to_string(uniqueValue));

    try {
        mafia::GameLogger logger(root);
        logger.startGame({
            {1, "First", mafia::RoleType::Mafia},
            {2, "Second", mafia::RoleType::Doctor},
        });

        mafia::StepResult voting;
        voting.completedAt = std::chrono::system_clock::now();
        voting.actions = {
            {1, 1, mafia::ActionType::Vote, 2},
            {1, 2, mafia::ActionType::Vote, 1},
        };
        voting.actionHistory = voting.actions;
        logger.logStep(1, mafia::GamePhase::Voting, voting);

        mafia::StepResult night;
        night.completedAt = std::chrono::system_clock::now();
        night.actions = {
            {2, 1, mafia::ActionType::MafiaKill, 2},
            {2, 2, mafia::ActionType::Heal, 2},
        };
        night.actionHistory = night.actions;
        night.mafiaTarget = 2;
        night.doctorTarget = 2;
        logger.logStep(1, mafia::GamePhase::Night, night);
        logger.finishGame(
            mafia::Winner::Mafia,
            {{1, true}, {2, true}}
        );

        const std::filesystem::path round =
            logger.sessionDirectory() / "round_001.txt";
        const std::filesystem::path summary =
            logger.sessionDirectory() / "summary.txt";
        assert(std::filesystem::exists(round));
        assert(std::filesystem::exists(summary));

        const std::string roundText = readFile(round);
        assert(roundText.find("--- Voting ---") != std::string::npos);
        assert(roundText.find("--- Night ---") != std::string::npos);
        assert(roundText.find("First (#1) votes against Second (#2)") !=
            std::string::npos);
        assert(roundText.find("Doctor protected: Second (#2)") !=
            std::string::npos);

        const std::string summaryText = readFile(summary);
        assert(summaryText.find("Winner: Mafia") != std::string::npos);
        assert(summaryText.find("Votes cast: 1") != std::string::npos);
        assert(summaryText.find("Heals: 1") != std::string::npos);
    } catch (...) {
        std::filesystem::remove_all(root);
        throw;
    }

    std::filesystem::remove_all(root);
}
