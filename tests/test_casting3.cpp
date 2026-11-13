#include "test_support.h"
#include "mordor/casting2.h"
#include "mordor/casting3.h"

#include <cmath>

// Helpers (file scope, unique names to avoid clashes with the harness).

static bool c3_near(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

// Sets stats STR, INT, WIS and gives every stat a max of 18 so that no
// clamping against a zero max can alter the values under test.
static void c3_setStats(mordor::Character& c, int str, int intel, int wis) {
    for (unsigned k = 0; k < 6; ++k) {
        c.abilities.statMax[k] = 18;
    }
    c.abilities.stats[0] = str;   // StatIndex::Str
    c.abilities.stats[1] = intel; // StatIndex::Int
    c.abilities.stats[2] = wis;   // StatIndex::Wis
}

// ---- Spell cost: published and engine whole-number rules (R10) ----

// ---- Cost minimum clamp (R8, engine rule) ----

// ---- Archive cost product: StartSpellLevel * SpellPointRatio * BaseMod^2 * 1.9 / SL (R7) ----

TEST(casting3_cost_factor_constant) {
    CHECK(c3_near(mordor::kCostFactor, 1.9));
}

// ---- Spell level from guild level (R37: ceil(L/2) equivalent forms) ----

// ---- Guild level equivalent: Spell level * 2 - 1 (R9 stated formula) ----

// ---- Max spell points: (INT + WIS) * 5 (R33) ----

// ---- Monster low-level formula: VbInt(L - L/3.0) (R13 engine) ----

// ---- Cast gate rounding: half-to-even (R32) ----

// ---- Charm refused with no free companion slot (R30, c6 rule) ----

// ---- Combat state gating (R14, R15, engine rules) ----

// ---- Monsters present (engine count rule, R16) ----

// ---- Protection base for Sorceror and Wizard (R18, manual and archive) ----

// ---- Stat minimums for item-spell gating (R25, engine rule) ----

// ---- Item spell level (R6 engine: negative item level uses caster guild SL) ----

// ---- Level-0 item spell level (R5 / c6 engine, cast path) ----

// ---- Level-0 item SP charge (R4 formula: charge only when itemLevel == 0) ----

// ---- Life Domination bind effect of 150 sets ForLife (R29, engine) ----

// ---- Constants used by the cast rules ----

TEST(casting3_constants_match_source_values) {
    CHECK_EQ(mordor::kSpellLevelCap, 255);
    CHECK_EQ(mordor::kBindArchiveBinding, 100);
    CHECK_EQ(mordor::kFireboltBaseLevel, 1);
    CHECK_EQ(mordor::kBlueFlameBaseLevel, 3);
    CHECK_EQ(mordor::kCauseWoundsBaseLevel, 1);
}

// ---- Documented quirk: the published formula can drop a whole product by one ----

