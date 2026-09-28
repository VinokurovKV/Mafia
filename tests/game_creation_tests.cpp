#include "mafia/game.hpp"

#include <algorithm>
#include <cassert>
#include <set>
#include <stdexcept>

namespace {

void verifyParty(std::size_t playerCount, std::size_t mafiaDivisor) {
    mafia::Game game(playerCount, mafiaDivisor);
    const mafia::GameSnapshot snapshot = game.snapshot();

    assert(snapshot.round == 1);
    assert(snapshot.phase == mafia::GamePhase::Day);
    assert(!snapshot.winner.has_value());
    assert(snapshot.players.size() == playerCount);

    std::set<mafia::PlayerId> ids;
    std::size_t mafiaCount = 0;
    std::size_t civilianCount = 0;
    std::size_t doctorCount = 0;
    std::size_t commissionerCount = 0;
    std::size_t maniacCount = 0;

    for (const mafia::PlayerState& player : snapshot.players) {
        assert(player.alive);
        ids.insert(player.id);

        switch (game.roleOf(player.id)) {
            case mafia::RoleType::Mafia:
                ++mafiaCount;
                break;
            case mafia::RoleType::Civilian:
                ++civilianCount;
                break;
            case mafia::RoleType::Doctor:
                ++doctorCount;
                break;
            case mafia::RoleType::Commissioner:
                ++commissionerCount;
                break;
            case mafia::RoleType::Maniac:
                ++maniacCount;
                break;
            case mafia::RoleType::Eavesdropper:
            case mafia::RoleType::Witness:
            case mafia::RoleType::Bull:
                assert(false);
                break;
        }
    }

    assert(ids.size() == playerCount);
    for (mafia::PlayerId id = 1; id <= playerCount; ++id) {
        assert(ids.contains(id));
    }

    const std::size_t expectedMafia = std::max<std::size_t>(
        1,
        playerCount / mafiaDivisor
    );
    assert(mafiaCount == expectedMafia);
    assert(doctorCount == 1);
    assert(commissionerCount == 1);
    assert(maniacCount == 1);
    assert(civilianCount == playerCount - expectedMafia - 3);
}

void verifyConfiguredParty() {
    mafia::GameConfig config;
    config.playerCount = 10;
    config.roleConfigFile = "config/roles.yaml";
    mafia::Game game(config);

    std::size_t mafiaClanCount = 0;
    std::size_t bullCount = 0;
    std::size_t eavesdropperCount = 0;
    std::size_t witnessCount = 0;
    for (const mafia::PlayerState& player : game.snapshot().players) {
        const mafia::RoleType role = game.roleOf(player.id);
        mafiaClanCount += mafia::isMafiaRole(role);
        bullCount += role == mafia::RoleType::Bull;
        eavesdropperCount += role == mafia::RoleType::Eavesdropper;
        witnessCount += role == mafia::RoleType::Witness;
    }

    assert(mafiaClanCount == 3);
    assert(bullCount == 1);
    assert(eavesdropperCount == 1);
    assert(witnessCount == 1);
}

void verifyInvalidConfiguration() {
    bool invalidPlayerCount = false;
    try {
        mafia::Game game(4);
        static_cast<void>(game);
    } catch (const std::invalid_argument&) {
        invalidPlayerCount = true;
    }
    assert(invalidPlayerCount);

    bool invalidDivisor = false;
    try {
        mafia::Game game(10, 2);
        static_cast<void>(game);
    } catch (const std::invalid_argument&) {
        invalidDivisor = true;
    }
    assert(invalidDivisor);

    bool tooFewPlayersForConfiguredRoles = false;
    try {
        mafia::GameConfig config;
        config.playerCount = 5;
        config.roleConfigFile = "config/roles.yaml";
        mafia::Game game(config);
        static_cast<void>(game);
    } catch (const std::invalid_argument&) {
        tooFewPlayersForConfiguredRoles = true;
    }
    assert(tooFewPlayersForConfiguredRoles);

    bool missingRoleConfig = false;
    try {
        mafia::GameConfig config;
        config.playerCount = 10;
        config.roleConfigFile = "config/missing.yaml";
        mafia::Game game(config);
        static_cast<void>(game);
    } catch (const std::runtime_error&) {
        missingRoleConfig = true;
    }
    assert(missingRoleConfig);
}

}  // namespace

int main() {
    verifyParty(5, 3);
    verifyParty(10, 3);
    verifyParty(20, 4);
    verifyConfiguredParty();
    verifyInvalidConfiguration();
}
