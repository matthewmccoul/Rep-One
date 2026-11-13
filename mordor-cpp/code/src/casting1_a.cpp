// src/casting1_a.cpp
// casting1 part A: cast gate and refusal order, SP and charge spending,
// spell level, reach, engine and published cost, archive/sheet/charm formulas,
// spell-point recalculation, and monster spell selection.
//
// UNKNOWN (global): game_data.h field names are not given in the SRD part or the
// foundation text. This file assumes:
//   SpellDef { level, effect, damage2, category, costRatio, required (Vec<i32>), name }
//   GuildDef { spellRate, spellMax } indexed by school 0..kSpellSchoolCount-1
//   ctx.data.guilds and ctx.data.spells indexed by guild / spell index
// Adjust to the generated game_data.h if these differ.
//
// Helper functions are deliberately avoided (only the declared functions are
// defined here); repeated logic is written inline.

#include <cmath>

#include "mordor/attributes.h"
#include "mordor/casting1.h"
#include "mordor/ref_fn1485.h"
#include "mordor/casting2.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {
// UNKNOWN: raw Effect ids for Teleport and Cure Paralysis are not in this SRD part.
// Placeholder values; replace with the values from the spell table.
constexpr i32 kRawEffectTeleport = -1;
constexpr i32 kRawEffectCureParalysis = -2;

// FIXED: SpellDef has no member named costRatio in game_data.h (compiler error at
// spellCostEngine). The real field name was not available, so the cost ratio is
// a placeholder. Replace with the correct SpellDef member once confirmed.
constexpr i32 kPlaceholderCostRatio = 1;
}  // namespace

// ---------------------------------------------------------------------------
// Spell use classification (R1, R2)
// ---------------------------------------------------------------------------

SpellUse spellUse(const SpellDef& spell) {
    // R2.1: raw Effect is Teleport or Cure Paralysis -> Either.
    // Raw field only: the whole-party variant (+200) does not match [c1].
    if (spell.effect == kRawEffectTeleport || spell.effect == kRawEffectCureParalysis) {
        return SpellUse::Either;
    }
    // R2.2: Damage2 == 0 or Category == 9 (Heal) -> NonCombat.
    if (spell.damage2 == 0 || spell.category == kCategoryHeal) {
        return SpellUse::NonCombat;
    }
    // R2.3: otherwise Combat.
    return SpellUse::Combat;
}

// ---------------------------------------------------------------------------
// Unfit gate (R3, R4)
// ---------------------------------------------------------------------------

CastRefusal unfitGateCheck(const Ctx& ctx, const Character& who, const SpellDef& spell) {
    // R4. Checks in order; the first failure refuses.
    const u32 school = static_cast<u32>(spell.category);
    bool knows = false;
    bool levelOk = false;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = who.guilds[static_cast<u32>(g)];
        const GuildDef& def = ctx.data.guilds[static_cast<u32>(g)];
        if (m.level <= 0) {
            continue;
        }
        const f32 rate = def.spellRate[school];
        if (rate <= 0.0f) {
            continue;
        }
        // knows: SpellMax[cat] >= spell.Level
        if (def.spellMax[school] >= spell.level) {
            knows = true;
        }
        // levelOk: round_even(spell.Level * rate) <= floor(g.Level/2 + 0.5)
        const i64 rated = roundHalfEven(static_cast<f64>(spell.level) * static_cast<f64>(rate));
        const i64 half = vbInt(static_cast<f64>(m.level) / 2.0 + 0.5);
        if (rated <= half) {
            levelOk = true;
        }
    }
    // Step 1: Known.
    if (!knows) {
        return CastRefusal::NotKnown;
    }
    // Step 2: Spell level.
    if (!levelOk) {
        return CastRefusal::SpellLevel;
    }
    // Step 3: Stats. StatWithMods(i) >= Required[i] for i in 0..5 and i < Required.Length.
    const i32 reqCount = static_cast<i32>(spell.required.size());
    for (i32 i = 0; i < kStatCount && i < reqCount; ++i) {
        const i32 have = statWithMods(who, static_cast<StatIndex>(i));
        if (have < spell.required[static_cast<u32>(i)]) {
            return CastRefusal::Stat;
        }
    }
    return CastRefusal::None;
}

// ---------------------------------------------------------------------------
// Cast refusal order (R5, R6, R7, R8)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Cast roll and order decision (R12, R13)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Spending points and charges (R16, R18, R19)
// ---------------------------------------------------------------------------

void payCastCost(Character& caster, i32 cost) {
    // R16: points are taken (no clamp is stated in the SRD).
    caster.vitals.curSpellPoints -= cost;
}

i32 castCostSp(const Ctx& ctx, const Character& caster, const SpellDef& spell, i32 itemLevel) {
    // Cast gate [c43] and R18: cost = (itemLevel != 0) ? 0 : Spellbook.Cost(spell, who).
    if (itemLevel != 0) {
        return 0;
    }
    return spellCostEngine(ctx, caster, spell);
}

// ---------------------------------------------------------------------------
// Spell level (R21, R22)
// ---------------------------------------------------------------------------

i32 guildSpellLevel(i32 guildLevel) {
    // R21: SL = clamp(Int(guildLevel/2 + 0.5), 1, 255).
    i64 sl = vbInt(static_cast<f64>(guildLevel) / 2.0 + 0.5);
    if (sl < kSpellLevelMin) {
        sl = kSpellLevelMin;
    }
    if (sl > kSpellLevelMax) {
        sl = kSpellLevelMax;
    }
    return static_cast<i32>(sl);
}

// ---------------------------------------------------------------------------
// Learning and reach (R23, R24, R25, R26)
// ---------------------------------------------------------------------------

bool guildReachesSpell(i32 spellLevel, f32 rate, i32 guildLevel) {
    // R23: only a joined guild (Level > 0) with a rate above 0 for the class can reach a spell.
    // A zero rate means the guild does not teach this school, so it never reaches it.
    if (guildLevel <= 0 || rate <= 0.0f) {
        return false;
    }
    // R23 / R25: reach test on the exact product, no rounding.
    // listed if Level * rate <= guildLevel/2 + 0.5.
    const f64 product = static_cast<f64>(spellLevel) * static_cast<f64>(rate);
    const f64 half = static_cast<f64>(guildLevel) / 2.0 + 0.5;
    return product <= half;
}

bool spellWithinReach(const Ctx& ctx, const Character& caster, const SpellDef& spell) {
    // R23: some joined guild (Level > 0) with rate > 0, a ceiling that reaches the
    // spell level, and guildReachesSpell passes.
    const u32 school = static_cast<u32>(spell.category);
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = caster.guilds[static_cast<u32>(g)];
        if (!m.joined || m.level <= 0) {
            continue;
        }
        const GuildDef& def = ctx.data.guilds[static_cast<u32>(g)];
        const f32 rate = def.spellRate[school];
        if (rate <= 0.0f) {
            continue;
        }
        if (def.spellMax[school] < spell.level) {
            continue;
        }
        if (guildReachesSpell(spell.level, rate, m.level)) {
            return true;
        }
    }
    return false;
}

Vec<i32> listReachableSpellIds(const Ctx& ctx, const Character& caster) {
    // R23: ids of spells within reach. Id is the index in ctx.data.spells.
    // UNKNOWN: item-only spells (R67, other half) are not excluded here.
    Vec<i32> out;
    const i32 n = static_cast<i32>(ctx.data.spells.size());
    for (i32 i = 0; i < n; ++i) {
        if (spellWithinReach(ctx, caster, ctx.data.spells[static_cast<u32>(i)])) {
            out.push_back(i);
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Rated level and spell cost (R28, R29)
// ---------------------------------------------------------------------------

i32 spellCostEngine(const Ctx& ctx, const Character& caster, const SpellDef& spell) {
    // Reference Spells.Cost (Spellbook.Cost): over the guilds of the caster that have a spell table, a level above 0 and a
    // SpellMax ceiling that reaches the spell, the price is Fn1485Price(spell level, the spell's own u4 ratio, the guild's
    // spell rate, the guild's spell level); the cheapest accepted price wins and is raised to the smallest rated level
    // seen. No guild reaching the spell gives 0. (The earlier version used a placeholder ratio of 1 instead of u4.)
    i32 best = 0;
    i32 floorV = -1;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = caster.guilds[static_cast<u32>(g)];
        const GuildDef& def = ctx.data.guilds[static_cast<u32>(g)];
        if (!def.hasSpellTable || m.level <= 0) continue;
        if (spell.category < 0 || spell.category >= static_cast<i32>(def.spellMax.size())) continue;
        if (spell.level > def.spellMax[static_cast<u32>(spell.category)]) continue;
        const i32 sl = guildSpellLevel(m.level);
        i32 rated = 0;
        const i32 cost = fn1485Price(spell.level, spell.u4, def.spellRate[static_cast<u32>(spell.category)], sl, rated);
        if (cost > best && best != 0) continue;
        best = cost;
        if (rated < floorV || floorV == -1) floorV = rated;
        if (best < floorV) best = floorV;
    }
    return best;
}

// ---------------------------------------------------------------------------
// Archive formulas (R31, R32; not engine-verified)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Spreadsheet common core (R33)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Sorceror I and J (R34)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Charm of Opening spreadsheet (R35)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Archive availability and item level (R36, R37, R20; not engine-verified)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Spell points (recalc and restore)
// ---------------------------------------------------------------------------

i32 naturalMaxSpellPoints(const Abilities& abilities) {
    // UNKNOWN: the max-SP formula is the casting2 maxSpellPoints(naturalInt, naturalWis).
    // Natural Int and Wis are read from the base stats.
    return maxSpellPoints(abilities.stats[static_cast<u32>(StatIndex::Int)],
                          abilities.stats[static_cast<u32>(StatIndex::Wis)]);
}

void recalcSpellPoints(Character& who, bool refill) {
    // UNKNOWN: recalculation rule not in this SRD part. Assumed:
    //   maximum = naturalMaxSpellPoints; refill sets current to maximum;
    //   otherwise current is clamped down to maximum.
    const i32 mx = naturalMaxSpellPoints(who.abilities);
    who.vitals.maxSpellPoints = mx;
    if (refill) {
        who.vitals.curSpellPoints = mx;
    } else if (who.vitals.curSpellPoints > mx) {
        who.vitals.curSpellPoints = mx;
    }
}

// ---------------------------------------------------------------------------
// Monster spells (R49)
// ---------------------------------------------------------------------------

}  // namespace mordor
