#include "test_support.h"
#include "mordor/monsters1a.h"

// FIXED: `i32` lives in namespace mordor and is not visible unqualified in this file.
using mordor::i32;

// SKIP B_0062#8: needs MdrLoader.LoadMonsters on MDATA5; no loader in the headers to look up by duplicated id 307
// SKIP B_0062#9: needs MdrLoader.LoadDungeon on MDATA11.MDR; no loader in the headers
// SKIP B_0062#10: needs MdrLoader.LoadMonsters on MDATA5 to count LevelFound > floors; no loader in the headers
// SKIP B_0062#11: needs MdrLoader.LoadMonsters on MDATA5 to test Asmodeus's LevelFound; no loader in the headers
// SKIP B_0062#12: needs MdrLoader.LoadMonsters on MDATA5 to test Shadow Ninja's LevelFound; no loader in the headers
// SKIP B_0062#13: needs loaded floor squares (Floor.AreaAt, Floor.Has) across 15 floors; no Floor API in the headers
// SKIP B_0062#14: needs loaded floor squares (Floor.AreaAt, Floor.Has) across 15 floors; no Floor API in the headers
// SKIP B_0062#15: needs loaded floor squares (Floor.AreaAt, Floor.Has) across 15 floors; no Floor API in the headers
// SKIP B_0062#16: needs loaded floor squares (Floor.AreaAt, Floor.Has) across 15 floors; no Floor API in the headers

// B_0063#2: the reference draws AcidTarget with four companion slots occupied; the closest
// header function is rollAcidSlot(Ctx&), which takes no companion array, so the occupancy
// setup cannot be expressed here. The assertion checks that no draw returns slot 5.
TEST(engine_B_0063_2_acid_slot_never_five) {
    tsup::Env env(11);
    i32 fives = 0;
    for (i32 i = 0; i < 60000; ++i) {
        if (rollAcidSlot(env.ctx) == 5) {
            ++fives;
        }
    }
    CHECK_EQ(fives, 0);
}
