#include "mafia/game.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>

#include "mafia/console_strategy.hpp"
#include "mafia/random_strategy.hpp"
#include "mafia/role_config.hpp"
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
          "logs",
          "",
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

    const std::vector<RoleType> additionalRoles =
        config.roleConfigFile.empty()
        ? std::vector<RoleType>{}
        : loadAdditionalRoles(config.roleConfigFile);
    const bool bullEnabled =
        std::ranges::find(additionalRoles, RoleType::Bull) !=
        additionalRoles.end();

    std::vector<RoleType> roles;
    roles.reserve(config.playerCount);
    roles.insert(
        roles.end(),
        mafiaCount - static_cast<std::size_t>(bullEnabled),
        RoleType::Mafia
    );
    if (bullEnabled) {
        roles.push_back(RoleType::Bull);
    }
    roles.push_back(RoleType::Doctor);
    roles.push_back(RoleType::Commissioner);
    roles.push_back(RoleType::Maniac);
    for (const RoleType role : additionalRoles) {
        if (role != RoleType::Bull) {
            roles.push_back(role);
        }
    }
    if (roles.size() > config.playerCount) {
        throw std::invalid_argument(
            "Player count is too small for the configured roles"
        );
    }
    roles.insert(
        roles.end(),
        config.playerCount - roles.size(),
        RoleType::Civilian
    );

    std::mt19937 generator(std::random_device{}());
    std::ranges::shuffle(roles, generator);

    for (std::size_t index = 0; index < roles.size(); ++index) {
        const PlayerId id = index + 1;
        const std::string name = "Player " + std::to_string(id);
        SharedPtr<DecisionStrategy> strategy;
        if (config.interactive && id == 1) {
            strategy.reset(new ConsoleStrategy(std::cin, std::cout));
            humanPlayerId = id;
        } else {
            strategy.reset(new RandomStrategy());
        }

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
            case RoleType::Eavesdropper:
                addPlayer<Eavesdropper>(id, name, strategy);
                break;
            case RoleType::Witness:
                addPlayer<Witness>(id, name, strategy);
                break;
            case RoleType::Bull:
                addPlayer<Bull>(id, name, strategy);
                break;
        }
    }
}

void Game::run() {
    if (players.empty()) {
        return;
    }

    startFileLogging();
    announceGameStart();

    while (!checkVictory()) {
        state.phase = GamePhase::Voting;
        announcePhase(GamePhase::Voting);
        const StepResult votingResult = host.conductStep(
            {nextStepId++, GamePhase::Voting},
            snapshot()
        );
        if (gameLogger.has_value()) {
            gameLogger->logStep(
                state.round,
                GamePhase::Voting,
                votingResult
            );
        }
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
        if (gameLogger.has_value()) {
            gameLogger->logStep(
                state.round,
                GamePhase::Night,
                nightResult
            );
        }
        announceStepResult(GamePhase::Night, nightResult);
        applyStepResult(nightResult);

        if (!checkVictory()) {
            ++state.round;
        }
    }

    state.phase = GamePhase::Finished;
    if (gameLogger.has_value()) {
        gameLogger->finishGame(*state.winner, state.players);
    }
    announceWinner();
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
    const auto player = std::ranges::find_if(
        players,
        [id](const SharedPtr<Player>& current) {
            return current->id() == id;
        }
    );

    if (player == players.end()) {
        throw std::out_of_range("Unknown player id");
    }
    return (*player)->role();
}

void Game::applyStepResult(const StepResult& result) {
    if (result.doctorTarget.has_value()) {
        state.lastDoctorTarget = result.doctorTarget;
    }

    for (const PlayerId eliminatedId : result.eliminated) {
        const auto player = std::ranges::find(
            state.players,
            eliminatedId,
            &PlayerState::id
        );

        if (player != state.players.end()) {
            player->alive = false;
        }
    }
}

bool Game::checkVictory() {
    const auto isAlive = [this](const SharedPtr<Player>& player) {
        const auto stateEntry = std::ranges::find(
            state.players,
            player->id(),
            &PlayerState::id
        );
        return stateEntry != state.players.end() && stateEntry->alive;
    };

    const auto countLivingRoles = [this, &isAlive](auto predicate) {
        return static_cast<std::size_t>(std::ranges::count_if(
            players,
            [&isAlive, &predicate](const SharedPtr<Player>& player) {
                return isAlive(player) && predicate(player->role());
            }
        ));
    };

    const std::size_t mafiaCount = countLivingRoles([](RoleType role) {
        return isMafiaRole(role);
    });
    const std::size_t maniacCount = countLivingRoles([](RoleType role) {
        return role == RoleType::Maniac;
    });
    const std::size_t civilianCount = countLivingRoles([](RoleType role) {
        return !isMafiaRole(role) && role != RoleType::Maniac;
    });

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

    const std::size_t mafiaCount = static_cast<std::size_t>(
        std::ranges::count_if(
            players,
            [](const SharedPtr<Player>& player) {
                return isMafiaRole(player->role());
            }
        )
    );

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

    if (gameLogger.has_value()) {
        std::cout
            << "File logs: "
            << gameLogger->sessionDirectory().string() << "\n";
    }

    if (
        config.logLevel == LogLevel::Full &&
        config.announcementMode == AnnouncementMode::Open
    ) {
        std::cout << "Role distribution:\n";
        for (const SharedPtr<Player>& player : players) {
            std::cout
                << "  " << player->name() << " (#" << player->id() << "): "
                << roleName(player->role()) << '\n';
        }
    }

    if (humanPlayerId.has_value()) {
        const RoleType humanRole = roleOf(*humanPlayerId);
        std::cout
            << "You control " << playerName(*humanPlayerId) << ".\n"
            << "Your role: " << roleName(humanRole) << ".\n";

        if (isMafiaRole(humanRole)) {
            std::cout << "Your mafia teammates:";
            bool hasTeammates = false;
            for (const SharedPtr<Player>& player : players) {
                if (
                    player->id() != *humanPlayerId &&
                    isMafiaRole(player->role())
                ) {
                    std::cout << ' ' << playerName(player->id());
                    hasTeammates = true;
                }
            }
            if (!hasTeammates) {
                std::cout << " none";
            }
            std::cout << "\n";
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
        const std::vector<Action>& history = result.actionHistory.empty()
            ? result.actions
            : result.actionHistory;
        for (const Action& action : history) {
            std::cout
                << "  " << playerName(action.actor) << ' '
                << actionName(action.type) << ' '
                << playerName(action.target) << '\n';
        }
    } else if (phase == GamePhase::Night && openAnnouncements) {
        for (const Action& action : result.actions) {
            if (
                action.type == ActionType::Check ||
                action.type == ActionType::Listen ||
                action.type == ActionType::Observe ||
                action.type == ActionType::MafiaKill
            ) {
                continue;
            }
            std::cout
                << "  " << actionName(action.type) << ' '
                << playerName(action.target) << '\n';
        }
    }

    if (humanPlayerId.has_value()) {
        for (const InvestigationResult& investigation : result.investigations) {
            if (investigation.investigator != *humanPlayerId) {
                continue;
            }
            std::cout
                << "Your investigation: "
                << playerName(investigation.target) << " is "
                << (investigation.targetIsMafia ? "mafia" : "not mafia")
                << ".\n";
        }
        for (const EavesdropResult& observation : result.eavesdropResults) {
            if (observation.listener != *humanPlayerId) {
                continue;
            }
            std::cout
                << "You listened at " << playerName(observation.target)
                << ": ";
            if (observation.directedActions.empty()) {
                std::cout << "no night action was directed there";
            } else {
                std::cout << "directed actions:";
                for (const ActionType action : observation.directedActions) {
                    std::cout << ' ' << actionName(action);
                }
            }
            std::cout << ".\n";
        }
        for (const WitnessResult& observation : result.witnessResults) {
            if (observation.witness != *humanPlayerId) {
                continue;
            }
            std::cout
                << "You observed " << playerName(observation.target)
                << ": ";
            if (observation.attackers.empty()) {
                std::cout << "there was no attack";
            } else {
                std::cout << "attackers:";
                for (const PlayerId attacker : observation.attackers) {
                    std::cout << ' ' << playerName(attacker);
                }
            }
            std::cout << ".\n";
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
        for (const EavesdropResult& observation : result.eavesdropResults) {
            std::cout
                << "  Listen result for "
                << playerName(observation.listener) << " at "
                << playerName(observation.target) << ":";
            if (observation.directedActions.empty()) {
                std::cout << " no directed actions";
            } else {
                for (const ActionType action : observation.directedActions) {
                    std::cout << ' ' << actionName(action);
                }
            }
            std::cout << '\n';
        }
        for (const WitnessResult& observation : result.witnessResults) {
            std::cout
                << "  Witness result for "
                << playerName(observation.witness) << " watching "
                << playerName(observation.target) << ":";
            if (observation.attackers.empty()) {
                std::cout << " no attack";
            } else {
                std::cout << " attackers";
                for (const PlayerId attacker : observation.attackers) {
                    std::cout << ' ' << playerName(attacker);
                }
            }
            std::cout << '\n';
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

void Game::startFileLogging() {
    if (!outputEnabled || gameLogger.has_value()) {
        return;
    }

    std::vector<LoggedPlayer> loggedPlayers;
    loggedPlayers.reserve(players.size());
    for (const SharedPtr<Player>& player : players) {
        loggedPlayers.push_back({
            player->id(),
            player->name(),
            player->role(),
        });
    }

    gameLogger.emplace(config.logDirectory);
    gameLogger->startGame(std::move(loggedPlayers));
}

std::string Game::playerName(PlayerId id) const {
    const auto player = std::ranges::find_if(
        players,
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
    return isMafiaRole(role) ? "mafia" : "not mafia";
}

}  // namespace mafia
