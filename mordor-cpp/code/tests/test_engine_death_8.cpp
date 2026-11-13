#include "test_support.h"
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/death1a.h"

using namespace mordor;

// SKIP B_0066#20: RoomRoster.Lines has no equivalent function in the headers.
// SKIP B_0066#21: RoomRoster.Lines has no equivalent function in the headers.
// SKIP B_0066#69: Party.RemoveTheDead has no equivalent; removePartyMember does not model the same messages/marking.
// SKIP B_0066#136: CombatEngine.MonsterAttacks has no equivalent in the headers; needs combat simulation.
// SKIP B_0066#137: Combat caption ordering needs CombatEngine simulation, no equivalent function.
// SKIP B_0066#138: Combat death message ordering needs CombatEngine simulation, no equivalent function.

TEST(engine_B_0066_132_new_character_area_is_unset) {
    Character ch;
    CHECK_EQ(ch.location.area, -1);
}

