#include "test_support.h"
#include <cmath>
#include "mordor/companions1a.h"
#include "mordor/companions1b.h"
#include "mordor/companions2a.h"

using namespace mordor;

// Spit share is assumed to return a fraction (0..1); the expected values are percents.

TEST(engine_B_0064_78_spit_one_companion_share) {
    const f64 pct = spitShare(1) * 100.0;
    CHECK(std::fabs(pct - 50.0) <= 1.5);
}

TEST(engine_B_0064_79_spit_two_companions_share) {
    const f64 pct = spitShare(2) * 100.0;
    CHECK(std::fabs(pct - 66.7) <= 1.5);
}

TEST(engine_B_0064_80_spit_three_companions_share) {
    const f64 pct = spitShare(3) * 100.0;
    CHECK(std::fabs(pct - 75.0) <= 1.5);
}

TEST(engine_B_0064_81_dead_only_companion_spat_returns_none) {
    tsup::Env env(81);
    CompanionSlots attacker{};
    attacker[0].occupied = true;
    attacker[0].hits = 10;
    attacker[0].maxHits = 10;
    CompanionSlots victim{};
    victim[0].occupied = true;
    victim[0].hits = 0;
    victim[0].maxHits = 20;
    CHECK_EQ(drawSpitTarget(env.ctx, attacker, victim), -1);
}

TEST(engine_B_0064_82_fifth_companion_add_returns_minus_one) {
    tsup::Env env(82);
    CompanionSlots slots{};
    for (i32 i = 0; i < kCompanionSlotsPerCharacter; ++i) {
        slots[i].occupied = true;
        slots[i].hits = 10;
        slots[i].maxHits = 10;
    }
    Companion incoming{};
    incoming.occupied = true;
    incoming.hits = 10;
    incoming.maxHits = 10;
    CHECK_EQ(placeNewCompanion(env.ctx, slots, incoming), -1);
}

// (none: the test's expected values and setup are consistent with the gate and slot logic)

TEST(engine_B_0064_84_tie_carry_candidate_is_first_slot) {
    tsup::Env env(84);
    CompanionSlots slots{};
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].maxHits = 20;
    slots[2].occupied = true;
    slots[2].hits = -4;
    slots[2].maxHits = 20;
    Arr<i32, kCompanionSlots> bodySizes{};
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        bodySizes[i] = 1;
    }
    Inventory inv{};
    runCarryChoice(env.ctx, slots, inv, bodySizes, 2);
    CHECK_EQ(inv.carryIndex, 0);
}

// SKIP B_0064#85: needs EventLog message accessor, which is not in the headers.

TEST(engine_B_0064_86_tie_carry_leaves_slots_0_and_empties_rest) {
    tsup::Env env(86);
    CompanionSlots slots{};
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].maxHits = 20;
    slots[2].occupied = true;
    slots[2].hits = -4;
    slots[2].maxHits = 20;
    Arr<i32, kCompanionSlots> bodySizes{};
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        bodySizes[i] = 1;
    }
    Inventory inv{};
    runCarryChoice(env.ctx, slots, inv, bodySizes, 2);
    CHECK(slots[0].occupied);
    CHECK_EQ(slots[0].hits, 0);
    CHECK(!slots[1].occupied);
    CHECK(!slots[2].occupied);
    CHECK(!slots[3].occupied);
}

// SKIP B_0064#87: needs EventLog message count accessor, which is not in the headers.

