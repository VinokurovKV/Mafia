#include "mafia/game.hpp"

#include <algorithm>

namespace mafia {

void Game::run() {
    if (players.empty()) {
        return;
    }

    try {
        startPlayerThreads();
        state.phase = GamePhase::Voting;

        const StepRequest request{nextStepId++, GamePhase::Voting};
        const StepResult result = host.conductStep(request, snapshot());
        applyStepResult(result);
    } catch (...) {
        stopPlayerThreads();
        throw;
    }

    stopPlayerThreads();
}

GameSnapshot Game::snapshot() const {
    return GameSnapshot{state.round, state.phase, state.players};
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

bool Game::checkVictory() const {
    return false;
}

}  // namespace mafia
