#include "test_support.h"
#include "mordor/spells1a.h"

#include <cmath>
#include <cstdlib>

#include "mordor/companions1a.h"
#include "mordor/companions2a.h"
#include "mordor/state_party.h"

using namespace mordor;

namespace {

struct MishapTally {
    i32 none = 0;  // draws with no mishap
    i32 v0 = 0;    // mishaps landing on member 0
    i32 v1 = 0;    // mishaps landing on member 1 (owner)
    i32 v2 = 0;    // mishaps landing on member 2
};

// Party of three, owner at index 1, 40000 breath mishap draws.
MishapTally tallyBreathMishaps() {
    tsup::Env env(2024);
    static Party party;
    party.size = 3;
    const i32 bodies = countBodies(3, 4);
    MishapTally t{};
    for (i32 i = 0; i < 40000; ++i) {
        MishapResult r = rollMishap(env.ctx, party, 1, MishapKind::Breath, 10, bodies);
        if (!r.happened) {
            t.none++;
        } else if (r.victimIndex == 0) {
            t.v0++;
        } else if (r.victimIndex == 1) {
            t.v1++;
        } else if (r.victimIndex == 2) {
            t.v2++;
        }
    }
    return t;
}

i32 expectedBreathRange(i32 attackerLevel, i32 targetLevel) {
    const f64 a = static_cast<f64>(attackerLevel + 1);
    const f64 v = (std::log(a) + a / kBreathSeedDivisor) / std::log(kBreathLogBase)
                  - static_cast<f64>(targetLevel) / 10.0 + 1.0;
    return static_cast<i32>(vbInt(v));
}

}  // namespace

// B_0064#51: fifth companion add gets no slot when all four are occupied.
TEST(engine_B_0064_51_fifth_companion_no_slot) {
    // R2: four fixed slots, 0 to 3. An empty slot has Ref = -1 (Character.cs:82).
    // A slot is occupied when it holds a companion, so set Ref to a creature index.
    CompanionSlots slots{};
    for (i32 i = 0; i < kCompanionSlotsPerCharacter; ++i) {
        slots[static_cast<u32>(i)].ref = i;
    }
    CHECK_EQ(firstFreeCompanionSlot(slots), -1);
}

// B_0064#52: party of three with four living companions has 7 bodies.
TEST(engine_B_0064_52_bodies_with_four_living_companions) {
    CompanionSlots slots{};
    for (i32 i = 0; i < 4; ++i) {
        Companion& c = slots[static_cast<u32>(i)];
        c.occupied = true;
        c.hits = 10;
        c.maxHits = 10;
        c.isDead = false;
    }
    CHECK_EQ(countBodies(3, livingCompanionCount(slots)), 7);
}

// B_0064#53: one of four companions at 0 hits is not counted; 6 bodies.
TEST(engine_B_0064_53_bodies_with_one_dead_companion) {
    CompanionSlots slots{};
    for (i32 i = 0; i < 4; ++i) {
        Companion& c = slots[static_cast<u32>(i)];
        c.occupied = true;
        c.hits = 10;
        c.maxHits = 10;
        c.isDead = false;
    }
    slots[0].hits = 0;
    CHECK_EQ(countBodies(3, livingCompanionCount(slots)), 6);
}

// B_0064#54: owner (index 1) is never the victim of a mishap.
TEST(engine_B_0064_54_owner_never_victim) {
    MishapTally t = tallyBreathMishaps();
    CHECK_EQ(t.v1, 0);
}

// B_0064#55: member 0 is chosen as victim at least once.
TEST(engine_B_0064_55_member0_chosen) {
    MishapTally t = tallyBreathMishaps();
    CHECK(t.v0 >= 1);
}

// B_0064#56: member 2 is chosen as victim at least once.
TEST(engine_B_0064_56_member2_chosen) {
    MishapTally t = tallyBreathMishaps();
    CHECK(t.v2 >= 1);
}

// B_0064#57: victim counts of members 0 and 2 differ by less than a fifth of member 0's count.
TEST(engine_B_0064_57_members_0_and_2_balanced) {
    MishapTally t = tallyBreathMishaps();
    CHECK(std::abs(t.v0 - t.v2) * 5 < t.v0);
}

// B_0064#58: no-mishap draws (no victim) are at least the mishaps on members 0 and 2 combined.
// Assumption: "owner-spared draws" is read as draws where no mishap happened.
TEST(engine_B_0064_58_no_mishap_at_least_victims_0_and_2) {
    MishapTally t = tallyBreathMishaps();
    CHECK(t.none >= t.v0 + t.v2);
}

// B_0064#59: breath range matches the original formula for all four pairs.
TEST(engine_B_0064_59_breath_range_formula) {
    CHECK_EQ(specialHitRange(10, 5), expectedBreathRange(10, 5));
    CHECK_EQ(specialHitRange(100, 30), expectedBreathRange(100, 30));
    CHECK_EQ(specialHitRange(1, 1), expectedBreathRange(1, 1));
    CHECK_EQ(specialHitRange(500, 200), expectedBreathRange(500, 200));
}

// B_0064#60: a higher target level yields a smaller range.
TEST(engine_B_0064_60_range_falls_with_target_level) {
    CHECK(specialHitRange(100, 200) < specialHitRange(100, 10));
}
