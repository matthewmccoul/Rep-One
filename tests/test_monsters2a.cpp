#include "test_support.h"
#include "mordor/monsters2a.h"

#include <cstring>
#include <vector>

using mordor::MonsterKind;
using mordor::f64;  // FIXED: f64 is declared in mordor, not in the global scope.

// UNRESOLVED: test_support.h was not provided, so the constructor signature of
// tsup::Env is unknown. The compiler rejected tsup::Env(std::vector<double>).
// Each Env below now takes a braced single-value list, Env({v}), which matches
// an initializer_list or vector parameter. If the compiler still rejects these
// lines, read the Env constructor in test_support.h and adjust the argument
// form to match. The roll values and test expectations are unchanged.

TEST(monsters2a_constants_match_srd) {
    CHECK_EQ(mordor::kMonsterClassCount, 18);
    CHECK_EQ(mordor::kLairNamedPercent, 75);
    CHECK_EQ(mordor::kAnyClassRange, 100);
    CHECK_EQ(mordor::kAnyClassThreshold, 2);
    CHECK_EQ(mordor::kNeutralLeadPeacedPercent, 40);
    CHECK_EQ(mordor::kRareChanceBelow, 10);
    CHECK_EQ(mordor::kMaxJoinOffersPerGroup, 4);
    CHECK_EQ(mordor::kCritK, 650);
    CHECK_EQ(mordor::kCritLo, 2);
    CHECK_EQ(mordor::kCritHi, 50);
    CHECK_EQ(mordor::kBackstabK, 500);
    CHECK_EQ(mordor::kBackstabLo, 4);
    CHECK_EQ(mordor::kBackstabHi, 75);
    CHECK_EQ(mordor::kAttemptsPerGroupCap, 5);
    CHECK_EQ(mordor::kRespawnPerEmptiedRoomSeconds, 5);
    CHECK_EQ(mordor::kAllElevenFlagsMask, 2047);
}

TEST(monster_kind_names_table_entries) {
    CHECK(std::strcmp(mordor::kMonsterKindNames[0], "Humanoid") == 0);
    CHECK(std::strcmp(mordor::kMonsterKindNames[6], "Dragon") == 0);
    CHECK(std::strcmp(mordor::kMonsterKindNames[17], "Indigini") == 0);
}

