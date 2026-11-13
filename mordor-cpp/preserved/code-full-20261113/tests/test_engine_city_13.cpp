#include "test_support.h"
#include "mordor/city1.h"

using namespace mordor;

// SKIP B_0067#14: needs MDATA3.MDR item-list loading and StoreStock.Replenish with a seeded RNG over 200 restocks; no equivalent in headers (replenishStore is a different signature and no cursed-count query exists).

TEST(engine_B_0068_17_row_visible_without_mod) {
    // No mod loaded: liveMods == 0 and no hidden rows recorded, so the row is visible.
    TownModAnswers mods{};
    CHECK_EQ(mods.liveMods, 0);
    CHECK(storeRowVisible(mods, 1));
}

// SKIP A_02#16: LimitedVersion.FullPrice has no equivalent function in the headers.

// SKIP A_02#17: LimitedVersion.Apply reads a folder and produces an About date and order sentence; no equivalent in headers.

// SKIP A_03#28: MDATA6 shelf-record import (item Id:alignment=count) has no equivalent function in the headers.
