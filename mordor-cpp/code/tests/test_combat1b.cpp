#include <cmath>
#include "mordor/spells1a.h"

#include "test_support.h"
#include "mordor/combat1b.h"

using namespace mordor;

namespace {

bool approxEq(f64 a, f64 b) {
    return std::fabs(a - b) < 1e-9;
}

// Living companion: occupied, not dead, has hits.
void makeLiving(Companion& c) {
    c.occupied = true;
    c.isDead = false;
    c.deathState = CompanionDeathState::Normal;
    c.hits = 10;
    c.maxHits = 10;
}

}  // namespace

// R34: with no living companions the character is always the target,
// and no draw is needed (short-circuit on alive > 0).
TEST(PickTarget_NoLivingCompanionsHitsCharacter) {
    tsup::Env env({0.9, 0.9});
    Party party{};
    Arr<Companion, kCompanionSlots> companions{};
    CHECK_EQ(pickMonsterTarget(env.ctx, party, companions), 0);
}

// R34: alive = 4, character share = 100/(4+1) = 20.
// Rnd 0.175 -> 17+1=18 < 20 -> character.
// Rnd 0.195 -> 19+1=20 not < 20 -> companion (any living slot is nonzero).
TEST(PickTarget_CharacterShareWithFourCompanions) {
    Party party{};
    Arr<Companion, kCompanionSlots> companions{};
    makeLiving(companions[0]);
    makeLiving(companions[1]);
    makeLiving(companions[2]);
    makeLiving(companions[3]);

    tsup::Env low({0.175, 0.175});
    CHECK_EQ(pickMonsterTarget(low.ctx, party, companions), 0);

    tsup::Env high({0.195, 0.195});
    CHECK(pickMonsterTarget(high.ctx, party, companions) != 0);
}

// R34 quirk: one living companion, but the pick always lands on an empty slot.
// Rnd 0.6: check 60+1=61 not < 50 -> companion branch; pick = Int(2.4)+1 = 3, empty.
// After 100 tries the character is hit.
TEST(PickTarget_FallsBackToCharacterAfterMisses) {
    Party party{};
    Arr<Companion, kCompanionSlots> companions{};
    makeLiving(companions[0]);
    tsup::Env env({0.6, 0.6});
    CHECK_EQ(pickMonsterTarget(env.ctx, party, companions), 0);
}

// R46 step 3: gold is split across the party; exact division only.
TEST(SplitFightGold_ExactDivision) {
    CHECK_EQ(splitFightGold(120000, 4), 30000);
    CHECK_EQ(splitFightGold(50000, 1), 50000);
}

