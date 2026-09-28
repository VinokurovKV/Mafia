#pragma once

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "mafia/host.hpp"
#include "mafia/game_logger.hpp"
#include "mafia/player.hpp"
#include "mafia/role_concepts.hpp"
#include "mafia/shared_ptr.hpp"
#include "mafia/types.hpp"

namespace mafia {

class Game {
public:
    Game() = default;
    explicit Game(
        std::size_t playerCount,
        std::size_t mafiaDivisor = 3
    );
    explicit Game(GameConfig config);

    template <PlayerRole Role>
    void addPlayer(
        PlayerId id,
        std::string name,
        SharedPtr<DecisionStrategy> strategy
    ) {
        const auto duplicate = std::ranges::find(
            state.players,
            id,
            &PlayerState::id
        );
        if (duplicate != state.players.end()) {
            throw std::logic_error("Player id is already used by Game");
        }

        SharedPtr<Player> player(
            new Role(id, std::move(name), std::move(strategy))
        );

        players.push_back(player);
        try {
            state.players.push_back({id, true});
            host.registerPlayer(*player);
        } catch (...) {
            if (!state.players.empty() && state.players.back().id == id) {
                state.players.pop_back();
            }
            players.pop_back();
            throw;
        }
    }

    void run();
    GameSnapshot snapshot() const;
    RoleType roleOf(PlayerId id) const;

private:
    void applyStepResult(const StepResult& result);
    bool checkVictory();
    void announceGameStart() const;
    void announcePhase(GamePhase phase) const;
    void announceStepResult(
        GamePhase phase,
        const StepResult& result
    ) const;
    void announceWinner() const;
    void startFileLogging();
    std::string playerName(PlayerId id) const;
    std::string disclosedStatus(PlayerId id) const;

    GameState state{
        1,
        GamePhase::Day,
        {},
        std::nullopt,
        std::nullopt,
    };
    Host host;
    std::vector<SharedPtr<Player>> players;
    StepId nextStepId = 1;
    GameConfig config;
    bool outputEnabled = false;
    std::optional<PlayerId> humanPlayerId;
    std::optional<GameLogger> gameLogger;
};

}  // namespace mafia
