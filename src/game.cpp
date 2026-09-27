#include "mafia/game.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "mafia/random_strategy.hpp"
#include "mafia/roles.hpp"

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
    }
    return "acts on";
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

std::string formatTime(std::chrono::system_clock::time_point value) {
    const std::time_t rawTime = std::chrono::system_clock::to_time_t(value);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &rawTime);
#else
    localtime_r(&rawTime, &localTime);
#endif

    std::ostringstream output;
    output << std::put_time(&localTime, "%H:%M:%S");
    return output.str();
}

}  // namespace

Game::Game(std::size_t playerCount, std::size_t mafiaDivisor)
    : Game(GameConfig{
          playerCount,
          mafiaDivisor,
          false,
          AnnouncementMode::Closed,
          LogLevel::Brief,
      }) {}

Game::Game(GameConfig configValue)
    : config(configValue),
      outputEnabled(true) {
    if (config.playerCount <= 4) {
        throw std::invalid_argument("Player count must be greater than four");
    }
    if (config.mafiaDivisor < 3) {
        throw std::invalid_argument("Mafia divisor must be at least three");
    }

    const std::size_t mafiaCount = std::max<std::size_t>(
        1,
        config.playerCount / config.mafiaDivisor
    );

    std::vector<RoleType> roles;
    roles.reserve(config.playerCount);
    roles.insert(roles.end(), mafiaCount, RoleType::Mafia);
    roles.push_back(RoleType::Doctor);
    roles.push_back(RoleType::Commissioner);
    roles.push_back(RoleType::Maniac);
    roles.insert(
        roles.end(),
        config.playerCount - roles.size(),
        RoleType::Civilian
    );

    std::mt19937 generator(std::random_device{}());
    std::shuffle(roles.begin(), roles.end(), generator);

    for (std::size_t index = 0; index < roles.size(); ++index) {
        const PlayerId id = index + 1;
        const std::string name = "Player " + std::to_string(id);
        SharedPtr<DecisionStrategy> strategy(new RandomStrategy());

        switch (roles[index]) {
            case RoleType::Mafia:
                addPlayer<Mafia>(id, name, strategy);
                break;
            case RoleType::Civilian:
                addPlayer<Civilian>(id, name, strategy);
                break;
            case RoleType::Doctor:
                addPlayer<Doctor>(id, name, strategy);
                break;
            case RoleType::Commissioner:
                addPlayer<Commissioner>(id, name, strategy);
                break;
            case RoleType::Maniac:
                addPlayer<Maniac>(id, name, strategy);
                break;
        }
    }
}

void Game::run() {
    if (players.empty()) {
        return;
    }

    announceGameStart();

    try {
        startPlayerThreads();

        while (!checkVictory()) {
            state.phase = GamePhase::Voting;
            announcePhase(GamePhase::Voting);
            const StepResult votingResult = host.conductStep(
                {nextStepId++, GamePhase::Voting},
                snapshot()
            );
            announceStepResult(GamePhase::Voting, votingResult);
            applyStepResult(votingResult);

            if (checkVictory()) {
                break;
            }

            state.phase = GamePhase::Night;
            announcePhase(GamePhase::Night);
            const StepResult nightResult = host.conductStep(
                {nextStepId++, GamePhase::Night},
                snapshot()
            );
            announceStepResult(GamePhase::Night, nightResult);
            applyStepResult(nightResult);

            if (!checkVictory()) {
                ++state.round;
            }
        }

        state.phase = GamePhase::Finished;
        announceWinner();
    } catch (...) {
        stopPlayerThreads();
        throw;
    }

    stopPlayerThreads();
}

GameSnapshot Game::snapshot() const {
    return GameSnapshot{
        state.round,
        state.phase,
        state.players,
        state.lastDoctorTarget,
        state.winner,
    };
}

RoleType Game::roleOf(PlayerId id) const {
    const auto player = std::find_if(
        players.begin(),
        players.end(),
        [id](const SharedPtr<Player>& current) {
            return current->id() == id;
        }
    );

    if (player == players.end()) {
        throw std::out_of_range("Unknown player id");
    }
    return (*player)->role();
}

void Game::startPlayerThreads() {
    playerThreads.reserve(players.size());
    for (const SharedPtr<Player>& player : players) {
        playerThreads.emplace_back(&Player::run, player.get());
    }
}

void Game::stopPlayerThreads() noexcept {
    for (const SharedPtr<Player>& player : players) {
        player->stop();
    }

    for (std::thread& thread : playerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    playerThreads.clear();
}

void Game::applyStepResult(const StepResult& result) {
    if (result.doctorTarget.has_value()) {
        state.lastDoctorTarget = result.doctorTarget;
    }

    for (const PlayerId eliminatedId : result.eliminated) {
        const auto player = std::find_if(
            state.players.begin(),
            state.players.end(),
            [eliminatedId](const PlayerState& playerState) {
                return playerState.id == eliminatedId;
            }
        );

        if (player != state.players.end()) {
            player->alive = false;
        }
    }
}

bool Game::checkVictory() {
    std::size_t mafiaCount = 0;
    std::size_t civilianCount = 0;
    std::size_t maniacCount = 0;

    for (const SharedPtr<Player>& player : players) {
        const auto stateEntry = std::find_if(
            state.players.begin(),
            state.players.end(),
            [&player](const PlayerState& current) {
                return current.id == player->id();
            }
        );

        if (stateEntry == state.players.end() || !stateEntry->alive) {
            continue;
        }

        switch (player->role()) {
            case RoleType::Mafia:
                ++mafiaCount;
                break;
            case RoleType::Maniac:
                ++maniacCount;
                break;
            case RoleType::Civilian:
            case RoleType::Doctor:
            case RoleType::Commissioner:
                ++civilianCount;
                break;
        }
    }

    state.winner.reset();
    if (mafiaCount == 0 && maniacCount == 0) {
        state.winner = Winner::Civilians;
    } else if (
        mafiaCount == 0 &&
        maniacCount > 0 &&
        civilianCount <= 1
    ) {
        state.winner = Winner::Maniac;
    } else {
        const std::size_t nonMafiaCount = civilianCount + maniacCount;
        if (
            mafiaCount > nonMafiaCount ||
            (mafiaCount == nonMafiaCount && maniacCount == 0)
        ) {
            state.winner = Winner::Mafia;
        }
    }

    return state.winner.has_value();
}

void Game::announceGameStart() const {
    if (!outputEnabled) {
        return;
    }

    const std::size_t mafiaCount = static_cast<std::size_t>(std::count_if(
        players.begin(),
        players.end(),
        [](const SharedPtr<Player>& player) {
            return player->role() == RoleType::Mafia;
        }
    ));

    std::cout
        << "=== Mafia game started ===\n"
        << "Players: " << players.size()
        << ", mafia: " << mafiaCount << '\n'
        << "Announcements: "
        << (config.announcementMode == AnnouncementMode::Open
            ? "open"
            : "closed")
        << ", log: "
        << (config.logLevel == LogLevel::Full ? "full" : "brief")
        << "\n";

    if (config.logLevel == LogLevel::Full) {
        std::cout << "Role distribution:\n";
        for (const SharedPtr<Player>& player : players) {
            std::cout
                << "  " << player->name() << " (#" << player->id() << "): "
                << roleName(player->role()) << '\n';
        }
    }
}

void Game::announcePhase(GamePhase phase) const {
    if (!outputEnabled) {
        return;
    }

    if (phase == GamePhase::Voting) {
        std::cout
            << "\n=== Day " << state.round << " ===\n"
            << "Voting has started.\n";
    } else if (phase == GamePhase::Night) {
        std::cout
            << "\n=== Night " << state.round << " ===\n"
            << "Night roles are making their choices.\n";
    }
}

void Game::announceStepResult(
    GamePhase phase,
    const StepResult& result
) const {
    if (!outputEnabled) {
        return;
    }

    const bool fullLog = config.logLevel == LogLevel::Full;
    const bool openAnnouncements =
        config.announcementMode == AnnouncementMode::Open;

    if (fullLog) {
        std::cout
            << "Accepted actions (last received at "
            << formatTime(result.completedAt) << "):\n";
        for (const Action& action : result.actions) {
            std::cout
                << "  " << playerName(action.actor) << ' '
                << actionName(action.type) << ' '
                << playerName(action.target) << '\n';
        }
    } else if (phase == GamePhase::Night && openAnnouncements) {
        for (const Action& action : result.actions) {
            if (
                action.type == ActionType::Check ||
                action.type == ActionType::MafiaKill
            ) {
                continue;
            }
            std::cout
                << "  " << actionName(action.type) << ' '
                << playerName(action.target) << '\n';
        }
    }

    if (
        phase == GamePhase::Night &&
        result.mafiaTarget.has_value() &&
        (fullLog || openAnnouncements)
    ) {
        std::cout
            << "Mafia common target: "
            << playerName(*result.mafiaTarget);
        if (fullLog && result.mafiaConsensusRequired) {
            std::cout << " (agreed after discussion)";
        }
        std::cout << "\n";
    }

    if (fullLog) {
        for (const InvestigationResult& investigation : result.investigations) {
            std::cout
                << "  Check result for "
                << playerName(investigation.investigator) << ": "
                << playerName(investigation.target) << " is "
                << (investigation.targetIsMafia ? "mafia" : "not mafia")
                << '\n';
        }
    }

    if (result.eliminated.empty()) {
        std::cout << (phase == GamePhase::Voting
            ? "Voting ended in a tie. Nobody was eliminated.\n"
            : "Nobody was eliminated during the night.\n");
        return;
    }

    for (const PlayerId eliminated : result.eliminated) {
        std::cout
            << playerName(eliminated) << " was eliminated ("
            << disclosedStatus(eliminated) << ").\n";
    }
}

void Game::announceWinner() const {
    if (!outputEnabled || !state.winner.has_value()) {
        return;
    }

    std::cout
        << "\n=== Game finished ===\n"
        << "Winner: " << winnerName(*state.winner) << "\n";
}

std::string Game::playerName(PlayerId id) const {
    const auto player = std::find_if(
        players.begin(),
        players.end(),
        [id](const SharedPtr<Player>& current) {
            return current->id() == id;
        }
    );

    if (player == players.end()) {
        return "Unknown player #" + std::to_string(id);
    }
    return (*player)->name() + " (#" + std::to_string(id) + ")";
}

std::string Game::disclosedStatus(PlayerId id) const {
    const RoleType role = roleOf(id);
    if (config.announcementMode == AnnouncementMode::Open) {
        return std::string(roleName(role));
    }
    return role == RoleType::Mafia ? "mafia" : "not mafia";
}

}  // namespace mafia
