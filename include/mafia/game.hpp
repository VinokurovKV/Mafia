#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "mafia/host.hpp"
#include "mafia/player.hpp"
#include "mafia/shared_ptr.hpp"
#include "mafia/types.hpp"

namespace mafia {

class Game {
public:
    template <typename Role>
    void addPlayer(
        PlayerId id,
        std::string name,
        SharedPtr<DecisionStrategy> strategy
    ) {
        const auto duplicate = std::find_if(
            state.players.begin(),
            state.players.end(),
            [id](const PlayerState& player) {
                return player.id == id;
            }
        );
        if (duplicate != state.players.end()) {
            throw std::logic_error("Player id is already used by Game");
        }

        SharedPtr<Player> player(
            new Role(id, std::move(name), std::move(strategy), host)
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

private:
    void startPlayerThreads();
    void stopPlayerThreads() noexcept;
    void applyStepResult(const StepResult& result);
    bool checkVictory() const;

    GameState state{1, GamePhase::Day, {}};
    Host host;
    std::vector<SharedPtr<Player>> players;
    std::vector<std::thread> playerThreads;
    StepId nextStepId = 1;
};

}  // namespace mafia
