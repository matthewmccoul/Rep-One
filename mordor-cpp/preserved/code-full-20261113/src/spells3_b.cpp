// spells3_b.cpp: damage and healing resolution, effective spell level, and crest-granted spells.
// Implements SRD Part C rules R6 to R11 (c106, c107).
// Defines only: spellModifier2, spellDamage, healingModifier1, spellHealing,
// effectiveSpellLevel, crestGrantFor, castCrestSpell.
// damageModifier1 is defined elsewhere (other half of spells3); it is called here.

#include <cmath>

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/spells3.h"

namespace mordor {

// R6 legend (c107): SL is capped at 255.
// UNKNOWN: which SL the caller should pass for damage and heal (sheet $C$2 vs highest SL vs
// current guild SL; c74, c76, c106). This module caps whatever it is given.
i32 effectiveSpellLevel(i32 sl) {
    // CONFLICT: reference SpellDamage uses sl = Math.Max(1, sl) (Rules.cs:631); SRD R8 says SL 0
    // is UNKNOWN. Reference preferred: floor at 1. Upper bound 255 kept from the existing code.
    return clampv(sl, 1, 255);
}

// R6 step 2 (c107):
//   Modifier2 = Int(Rnd * (Modifier1 - ((SL - 1) / SL^2)) / 2) + 1
i32 spellModifier2(Ctx& ctx, double modifier1, i32 sl) {
    // UNKNOWN: behaviour at SL 0, where (SL-1)/SL^2 is undefined (R8, c107).
    // Chosen: the term is taken as 0 at SL 0 so no division by zero occurs.
    const double slD = static_cast<double>(sl);
    const double term = (sl == 0) ? 0.0 : (slD - 1.0) / (slD * slD);

    // UNKNOWN: range of Rnd (R6, c107). Uses VB Rnd via ctx.rng.
    const double rnd = ctx.rng.nextDouble();

    // RESOLVED-BY-REFERENCE: legend says Int(x) is truncation (c107). Reference SpellHeal
    // (Rules.cs:646) uses a C# (int) cast: `(int)(rng.NextDouble() * (mod1 - ((sl - 1) / (double)(sl * sl))) / 2) + 1`.
    // Truncation toward zero is used, not vbInt (floor). The two differ only for negative arguments.
    const i64 truncated = static_cast<i64>(rnd * (modifier1 - term) / 2.0);
    return static_cast<i32>(truncated) + 1;
}

// R6 (c107), in order:
//   1. Modifier1 from Base, SL and Power (damageModifier1).
//   2. Modifier2 from Rnd, Modifier1 and SL (spellModifier2).
//   3. Damage = Int(Modifier1 - Modifier2).
//   4. Damage = Damage * (1 - Resistance/100).
i32 spellDamage(Ctx& ctx, i32 base, i32 sl, i32 power, i32 targetResistance) {
    const i32 level = effectiveSpellLevel(sl);

    // Step 1
    const double m1 = damageModifier1(base, level, power);

    // Step 2
    const i32 m2 = spellModifier2(ctx, m1, level);

    // Step 3: Damage = Int(Modifier1 - Modifier2)
    const i64 damage = vbInt(m1 - static_cast<double>(m2));

    // Step 4: resistance scaling (R6 step 4). Reference Rules.SpellDamage: the resistance is clamped to
    // 0..100, the result rounds half to even (the running damage is an Integer variable, so the
    // multiplication rounds on the way back in) and is never below 0.
    const i32 resistance = targetResistance < 0 ? 0 : (targetResistance > 100 ? 100 : targetResistance);
    const double scaled = static_cast<double>(damage) * (1.0 - static_cast<double>(resistance) / 100.0);
    const i64 rounded = roundHalfEven(scaled);
    return static_cast<i32>(rounded < 0 ? 0 : rounded);
}

// R7 / "Real Healing Formula" (c107):
//   Modifier1 = Base + (Base * LN(SL) / LN(10)) + Int((SL / 10) * (Power-1))
// The Power term is the same as in the damage formula. The healing formula has no resistance term.
double healingModifier1(i32 base, i32 sl, i32 power) {
    const double b = static_cast<double>(base);

    // UNKNOWN: LN(0) is undefined (R8, c107). Chosen: log term is 0 when SL < 1.
    double logTerm = 0.0;
    if (sl >= 1) {
        logTerm = b * std::log(static_cast<double>(sl)) / std::log(10.0);
    }

    // Reference SpellHeal (Rules.cs:646) uses `+ (int)((sl / 10.0) * (power - 1))`, a C# cast
    // that truncates toward zero, so truncation is used here rather than vbInt (floor).
    // This term is outside the marker list; it is changed for consistency with the reference.
    const double powerTerm =
        static_cast<double>(static_cast<i64>((static_cast<double>(sl) / 10.0) * static_cast<double>(power - 1)));

    return b + logTerm + powerTerm;
}

// R7 (c107): steps 1 to 3 of R6 using the healing log term. No resistance step.
//   Healing = Int(Modifier1 - Modifier2)
i32 spellHealing(Ctx& ctx, i32 base, i32 sl, i32 power) {
    const i32 level = effectiveSpellLevel(sl);

    // Step 1
    const double m1 = healingModifier1(base, level, power);

    // Step 2
    const i32 m2 = spellModifier2(ctx, m1, level);

    // Step 3 (no resistance step for healing).
    return static_cast<i32>(vbInt(m1 - static_cast<double>(m2)));
}

// R10 table (c106). Crests not in the table grant no spell (Journeyman, Warrior, Ninja).
// CONFLICT: the Guild enum has no Journeyman entry. Guild::Nomad is assumed to be the
// Journeyman crest; it has no row here, so it grants no spell either way.
// Note: the SRD crest "Magi" is Guild::Mage.
CrestGrantRow crestGrantFor(Guild crest) {
    for (const CrestGrantRow& row : kCrestGrants) {
        if (row.crest == crest) {
            return row;
        }
    }
    return CrestGrantRow{crest, CrestSpell::None, 0};
}

// R10 (c106): each crest casts one spell at the caster's spell level, with the listed charges.
// R11 UNKNOWN: whether crest spells use a separate SL (c106). Chosen: the caster's current
// spell level field (Character::spellLevel).
CrestCastResult castCrestSpell(Ctx& ctx, const Character& caster, Guild crest) {
    CrestCastResult result{};

    const CrestGrantRow row = crestGrantFor(crest);
    if (row.spell == CrestSpell::None) {
        // Journeyman, Warrior, Ninja and any crest outside the table: no spell granted.
        return result;
    }

    result.cast = true;
    result.spell = row.spell;
    result.charges = row.charges;
    result.spellLevel = caster.spellLevel;

    ctx.say("spells3.crestSpellGranted", "Crest grants a spell", {}, {});
    return result;
}

}  // namespace mordor
