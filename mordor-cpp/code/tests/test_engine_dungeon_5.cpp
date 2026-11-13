#include "test_support.h"
#include "mordor/dungeon1b.h"

using namespace mordor;

// SKIP B_0063#118: needs FloorTeleporter/FloorField member access to find random-destination teleporters; not in the headers.
// SKIP B_0063#119: needs a simulation that follows fixed teleporter destinations; no equivalent function.
// SKIP B_0063#120: needs counting of fixed destinations and teleporter bits across data; no equivalent function.
// SKIP B_0063#121: needs square flag access (water, quicksand, extinguisher, etc.); FloorField members not in the headers.
// SKIP B_0063#122: needs per-square water and quicksand flag access; FloorField members not in the headers.
// SKIP B_0063#123: needs chute and square-effect flag access; FloorField members not in the headers.

TEST(engine_B_0063_124_total_squares) {
    const GameData& data = tsup::data();
    i64 total = 0;
    for (i32 level = 1; level <= 15 && level <= static_cast<i32>(data.floors.size()); ++level) {
        // FIXED: floorDef/FloorDef do not exist in the API index; mapHeight takes (data, level).
        const i64 h = static_cast<i64>(mapHeight(data, level));
        total += static_cast<i64>(mapWidth(data, level)) * h;
    }
    // The reference asserts 13500 (15 levels of 30 x 30) with FullCheck: full game only, skipped on the 3-level
    // shareware data that ships here, where the total is 3 x 900 = 2700.
    if (!tsup::fullGameOnly("squares across the dungeon (13500)")) {
        CHECK_EQ(total, 13500);
    } else {
        CHECK_EQ(total, 2700);
    }
}

TEST(engine_B_0063_125_rock_percent) {
    const GameData& data = tsup::data();
    i64 total = 0;
    i64 rock = 0;
    for (i32 level = 1; level <= 15 && level <= static_cast<i32>(data.floors.size()); ++level) {
        const i32 w = mapWidth(data, level);
        const i32 h = mapHeight(data, level);
        for (i32 x = 0; x < w; ++x) {   // 0-based squares API
            for (i32 y = 0; y < h; ++y) {
                ++total;
                if (isRockSquare(data, level, x, y)) {
                    ++rock;
                }
            }
        }
    }
    CHECK(total > 0);
    const double pct = 100.0 * static_cast<double>(rock) / static_cast<double>(total);
    CHECK(pct >= 7.0 && pct <= 9.0);
}

// SKIP B_0063#126: needs a FloorTeleporter configured as random-destination; its fields are not in the headers.
// SKIP B_0063#127: needs a 200-jump simulation with splitTeleportedParty and a random-destination FloorTeleporter setup; fields not in the headers.
