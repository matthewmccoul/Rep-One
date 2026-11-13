#include <cmath>

#include "test_support.h"
#include "mordor/death1a.h"

using namespace mordor;

namespace {

template <typename E>
i32 ordOf(E e) {
    return static_cast<i32>(e);
}

Character pendingBody() {
    Character c;
    c.raiseChance = 40;
    c.raiseKind = RaiseKind::RaiseDead;
    return c;
}

}  // namespace

TEST(dead_screen_owed_only_when_dead_and_alone) {
    Party party;
    Character who;
    party.size = 1;
    CHECK(!deadScreenOwed(who, party));  // alive
    who.deathState = DeathState::Dead;
    CHECK(deadScreenOwed(who, party));
    party.size = 2;
    CHECK(!deadScreenOwed(who, party));
}

