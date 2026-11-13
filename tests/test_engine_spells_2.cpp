#include "test_support.h"
#include "mordor/spells1a.h"
#include "mordor/spells2.h"

using namespace mordor;

// Builds a damage spell row from base and power, then returns its magnitude range at spell level sl.
static ValueRange damageAt(i64 base, i64 power, i64 sl) {
    SpellRow row{0, SpellEffectType::DETRIMENTAL, false, base, power, false};
    MagnitudeResult mag = spellMagnitude(row, sl);
    return damageRange(SpellEffectType::DETRIMENTAL, mag);
}

TEST(engine_B_0061_51_cold_blast_max_damage) {
    struct Case { i64 sl; i64 expect; };
    const Case cases[] = {{15, 40}, {65, 59}, {115, 69}, {255, 90}};
    for (const Case& c : cases) {
        ValueRange r = damageAt(15, 2, c.sl);
        CHECK(r.present);
        CHECK_EQ(r.max, c.expect);
    }
}

TEST(engine_B_0061_52_acidic_spray_max_damage) {
    struct Case { i64 sl; i64 expect; };
    const Case cases[] = {{15, 95}, {65, 172}, {115, 232}, {255, 387}};
    for (const Case& c : cases) {
        ValueRange r = damageAt(30, 11, c.sl);
        CHECK(r.present);
        CHECK_EQ(r.max, c.expect);
    }
}

// SKIP B_0061#54: the cast-time Protection value is computed from the caster's spell level, and the
// new API does not state how a Wizard at guild level 26 maps to that spell level.

TEST(engine_B_0061_55_protection_recompute) {
    CHECK_EQ(protectValueOnRecompute(26), 14);
}

// SKIP B_0062#53: needs MDATA2 spell loading and iteration over spell definitions; no loader/spell-list
// function appears in the headers.
// SKIP B_0062#54: needs character creation and spellbook castables query; no equivalent in headers.
// SKIP B_0062#55: needs character creation, pack placement and castables query; no equivalent in headers.
// SKIP B_0062#56: needs loading spell definitions and enumerating class-targeted spells; no loader in headers.
// SKIP B_0062#57: needs full combat cast of each class-targeted spell against monster rosters; the
//                 spell list is not available through the headers.
// SKIP B_0062#58: needs full combat cast of each class-targeted spell against other-class monsters; the
//                 spell list is not available through the headers.
