// Batch t100: translations of CombatEngine.CastSpell assertions (B_0062, B_0065).
// Test names will be engine_rtt100_<what> once the cast entry point is declared.
// No cases are expressible with the current declarations, so each is skipped.
#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/casting1.h"
#include "mordor/combat3.h"

using namespace mordor;

// SKIP B_0062#57: needs a castSpell entry point with combat-state damage tracking across a per-spell loop of 12 casts on same-class monsters; no such declaration exists.
// SKIP B_0062#58: needs the same cast entry point and damage tracking on monsters of a different class; no such declaration exists.
// SKIP B_0065#80: needs castSpell returning LastCaptions for Charm Humanoid on a one-hit monster of matching class; no caption accessor or charm-specific monster state is declared.
// SKIP B_0065#83: needs castSpell returning LastCaptions for Charm Humanoid against a 30000-hit monster; no caption accessor or monster hit-count state is declared.
// SKIP B_0065#86: needs castSpell returning LastCaptions for Charm Humanoid against a wrong-class monster; no caption accessor or class-match state is declared.
// SKIP B_0065#41: needs castSpell returning LastCaptions for Firebolt against a three-monster group; no group state or caption accessor is declared.
// SKIP B_0065#42: needs castSpell returning LastCaptions and damage dealt to the front monster of a group; no group damage state is declared.
// SKIP B_0065#48: needs castSpell returning LastCaptions across two monster groups from group 0; no multi-group combat state is declared.
