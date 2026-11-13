#include "test_support.h"
#include "mordor/spells3.h"

// SKIP B_0061#32: MdrLoader.LoadSpells has no equivalent in the new headers (no data loader exposed)
// SKIP B_0061#39: SpellHelpProbe.Compare has no equivalent in the new headers (needs help-text comparison tooling)
// SKIP B_0061#40: SpellHelpProbe.Compare has no equivalent in the new headers (needs help-text comparison tooling)
// SKIP B_0061#41: MdrLoader.LoadSpells has no equivalent in the new headers (no data loader exposed)
// SKIP B_0061#45: MdrLoader.LoadSpells has no equivalent in the new headers (Base/Power not exposed as a lookup)
// SKIP B_0061#46: MdrLoader.LoadSpells has no equivalent in the new headers (Base/Power not exposed as a lookup)
// SKIP B_0061#47: MdrLoader.LoadSpells has no equivalent in the new headers (Base/Power not exposed as a lookup)
// SKIP B_0061#48: MdrLoader.LoadSpells has no equivalent in the new headers (Base/Power not exposed as a lookup)

TEST(engine_B_0061_49_firebolt_max_damage_by_level) {
    // Random source always returns zero: scripted with zeros.
    tsup::Env env1({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env1.ctx, 10, 15, 1, 0), 26);

    tsup::Env env2({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env2.ctx, 10, 65, 1, 0), 35);

    tsup::Env env3({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env3.ctx, 10, 115, 1, 0), 38);

    tsup::Env env4({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env4.ctx, 10, 255, 1, 0), 43);
}

TEST(engine_B_0061_50_blue_flame_max_damage_by_level) {
    // Random source always returns zero: scripted with zeros.
    tsup::Env env1({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env1.ctx, 15, 15, 11, 0), 54);

    tsup::Env env2({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env2.ctx, 15, 65, 11, 0), 118);

    tsup::Env env3({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env3.ctx, 15, 115, 11, 0), 173);

    tsup::Env env4({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    CHECK_EQ(mordor::spellDamage(env4.ctx, 15, 255, 11, 0), 320);
}
