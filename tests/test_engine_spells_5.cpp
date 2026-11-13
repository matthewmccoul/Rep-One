#include "test_support.h"
#include "mordor/spells1a.h"

// Case ids use '_' in place of '#' so they form valid C++ identifiers.

// SKIP B_0063#106: DungeonNav.SetPosition has no equivalent function in the headers.
TEST(engine_B_0063_108_teleport_arrives_south_of_rock) {
  CHECK(mordor::arrivalOutcome(1, false) == mordor::ArrivalOutcome::Arrives);
}

TEST(engine_B_0063_109_teleport_level0_falls_from_sky) {
  CHECK(mordor::arrivalOutcome(0, false) == mordor::ArrivalOutcome::FallsFromTheSky);
}

// SKIP B_0063#110: the deepest floor level is not exposed by any header function, so the destination level cannot be set up.
// SKIP B_0065#40: AffectGroup, Damage1 and AffectMonster are not exposed as spell row fields; spellTargetCaps does not map to them.
// SKIP B_0065#41: needs a Firebolt cast over a seeded encounter and EventLog caption access, which the headers do not show.
// SKIP B_0065#42: needs a cast with scripted damage rolls and EventLog caption access, which the headers do not show.
// SKIP B_0065#43: CombatMonster.BySlot has no equivalent function in the headers.
// SKIP B_0065#44: Flamesheet multi-group cast needs encounter setup and caption/hit readback that the headers do not expose.
// SKIP B_0065#45: needs a Firebolt cast over a seeded encounter and a survivor count, which no header function returns directly.
