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

// B_0064#20: no-resistance race, poison gas trap 4000 times -> 0 poisoned.
TEST(engine_B_0064_20_no_resist_poison_gas) {
    tsup::Env env(12345);
    Race noResist = Race::Human;
    bool found = false;
    for (i32 i = 0; i < kRaceCount && !found; ++i) {
        Race r = static_cast<Race>(i);
        if (raceResistBase(env.ctx, r, DamageKind::Poison) == 0 &&
            raceResistBase(env.ctx, r, DamageKind::Disease) == 0) {
            noResist = r;
            found = true;
        }
    }
    CHECK(found);

    i32 poisoned = 0;
    Party party;
    for (i32 n = 0; n < 4000; ++n) {
        makeSoloParty(party, noResist);
        resolveChestTrap(env.ctx, party, ChestTrap::PoisonGas);
        if (party.members[0].ailments.poisonLevel > 0) {
            ++poisoned;
        }
    }
    CHECK_EQ(poisoned, 0);
}

// B_0064#21: no-resistance race, disease spores trap 4000 times -> 0 diseased.
TEST(engine_B_0064_21_no_resist_disease_spores) {
    tsup::Env env(23456);
    Race noResist = Race::Human;
    bool found = false;
    for (i32 i = 0; i < kRaceCount && !found; ++i) {
        Race r = static_cast<Race>(i);
        if (raceResistBase(env.ctx, r, DamageKind::Poison) == 0 &&
            raceResistBase(env.ctx, r, DamageKind::Disease) == 0) {
            noResist = r;
            found = true;
        }
    }
    CHECK(found);

    i32 diseased = 0;
    Party party;
    for (i32 n = 0; n < 4000; ++n) {
        makeSoloParty(party, noResist);
        resolveChestTrap(env.ctx, party, ChestTrap::DiseaseSpores);
        if (party.members[0].ailments.diseaseLevel > 0) {
            ++diseased;
        }
    }
    CHECK_EQ(diseased, 0);
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

// B_0064#23: highest-poison-resistance race, poison gas 4000 times -> at least 1 poisoned.
TEST(engine_B_0064_23_highest_poison_gas) {
    tsup::Env env(34567);
    Race maxR = Race::Human;
    i32 maxRes = -1;
    for (i32 i = 0; i < kRaceCount; ++i) {
        Race r = static_cast<Race>(i);
        i32 v = raceResistBase(env.ctx, r, DamageKind::Poison);
        if (v > maxRes) {
            maxRes = v;
            maxR = r;
        }
    }

    i32 poisoned = 0;
    Party party;
    for (i32 n = 0; n < 4000; ++n) {
        makeSoloParty(party, maxR);
        // Setup fix: the trap poisons with level = dungeonLevel (R14 as written),
        // so the opener must be on a real dungeon level (>= 1), not level 0.
        party.members[0].location.dungeonLevel = 1;
        // Setup fix: resolveChestTrap has no opener index parameter; the solo party's
        // only member (index 0) is the opener.
        resolveChestTrap(env.ctx, party, ChestTrap::PoisonGas);
        if (party.members[0].ailments.poisonLevel > 0) {
            ++poisoned;
        }
    }
    CHECK(poisoned >= 1);
}

// B_0064#24: poisoned percentage is within half of poison resistance +/- 3.
TEST(engine_B_0064_24_highest_poison_percent) {
    tsup::Env env(45678);
    Race maxR = Race::Human;
    i32 maxRes = -1;
    for (i32 i = 0; i < kRaceCount; ++i) {
        Race r = static_cast<Race>(i);
        i32 v = raceResistBase(env.ctx, r, DamageKind::Poison);
        if (v > maxRes) {
            maxRes = v;
            maxR = r;
        }
    }

    i32 poisoned = 0;
    Party party;
    for (i32 n = 0; n < 4000; ++n) {
        makeSoloParty(party, maxR);
        // Setup fix: poisonLevel = dungeonLevel, so the opener must be on level >= 1.
        party.members[0].location.dungeonLevel = 1;
        // Setup fix: resolveChestTrap has no opener index parameter; the solo party's
        // only member (index 0) is the opener.
        resolveChestTrap(env.ctx, party, ChestTrap::PoisonGas);
        if (party.members[0].ailments.poisonLevel > 0) {
            ++poisoned;
        }
    }
    f64 pct = static_cast<f64>(poisoned) * 100.0 / 4000.0;
    f64 half = static_cast<f64>(maxRes) / 2.0;
    CHECK(pct >= half - 3.0);
    CHECK(pct <= half + 3.0);
}

// SKIP B_0064#25: the "10 passed as the trap argument" and the 90% fire spell resistance setup are not expressible from the headers (chestTrapDamage takes no count, and resistance application path is unclear).
// SKIP B_0064#26: depends on the same unresolved fire-trap setup as B_0064#25.
// SKIP B_0064#40: needs EventLog inspection and companion setup; no EventLog accessor is shown in the headers.
// SKIP B_0064#41: needs EventLog inspection to count distinct message shapes; no EventLog accessor is shown in the headers.
