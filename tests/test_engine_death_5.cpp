#include "test_support.h"
#include "mordor/death1a.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

// SKIP B_0058#2: Party.RemoveTheDead has no equivalent in the headers (removePartyMember is a different operation)
// SKIP B_0058#3: inspects decompiled FRMCONTR.BAS source text, not a runtime function
// SKIP B_0058#4: inspects port CityWindow.axaml.cs source (UI code), not a runtime function
// SKIP B_0058#5: inspects port DungeonWindow.Mdi.cs source (UI code), not a runtime function
// SKIP B_0058#16: inspects port CityWindow.axaml.cs source (UI code), not a runtime function
// SKIP B_0059#86: inspects original MAINMOD2.BAS screen flow (needs UI), no equivalent function
// SKIP B_0059#87: inspects original MAINMOD2.BAS screen flow (needs UI), no equivalent function
// SKIP B_0059#88: inspects original MAINMOD2.BAS screen flow (needs UI), no equivalent function
// SKIP B_0059#91: inspects original MAINMOD2.BAS screen hosting (needs UI), no equivalent function

TEST(engine_B_0059_90_dead_and_alone_owed) {
    mordor::Party party;
    party.size = 1;
    party.members[0].deathState = mordor::DeathState::Dead;
    CHECK(mordor::deadScreenOwed(party.members[0], party));
}

TEST(engine_B_0059_90_alive_alone_not_owed) {
    mordor::Party party;
    party.size = 1;
    party.members[0].deathState = mordor::DeathState::Alive;
    CHECK(!mordor::deadScreenOwed(party.members[0], party));
}
