#include "test_support.h"

#include <cstddef>
#include <set>

#include "mordor/city1.h"
#include "mordor/city2b.h"
#include "mordor/city3a.h"

using namespace mordor;

// SKIP B_0063#51: no uncurse-check function in the headers (Uncursing.NotCursed answer not exposed).
// SKIP B_0063#63: PenBook stores only per-alignment counts by ref; no LevelFound is kept to check.
// SKIP B_0063#64: PenBook stores only per-alignment counts by ref; no LevelFound is kept to read.

TEST(engine_B_0063_56_deepest_stocked) {
    CHECK_EQ(kDeepestStocked, 14);
}

// Not emitted. The replacement would need the observed distinct count, which requires running the code.

TEST(engine_B_0063_60_store_floor15_or_deeper_count) {
    // Floor of a stocked item is taken from Item.level.
    tsup::Env env(20260922);
    i32 deepOnes = 0;
    for (i32 run = 0; run < 6000; ++run) {
        Vec<Item> store;
        replenishStore(env.ctx, store, 50, 15);
        for (const Item& it : store) {
            if (it.level >= 15) {
                ++deepOnes;
            }
        }
    }
    CHECK_EQ(deepOnes, 0);
}

TEST(engine_B_0063_61_store_deepest_floor) {
    // Floor of a stocked item is taken from Item.level.
    tsup::Env env(20260922);
    i32 deepest = -1;
    for (i32 run = 0; run < 6000; ++run) {
        Vec<Item> store;
        replenishStore(env.ctx, store, 50, 15);
        for (const Item& it : store) {
            if (it.level > deepest) {
                deepest = it.level;
            }
        }
    }
    CHECK_EQ(deepest, 14);
}

TEST(engine_B_0063_62_pen_distinct_creatures) {
    tsup::Env env(20260922);
    std::set<std::size_t> refs;
    for (i32 run = 0; run < 6000; ++run) {
        PenBook pens;
        restockPens(env.ctx, pens, 50, 15);
        for (std::size_t r = 0; r < pens.byRef.size(); ++r) {
            for (std::size_t k = 0; k < static_cast<std::size_t>(kPenAlignCount); ++k) {
                if (pens.byRef[r][k] > 0) {
                    refs.insert(r);
                    break;
                }
            }
        }
    }
    CHECK(static_cast<i32>(refs.size()) >= 100);
}

TEST(engine_B_0065_96_realign_good_to_neutral) {
    // Price 4 gold = 40000 Currency; fee 3 gold = 30000 Currency.
    CHECK_EQ(storeRealignFee(40000, ItemAlign::Good, ItemAlign::Neutral), 30000);
}

TEST(engine_B_0065_97_realign_good_to_evil) {
    // Price 4 gold = 40000 Currency; fee 6 gold = 60000 Currency.
    CHECK_EQ(storeRealignFee(40000, ItemAlign::Good, ItemAlign::Evil), 60000);
}
