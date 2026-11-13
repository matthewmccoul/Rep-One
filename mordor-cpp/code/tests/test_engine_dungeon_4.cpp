#include "test_support.h"
#include "mordor/dungeon1b.h"
#include "mordor/dungeon2a.h"

using namespace mordor;

// B_0063#103: SKIP B_0063#103: needs the Passwall setup (party position and floor state) from the prior case, which is not specified; etherealPortalStep outcome depends on it.

// B_0063#107: SKIP B_0063#107: setPartyPosition's rock test reads the floor's rock flag, and the FloorState encoding of that flag is not defined in the headers.

// B_0063#111: SpellEffect.MaxDungeonLevel maps to the full-build level limit.
TEST(engine_B_0063_111_max_dungeon_level) {
    CHECK_EQ(kFullBuildLevelLimit, 15);
    CHECK_EQ(buildLevelLimit(ReleaseKind::Full), 15);
}

// B_0063#112: SKIP B_0063#112: headers do not define which i32 build value selects the shipped full build for dungeonLevelCount.

// B_0063#114: SKIP B_0063#114: FloorTeleporter fields (fixed/random destination, X2/Y2/Z2) are not in the headers shown, so destinations cannot be enumerated.

// B_0063#115: SKIP B_0063#115: FloorTeleporter fields are not in the headers shown; cannot classify fixed-destination teleporters.

// B_0063#116: SKIP B_0063#116: FloorTeleporter fields are not in the headers shown; cannot classify random-destination teleporters.

// B_0063#117: SKIP B_0063#117: FloorTeleporter fields are not in the headers shown; cannot tell which fixed destinations change level.
