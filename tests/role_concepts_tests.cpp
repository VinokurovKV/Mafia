#include "mafia/role_concepts.hpp"
#include "mafia/roles.hpp"

namespace {

struct NotAPlayer {};

class AbstractPlayer : public mafia::Player {
public:
    using Player::Player;
    static constexpr bool participatesInVoting = true;
    static constexpr bool actsAtNight = true;
};

static_assert(mafia::PlayerRole<mafia::Mafia>);
static_assert(mafia::PlayerRole<mafia::Civilian>);
static_assert(mafia::PlayerRole<mafia::Doctor>);
static_assert(mafia::PlayerRole<mafia::Commissioner>);
static_assert(mafia::PlayerRole<mafia::Maniac>);
static_assert(mafia::PlayerRole<mafia::Eavesdropper>);
static_assert(mafia::PlayerRole<mafia::Witness>);
static_assert(mafia::PlayerRole<mafia::Bull>);

static_assert(mafia::VotingRole<mafia::Mafia>);
static_assert(mafia::VotingRole<mafia::Civilian>);
static_assert(mafia::VotingRole<mafia::Doctor>);
static_assert(mafia::VotingRole<mafia::Commissioner>);
static_assert(mafia::VotingRole<mafia::Maniac>);
static_assert(mafia::VotingRole<mafia::Eavesdropper>);
static_assert(mafia::VotingRole<mafia::Witness>);
static_assert(mafia::VotingRole<mafia::Bull>);

static_assert(mafia::NightActingRole<mafia::Mafia>);
static_assert(!mafia::NightActingRole<mafia::Civilian>);
static_assert(mafia::NightActingRole<mafia::Doctor>);
static_assert(mafia::NightActingRole<mafia::Commissioner>);
static_assert(mafia::NightActingRole<mafia::Maniac>);
static_assert(mafia::NightActingRole<mafia::Eavesdropper>);
static_assert(mafia::NightActingRole<mafia::Witness>);
static_assert(mafia::NightActingRole<mafia::Bull>);

static_assert(!mafia::PlayerRole<NotAPlayer>);
static_assert(!mafia::PlayerRole<AbstractPlayer>);

}  // namespace

int main() {}
