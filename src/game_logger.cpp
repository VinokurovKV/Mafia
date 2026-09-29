#include "mafia/game_logger.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace mafia {
namespace {

std::string_view roleName(RoleType role) {
    switch (role) {
        case RoleType::Mafia:
            return "Mafia";
        case RoleType::Civilian:
            return "Civilian";
        case RoleType::Doctor:
            return "Doctor";
        case RoleType::Commissioner:
            return "Commissioner";
        case RoleType::Maniac:
            return "Maniac";
        case RoleType::Eavesdropper:
            return "Eavesdropper";
        case RoleType::Witness:
            return "Witness";
        case RoleType::Bull:
            return "Bull";
    }
    return "Unknown";
}

std::string_view actionName(ActionType action) {
    switch (action) {
        case ActionType::Vote:
            return "votes against";
        case ActionType::MafiaKill:
            return "mafia targets";
        case ActionType::Heal:
            return "protects";
        case ActionType::Check:
            return "checks";
        case ActionType::Shoot:
            return "shoots";
        case ActionType::ManiacKill:
            return "maniac targets";
        case ActionType::Listen:
            return "listens at";
        case ActionType::Observe:
            return "observes";
    }
    return "acts on";
}

std::string_view phaseName(GamePhase phase) {
    switch (phase) {
        case GamePhase::Day:
            return "Day";
        case GamePhase::Voting:
            return "Voting";
        case GamePhase::Night:
            return "Night";
        case GamePhase::Finished:
            return "Finished";
    }
    return "Unknown";
}

std::string_view winnerName(Winner winner) {
    switch (winner) {
        case Winner::Civilians:
            return "Civilians";
        case Winner::Mafia:
            return "Mafia";
        case Winner::Maniac:
            return "Maniac";
    }
    return "Unknown";
}

std::tm localTime(std::time_t value) {
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &value);
#else
    localtime_r(&value, &result);
#endif
    return result;
}

std::string sessionName() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t rawTime = std::chrono::system_clock::to_time_t(now);
    const std::tm time = localTime(rawTime);

    std::ostringstream name;
    name << "game_" << std::put_time(&time, "%Y%m%d_%H%M%S");
    return name.str();
}

std::string formatTime(std::chrono::system_clock::time_point value) {
    const std::time_t rawTime = std::chrono::system_clock::to_time_t(value);
    const std::tm time = localTime(rawTime);

    std::ostringstream output;
    output << std::put_time(&time, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

}  // namespace

GameLogger::GameLogger(std::filesystem::path rootDirectory)
    : rootDirectory_(std::move(rootDirectory)) {}

void GameLogger::startGame(std::vector<LoggedPlayer> players) {
    if (started_) {
        throw std::logic_error("Game logger is already started");
    }

    std::error_code error;
    std::filesystem::create_directories(rootDirectory_, error);
    if (error) {
        throw std::runtime_error(
            "Cannot create log directory: " + error.message()
        );
    }

    const std::string baseName = sessionName();
    sessionDirectory_ = rootDirectory_ / baseName;
    std::size_t suffix = 1;
    while (!std::filesystem::create_directory(sessionDirectory_, error)) {
        if (error) {
            throw std::runtime_error(
                "Cannot create game log directory: " + error.message()
            );
        }
        sessionDirectory_ = rootDirectory_ /
            (baseName + "_" + std::to_string(suffix++));
    }

    players_ = std::move(players);
    for (const LoggedPlayer& player : players_) {
        statistics_.try_emplace(player.id);
    }
    started_ = true;
}

void GameLogger::logStep(
    int round,
    GamePhase phase,
    const StepResult& result
) {
    if (!started_) {
        throw std::logic_error("Game logger is not started");
    }

    std::ostringstream fileName;
    fileName << "round_" << std::setw(3) << std::setfill('0') << round
             << ".txt";
    const std::filesystem::path path = sessionDirectory_ / fileName.str();
    const bool newFile = !std::filesystem::exists(path);

    std::ofstream output(path, std::ios::app);
    if (!output) {
        throw std::runtime_error("Cannot open round log: " + path.string());
    }

    if (newFile) {
        output << "=== Round " << round << " ===\n";
    }
    output
        << "\n--- " << phaseName(phase) << " ---\n"
        << "Completed at: " << formatTime(result.completedAt) << "\n"
        << "Submitted actions:\n";

    const std::vector<Action>& history = result.actionHistory.empty()
        ? result.actions
        : result.actionHistory;
    if (history.empty()) {
        output << "  none\n";
    } else {
        for (const Action& action : history) {
            output
                << "  " << playerLabel(action.actor) << ' '
                << actionName(action.type) << ' '
                << playerLabel(action.target) << "\n";
            if (!action.message.empty()) {
                output << "    Message: " << action.message << "\n";
            }
            if (!action.reasoning.empty()) {
                output << "    Reasoning: " << action.reasoning << "\n";
            }
        }
    }

    if (phase == GamePhase::Night) {
        if (result.mafiaTarget.has_value()) {
            output
                << "Mafia common target: "
                << playerLabel(*result.mafiaTarget)
                << (result.mafiaConsensusRequired
                    ? " (discussion required)\n"
                    : "\n");
        }
        if (result.doctorTarget.has_value()) {
            output
                << "Doctor protected: "
                << playerLabel(*result.doctorTarget) << "\n";
        }
        for (const InvestigationResult& investigation : result.investigations) {
            output
                << "Investigation by "
                << playerLabel(investigation.investigator) << ": "
                << playerLabel(investigation.target) << " is "
                << (investigation.targetIsMafia ? "mafia" : "not mafia")
                << "\n";
        }
        for (const EavesdropResult& observation : result.eavesdropResults) {
            output
                << "Eavesdrop result for "
                << playerLabel(observation.listener) << " at "
                << playerLabel(observation.target) << ":";
            if (observation.directedActions.empty()) {
                output << " no directed actions";
            } else {
                for (const ActionType action : observation.directedActions) {
                    output << ' ' << actionName(action);
                }
            }
            output << "\n";
        }
        for (const WitnessResult& observation : result.witnessResults) {
            output
                << "Witness result for "
                << playerLabel(observation.witness) << " watching "
                << playerLabel(observation.target) << ":";
            if (observation.attackers.empty()) {
                output << " no attack";
            } else {
                output << " attackers";
                for (const PlayerId attacker : observation.attackers) {
                    output << ' ' << playerLabel(attacker);
                }
            }
            output << "\n";
        }
    }

    output << "Eliminated:";
    if (result.eliminated.empty()) {
        output << " none";
    } else {
        for (const PlayerId id : result.eliminated) {
            output << ' ' << playerLabel(id);
        }
    }
    output << "\n";

    if (!output) {
        throw std::runtime_error("Cannot write round log: " + path.string());
    }
    updateStatistics(round, phase, result);
}

void GameLogger::finishGame(
    Winner winner,
    const std::vector<PlayerState>& states
) {
    if (!started_) {
        throw std::logic_error("Game logger is not started");
    }

    const std::filesystem::path path = sessionDirectory_ / "summary.txt";
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Cannot open summary log: " + path.string());
    }

    output << "=== Game summary ===\nWinner: " << winnerName(winner) << "\n";
    for (const LoggedPlayer& player : players_) {
        const PlayerStatistics& stats = statistics_.at(player.id);
        const auto state = std::ranges::find(
            states,
            player.id,
            &PlayerState::id
        );
        const bool alive = state != states.end() && state->alive;

        output
            << "\n" << playerLabel(player.id) << "\n"
            << "  Role: " << roleName(player.role) << "\n"
            << "  Final status: " << (alive ? "alive" : "eliminated") << "\n";
        if (stats.eliminatedRound.has_value()) {
            output
                << "  Eliminated: round " << *stats.eliminatedRound
                << ", " << phaseName(*stats.eliminatedPhase) << "\n";
        }
        output
            << "  Submitted actions: " << stats.submittedActions << "\n"
            << "  Votes cast: " << stats.votesCast << "\n"
            << "  Votes received: " << stats.votesReceived << "\n"
            << "  Mafia target selections: " << stats.mafiaTargets << "\n"
            << "  Heals: " << stats.heals << "\n"
            << "  Checks: " << stats.checks << "\n"
            << "  Commissioner shots: " << stats.shots << "\n"
            << "  Maniac target selections: " << stats.maniacTargets << "\n";
        output
            << "  Eavesdrops: " << stats.listens << "\n"
            << "  Witness observations: " << stats.observations << "\n";
    }

    if (!output) {
        throw std::runtime_error("Cannot write summary log: " + path.string());
    }
}

const std::filesystem::path& GameLogger::sessionDirectory() const noexcept {
    return sessionDirectory_;
}

std::string GameLogger::playerLabel(PlayerId id) const {
    const auto player = std::ranges::find(
        players_,
        id,
        &LoggedPlayer::id
    );
    if (player == players_.end()) {
        return "Unknown player (#" + std::to_string(id) + ")";
    }
    return player->name + " (#" + std::to_string(id) + ")";
}

void GameLogger::updateStatistics(
    int round,
    GamePhase phase,
    const StepResult& result
) {
    const std::vector<Action>& history = result.actionHistory.empty()
        ? result.actions
        : result.actionHistory;
    for (const Action& action : history) {
        ++statistics_.at(action.actor).submittedActions;
    }

    for (const Action& action : result.actions) {
        PlayerStatistics& actor = statistics_.at(action.actor);
        switch (action.type) {
            case ActionType::Vote:
                ++actor.votesCast;
                ++statistics_.at(action.target).votesReceived;
                break;
            case ActionType::MafiaKill:
                ++actor.mafiaTargets;
                break;
            case ActionType::Heal:
                ++actor.heals;
                break;
            case ActionType::Check:
                ++actor.checks;
                break;
            case ActionType::Shoot:
                ++actor.shots;
                break;
            case ActionType::ManiacKill:
                ++actor.maniacTargets;
                break;
            case ActionType::Listen:
                ++actor.listens;
                break;
            case ActionType::Observe:
                ++actor.observations;
                break;
        }
    }

    for (const PlayerId eliminated : result.eliminated) {
        PlayerStatistics& stats = statistics_.at(eliminated);
        if (!stats.eliminatedRound.has_value()) {
            stats.eliminatedRound = round;
            stats.eliminatedPhase = phase;
        }
    }
}

}  // namespace mafia
