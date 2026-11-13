#include "test_support.h"
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/combat2a.h"
#include "mordor/combat3.h"

using namespace mordor;

namespace {

constexpr i32 kRaceCount = 9;

void makeSoloParty(Party& p, Race r) {
    p = Party{};
    p.size = 1;
    Character& c = p.members[0];
    c.race = r;
    for (i32 s = 0; s < kStatCount; ++s) {
        c.abilities.stats[s] = 15;
    }
    c.vitals.curHits = 500;
    c.vitals.maxHits = 500;
}

}  // namespace

// B_0064#19: race table contains a race with zero poison and zero disease resistance.
TEST(engine_B_0064_19_zero_poison_disease_race) {
    tsup::Env env({0.5, 0.9});
    bool found = false;
    for (i32 i = 0; i < kRaceCount; ++i) {
        Race r = static_cast<Race>(i);
        if (raceResistBase(env.ctx, r, DamageKind::Poison) == 0 &&
            raceResistBase(env.ctx, r, DamageKind::Disease) == 0) {
            found = true;
        }
    }
    CHECK(found);
}

// B_0064#22: some race has poison resistance greater than zero.
TEST(engine_B_0064_22_poison_resist_race_exists) {
    tsup::Env env({0.5, 0.9});
    bool found = false;
    for (i32 i = 0; i < kRaceCount; ++i) {
        Race r = static_cast<Race>(i);
        if (raceResistBase(env.ctx, r, DamageKind::Poison) > 0) {
            found = true;
        }
    }
    CHECK(found);
}

// SKIP B_0064#25: the "10 passed as the trap argument" and the 90% fire spell resistance setup are not expressible from the headers (chestTrapDamage takes no count, and resistance application path is unclear).
// SKIP B_0064#26: depends on the same unresolved fire-trap setup as B_0064#25.
// SKIP B_0064#40: needs EventLog inspection and companion setup; no EventLog accessor is shown in the headers.
// SKIP B_0064#41: needs EventLog inspection to count distinct message shapes; no EventLog accessor is shown in the headers.
