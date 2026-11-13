// NEED: spellDamage (C# Rules.SpellDamage: ctx, baseDmg, power, sl, resistPct), resistedBy (C# SpecialHits.ResistedBy), resistance-index names for Fire/Cold/Acid/Electrical
// Add the header declaring spellDamage to the includes below once it exists.
#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/companions1b.h"   // afterResistance(i32 damage, i32 resistPercent)

using namespace mordor;

// B_0061#165, #166, #167: Rules.SpellDamage with a zero-returning random source.
// FIXED: spellDamage is not declared in any header in the API index, and Env is not visible here (tsup::Env). These tests are disabled until the declaration exists.
/*
TEST(engine_rtt080_spell_damage_firebolt_25pct_resist) {
  Env env({0.0});
  CHECK_EQ(spellDamage(env.ctx, 10, 1, 15, 25), 20);
}

TEST(engine_rtt080_spell_damage_firebolt_35pct_resist) {
  Env env({0.0});
  CHECK_EQ(spellDamage(env.ctx, 10, 1, 15, 35), 17);
}

TEST(engine_rtt080_spell_damage_firebolt_no_resist) {
  Env env({0.0});
  CHECK_EQ(spellDamage(env.ctx, 10, 1, 15, 0), 26);
}
*/

// B_0064#69, #68: SpecialHits.ResistedBy.
// SKIP B_0064#69: our declarations have no resistedBy function or resistance-index enum (only SpecialAttackKind{Breath, AcidSpit, Electrocute}); expected "The Acid resistance." cannot be checked yet. See NEED.
// SKIP B_0064#68: same reason; expected "The Cold resistance." cannot be checked yet. See NEED.
