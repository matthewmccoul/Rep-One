#include "test_support.h"
#include "mordor/items4.h"

// i32 lives in namespace mordor; bring it into scope for this translation unit.
using mordor::i32;

// Helpers local to this translation unit.
static i32 items4_lvl(mordor::IdLevel v) {
    return static_cast<i32>(v);
}

// ---- Constants (SRD T4, T5, T2 and quirk notes) ----

TEST(items4_drop_level_bounds) {
    CHECK_EQ(mordor::kItemDropLevelMin, 1);
    CHECK_EQ(mordor::kItemDropLevelMax, 16);
}

TEST(items4_spell_point_gain_constants) {
    // T2: Scroll of Spells adds 50 SP; Tome of Spells adds 200 SP.
    CHECK_EQ(mordor::kScrollOfSpellsGain, 50);
    CHECK_EQ(mordor::kTomeOfSpellsGain, 200);
}

TEST(items4_identify_constants) {
    // T5: dungeon levels 1-5 use the MOSTLY row; gap of 20 gives BARELY.
    CHECK_EQ(mordor::kIdentifyShallowMaxLevel, 5);
    CHECK_EQ(mordor::kIdentifyBarelyGap, 20);
}

TEST(items4_sentinel_constants) {
    CHECK_EQ(mordor::kNoGuildUse, -1);
    CHECK_EQ(mordor::kDropCategoryRandom, -1);
}

// ---- itemDropMinLevel (T4 author's min-level list) ----

// ---- identifyLevelFor (T5) ----

// ---- capIdentifyLevel (T6: only the General Store reaches ALL) ----

// ---- addSpellPointsFromItem (T2 consumables) ----

