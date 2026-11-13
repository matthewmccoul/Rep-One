// Batch t102: reference assertions from CombatEngine, CombatMonster and DungeonNav.
// None of these types have declarations in the shipped headers listed above, so every
// case is skipped until the declarations exist (see NEED below). No expected value was changed.
#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0065#46: CombatEngine.LastCaptionAims not declared; needs a Flamesheet fight setup and the caption-aim accessor.
// SKIP B_0065#49: CombatEngine.LastCaptionAims not declared; needs the two-group cast setup and the caption-aim accessor.
// SKIP B_0065#52: CombatEngine.LastCaptionAims not declared; needs the no-target cast setup and the caption-aim accessor.
// SKIP B_0065#47: CombatEngine.LastCastAim not declared; needs the Flamesheet cast setup and the cast-aim accessor.
// SKIP B_0065#50: CombatEngine.LastCastAim not declared; needs the two-group cast with no survivors and the cast-aim accessor.
// SKIP B_0065#43: CombatMonster.BySlot not declared; needs the CombatMonster type and the Runs/BySlot grouping rule with an empty slot 1.
// SKIP B_0063#106: DungeonNav.SetPosition not declared; the expected value is a relation (caster X/Y equal the rock square's X/Y), so it needs DungeonNav position accessors and a rock-square fixture.
// SKIP B_0063#101: DungeonNav.StepThrough not declared; needs a Floor with a rock square north of an open caster square, and the StepThrough return value.

// NEED: CombatEngine, CombatMonster, DungeonNav, Floor (with isRock/markExplored), LastCaptionAims, LastCastAim, BySlot, setPosition, stepThrough
