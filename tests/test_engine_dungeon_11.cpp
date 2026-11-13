#include "test_support.h"
#include "mordor/dungeon2a.h"

using namespace mordor;

// SKIP A_02#23: needs MdrLoader.LoadDungeon on the MDATA11 file; no loader function in the headers
// SKIP A_02#25: needs GameFiles.Check and loading of shareware dungeon, monster and item data; no equivalent in the headers
// SKIP A_02#95: asserts on decompiled MAIN.TXT and .BAS sources; not a unit-testable engine function
// SKIP A_03#13: needs the character record importer (offsets into a saved record); no equivalent in the headers
// SKIP A_03#31: needs the MDATA8 automap importer and SeenFlags; no equivalent in the headers
// SKIP A_03#33: needs the MDATA10 importer (room list, tallies, chest data); no equivalent in the headers
// SKIP A_03#46: needs DungeonState.Load reading the saves folder; no equivalent in the headers

TEST(engine_A_02_10_limited_teleporter_denied) {
    CHECK_EQ(isDestinationDenied(4, ReleaseKind::Limited), true);
    CHECK_EQ(isDestinationDenied(3, ReleaseKind::Limited), false);
    CHECK_EQ(isDestinationDenied(0, ReleaseKind::Limited), false);
}
