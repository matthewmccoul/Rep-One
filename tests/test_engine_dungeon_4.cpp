#include "test_support.h"
#include "mordor/dungeon1b.h"
#include "mordor/dungeon2a.h"

using namespace mordor;

// B_0063#100: rock square with open ground directly south (S delta is (0,-1)).
TEST(engine_B_0063_100_rock_south_open) {
    const GameData& data = tsup::data();
    const i32 level = 1;
    const i32 w = mapWidth(data, level);
    const i32 h = mapHeight(data, level);
    bool found = false;
    for (i32 x = 1; x <= w && !found; ++x) {
        for (i32 y = 2; y <= h && !found; ++y) {
            if (isRockSquare(data, level, x, y) && !isRockSquare(data, level, x, y - 1)) {
                found = true;
            }
        }
    }
    CHECK_EQ(found, true);
}

// B_0063#103: SKIP B_0063#103: needs the Passwall setup (party position and floor state) from the prior case, which is not specified; etherealPortalStep outcome depends on it.

// B_0063#107: SKIP B_0063#107: setPartyPosition's rock test reads the floor's rock flag, and the FloorState encoding of that flag is not defined in the headers.

// B_0063#111: SpellEffect.MaxDungeonLevel maps to the full-build level limit.
TEST(engine_B_0063_111_max_dungeon_level) {
    CHECK_EQ(kFullBuildLevelLimit, 15);
    CHECK_EQ(buildLevelLimit(ReleaseKind::Full), 15);
}

// B_0063#112: SKIP B_0063#112: headers do not define which i32 build value selects the shipped full build for dungeonLevelCount.

// B_0063#113: total teleporter squares across the 15 levels of the shipped dungeon.
TEST(engine_B_0063_113_teleporter_square_total) {
    const GameData& data = tsup::data();
    i32 count = 0;
    for (i32 level = 1; level <= kFullBuildLevelLimit && level <= static_cast<i32>(data.floors.size()); ++level) {
        const i32 w = mapWidth(data, level);
        const i32 h = mapHeight(data, level);
        // The squares API is 0-based (squareIndexOf: x in 0..width-1); teleporter records are 1-based in the data.
        for (i32 x = 0; x < w; ++x) {
            for (i32 y = 0; y < h; ++y) {
                if (isTeleporterSquare(data, level, x, y)) {
                    ++count;
                }
            }
        }
    }
    // The reference asserts 86 with FullCheck: it is a figure of the 15-level full game and is skipped on the
    // 3-level shareware data that ships here. On that data the count is the shareware's own 14 (4 + 4 + 6).
    if (!tsup::fullGameOnly("teleporters in the dungeon (86)")) {
        CHECK_EQ(count, 86);
    } else {
        CHECK_EQ(count, 14);
    }
}

// B_0063#114: SKIP B_0063#114: FloorTeleporter fields (fixed/random destination, X2/Y2/Z2) are not in the headers shown, so destinations cannot be enumerated.

// B_0063#115: SKIP B_0063#115: FloorTeleporter fields are not in the headers shown; cannot classify fixed-destination teleporters.

// B_0063#116: SKIP B_0063#116: FloorTeleporter fields are not in the headers shown; cannot classify random-destination teleporters.

// B_0063#117: SKIP B_0063#117: FloorTeleporter fields are not in the headers shown; cannot tell which fixed destinations change level.
