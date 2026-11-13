#include "test_support.h"
#include "mordor/spells2.h"
#include "mordor/spells1a.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

// SKIP B_0065#46: Flamesheet spell row, its RNG script, and the aimed-group setup are not specified in the cases; the group-0 caption and target value depend on those.
// SKIP B_0065#47: Same as B_0065#46; the post-cast target mark depends on unspecified Flamesheet data and rolls.
// SKIP B_0065#48: Requires a Firebolt spell id from game data and two one-hit groups with a scripted RNG; neither is given in the cases, and the caption join format is not defined.
// SKIP B_0065#49: Requires the two-group Firebolt setup from B_0065#48; the resulting target-mark walk depends on the unspecified spell id and rolls.
// SKIP B_0065#50: Needs a cast that kills every group; the kill rolls and spell id are not given, so targetMark = -1 cannot be set up reliably.
// SKIP B_0065#51: Needs a no-target Firebolt cast over a dead first group; the spell id and rolls are not given, and the caption format is not defined.
// SKIP B_0065#52: Needs the no-target setup from B_0065#51; the walked target-mark groups depend on unspecified data.
// SKIP B_0065#53: Needs the dead-first-group setup from B_0065#51; the spell id and rolls are not given, and the caption format is not defined.
// SKIP B_0065#54: judgeSpellVictim returns VictimVerdict::MagicImmune, but the caption "Failed!" is produced by a caption path (emitGroupCaptions) whose text is not exposed in the headers; the expected value cannot be asserted directly.
// SKIP B_0065#73: applyFate picks victims with RNG and emits text, but the line format and the scripted draws for the Fate cast are not specified.

