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

bool unfitGateApplies(bool fromBufferOrItem, i32 itemLevel) {
    // R3: runs only for buffers and for item spells whose item level is 0.
    // Any non-zero item level skips all three checks. Caster-cast spells skip it
    // because they are not fromBufferOrItem.
    return fromBufferOrItem && itemLevel == 0;
}

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

bool refusalClearsItemSlot(CastRefusal refusal) {
    // R5 table: ClearsItemSlot for NonCombat, AntiMagic, CombatOnly, TooLongPeaced only.
    switch (refusal) {
        case CastRefusal::NonCombat:
        case CastRefusal::AntiMagic:
        case CastRefusal::CombatOnly:
        case CastRefusal::TooLongPeaced:
            return true;
        default:
            return false;
    }
}

bool isPeaced(const CastSituation& situation) {
    // R7: monsters on the square but not engaged (not come for you).
    return situation.monstersPresent && !situation.engaged;
}

bool castGateMonsters(bool combatOver, i32 waitingCount) {
    // R8: CastGate monsters flag = (combat not over) or (waiting count > 0).
    return !combatOver || waitingCount > 0;
}

// ---------------------------------------------------------------------------
// Cast roll and order decision (R12, R13)
// ---------------------------------------------------------------------------

i32 drawCastRoll(Ctx& ctx) {
    // R12: l01D0 = Int(Rnd * 100). Drawn once per passing cast, before any effect.
    return static_cast<i32>(ctx.rng.below(kCastRollSides));
}

CastDisposition castDisposition(bool monstersPresent, bool fightOn, SpellUse use) {
    // R13: monsters present and (fight on or spell not NonCombat) -> BecomesOrder.
    if (monstersPresent && (fightOn || use != SpellUse::NonCombat)) {
        return CastDisposition::BecomesOrder;
    }
    return CastDisposition::CastNow;
}

void enforceStandingOrderSp(CombatOrder& order, i32 curSpellPoints, i32 cost) {
    // R15: a standing spell order whose caster lacks SP becomes Fight, keeping its target.
    // A cost-0 spell is always affordable.
    // UNKNOWN: the caller is assumed to pass only standing spell orders; the order
    // kind is not otherwise tagged as a spell order in CombatOrder.
    if (cost > 0 && cost > curSpellPoints) {
        order.kind = CombatOrderKind::Fight;
        // target is intentionally kept.
    }
}

// ---------------------------------------------------------------------------
// Spending points and charges (R16, R18, R19)
// ---------------------------------------------------------------------------

bool spendsCastPoints(bool targetChosen, bool killedByTeleport) {
    // R16: points taken only after target selection succeeds; a death by teleport spends nothing.
    return targetChosen && !killedByTeleport;
}

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

bool spendItemCharge(Item& item, bool castCompleted) {
    // R19: a charge is spent only on a completed cast, one per cast (MONSMOD.BAS:312-313).
    // UNKNOWN: the meaning of the return value (charge spent) is not given in this SRD part.
    if (!castCompleted) {
        return false;
    }
    item.charges -= 1;
    return true;
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

i32 casterSpellLevel(const Ctx& ctx, const Character& caster, const SpellDef& spell) {
    // R22: highest SL over joined guilds with SpellMax[cat] >= spell.Level.
    // RESOLVED-BY-REFERENCE: Spells.cs Cost comment: "The gate is the ceiling alone (Level <= SpellMax
    // and a level in the guild) ... exactly as in SpellLevelFor". The SRD's SpellRate[cat] > 0 test is
    // therefore not applied here.
    const u32 school = static_cast<u32>(spell.category);
    i32 best = 0;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = caster.guilds[static_cast<u32>(g)];
        if (!m.joined || m.level <= 0) {
            continue;
        }
        const GuildDef& def = ctx.data.guilds[static_cast<u32>(g)];
        if (def.spellMax[school] < spell.level) {
            continue;
        }
        const i32 sl = guildSpellLevel(m.level);
        if (sl > best) {
            best = sl;
        }
    }
    // R20 item 4: SpellLevelFor clamps to at least 1.
    // UNKNOWN: the empty-set result (no guild qualifies) is taken as the clamp minimum.
    return best > 0 ? best : kSpellLevelMin;
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

void announceNewlyReachable(Ctx& ctx, const Character& before, const Character& after) {
    // R24: reach computed before and after; newly reachable spells announced once,
    // with a header line.
    const Vec<i32> was = listReachableSpellIds(ctx, before);
    const Vec<i32> now = listReachableSpellIds(ctx, after);
    Vec<i32> fresh;
    for (i32 id : now) {
        bool seen = false;
        for (i32 old : was) {
            if (old == id) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            fresh.push_back(id);
        }
    }
    if (fresh.empty()) {
        return;
    }
    // UNKNOWN: header wording is not given in the SRD part.
    ctx.say("casting.reach.header", "New spells within reach:", {}, {});
    for (i32 id : fresh) {
        // UNKNOWN: spell display name field assumed to be SpellDef::name.
        const Str name = ctx.data.spells[static_cast<u32>(id)].name;
        ctx.say("casting.reach.spell", name, {id}, {name});
    }
}

i32 shownSpellLevel(i32 spellLevel, f32 rate) {
    // R25: spellLevelShown = floor(exact product).
    return static_cast<i32>(vbInt(static_cast<f64>(spellLevel) * static_cast<f64>(rate)));
}

i32 teachingGuild(const Ctx& ctx, const Character& caster, const SpellDef& spell) {
    // R26: among reaching guilds, the highest spell level wins; first wins on a tie.
    // Guild 0 is used if none reaches.
    const u32 school = static_cast<u32>(spell.category);
    i32 bestGuild = 0;
    i32 bestSl = -1;
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
        if (!guildReachesSpell(spell.level, rate, m.level)) {
            continue;
        }
        const i32 sl = guildSpellLevel(m.level);
        if (sl > bestSl) {
            bestSl = sl;
            bestGuild = g;
        }
    }
    return bestGuild;
}

// ---------------------------------------------------------------------------
// Rated level and spell cost (R28, R29)
// ---------------------------------------------------------------------------

i32 ratedSpellLevel(i32 spellLevel, f32 rate) {
    // R4 / R28: rated = Round_half_even(level * rate).
    return static_cast<i32>(roundHalfEven(static_cast<f64>(spellLevel) * static_cast<f64>(rate)));
}

i32 spellPriceForGuild(i32 spellLevel, f32 rate, i32 costRatio, i32 guildLevel) {
    // R28 per-guild price:
    //   SL = Int(GuildLevel/2 + .5) clamp 1..255
    //   r = (rate == 0 ? 1 : rate)
    //   rated = Round_half_even(level * r)
    //   price = Int((rated / SL) * u4 * r * 1.9)
    //   if exact product is a positive whole number: price -= 1
    const i64 sl = guildSpellLevel(guildLevel);
    const f64 r = (rate == 0.0f) ? 1.0 : static_cast<f64>(rate);
    const i64 rated = roundHalfEven(static_cast<f64>(spellLevel) * r);
    const f64 exact = (static_cast<f64>(rated) / static_cast<f64>(sl)) *
                      static_cast<f64>(costRatio) * r * 1.9;
    i64 price = vbInt(exact);
    // Port deviation (R29): floor the exact product, subtract one for whole products.
    if (exact > 0.0 && exact == static_cast<f64>(vbInt(exact))) {
        price -= 1;
    }
    return static_cast<i32>(price);
}

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

i32 spellCostPublished(i32 spellLevel, i32 costRatio, f32 rate, i32 guildLevel) {
    // R29 published test-page formula:
    //   cost = Int(rated/SL * ratio * rate * 1.9 - 0.0001)
    // Ratio 0 gives -1 here (engine gives 0; see R29 port deviation).
    const i64 sl = guildSpellLevel(guildLevel);
    const i32 rated = ratedSpellLevel(spellLevel, rate);
    const f64 v = (static_cast<f64>(rated) / static_cast<f64>(sl)) *
                      static_cast<f64>(costRatio) * static_cast<f64>(rate) * 1.9 -
                  0.0001;
    return static_cast<i32>(vbInt(v));
}

// ---------------------------------------------------------------------------
// Archive formulas (R31, R32; not engine-verified)
// ---------------------------------------------------------------------------

i32 archiveSpellCost(i32 startSpellLevel, f32 spellPointRatio, f32 baseMod, i32 sl) {
    // R31: floor((StartSpellLevel * SpellPointRatio * BaseMod^2 * 1.9 / SL) - 0.0001)
    // RESOLVED-BY-REFERENCE: Spells.cs Cost, fn1485 (MAINMOD2.BAS:2868-2903): l0484 = Level * rate is an
    // Integer (rounds half to even), then Int(((l0484 / SL) * U4) * rate * 1.9). No epsilon is applied.
    // The -0.0001 is the SRD's approximation (see ERRATUM ch09).
    if (sl == 0) {
        return 0;  // UNKNOWN: divide-by-zero guard; not in the SRD.
    }
    const f64 start = static_cast<f64>(startSpellLevel);
    const f64 ratio = static_cast<f64>(spellPointRatio);
    const f64 base = static_cast<f64>(baseMod);
    const f64 scaled = static_cast<f64>(roundHalfEven(start * base));
    const f64 v = (scaled / static_cast<f64>(sl)) * ratio * base * 1.9;
    return static_cast<i32>(vbInt(v));
}

i32 archiveMinimumCost(i32 startSpellLevel, f32 baseMod) {
    // R31: minimum cost = StartSpellLevel * BaseMod.
    // RESOLVED-BY-REFERENCE: Spells.cs Cost, fn1485: "l0484 = SpellDefs(s).Level * l0488 ... l0484 is an
    // Integer, so this rounds" (VB half-to-even); the minimum is floor = l0484.
    return static_cast<i32>(roundHalfEven(static_cast<f64>(startSpellLevel) * static_cast<f64>(baseMod)));
}

i32 archiveInitialCost(f32 spellPointRatio, f32 baseMod) {
    // R31: Initial Spell Cost = floor((SpellPointRatio * BaseMod * 1.9) - 0.0001)
    const f64 v = static_cast<f64>(spellPointRatio) * static_cast<f64>(baseMod) * 1.9 - 0.0001;
    return static_cast<i32>(vbInt(v));
}

i32 archiveLevelForDesiredCost(i32 startSpellLevel, f32 spellPointRatio, f32 baseMod,
                               i32 desiredCost) {
    // R31: ceil(StartSpellLevel * SpellPointRatio * BaseMod^2 * 1.9 / (DesiredCost + 1))
    if (desiredCost + 1 == 0) {
        return 0;  // UNKNOWN: divide-by-zero guard; not in the SRD.
    }
    const f64 start = static_cast<f64>(startSpellLevel);
    const f64 ratio = static_cast<f64>(spellPointRatio);
    const f64 base = static_cast<f64>(baseMod);
    const f64 v = start * ratio * base * base * 1.9 / static_cast<f64>(desiredCost + 1);
    i64 ceilled = vbInt(v);
    if (static_cast<f64>(ceilled) < v) {
        ceilled += 1;  // ceiling
    }
    return static_cast<i32>(ceilled);
}

i32 archiveGuildLevelEquivalent(i32 spellLevel) {
    // R31: Guild level equivalent = Spell level * 2 - 1.
    return spellLevel * 2 - 1;
}

// ---------------------------------------------------------------------------
// Spreadsheet common core (R33)
// ---------------------------------------------------------------------------

i32 sheetCommonInitialCost(f32 guildMod, f32 scRatio) {
    // R33 D = ROUNDDOWN(Q*R*1.9 - 0.0001, 0). ROUNDDOWN truncates toward zero.
    const f64 v = static_cast<f64>(guildMod) * static_cast<f64>(scRatio) * 1.9 - 0.0001;
    return static_cast<i32>(static_cast<i64>(v));
}

i32 sheetCommonCost(i32 initialLevel, f32 guildMod, f32 scRatio, i32 sl) {
    // R33 E = IF(ROUNDDOWN(C*Q*R*1.9/SL - 0.0001, 0) < C, C, ROUNDDOWN(...)).
    // Equivalent to MAX(ROUNDDOWN(...), C). ROUNDDOWN truncates toward zero.
    if (sl == 0) {
        return initialLevel;  // UNKNOWN: divide-by-zero guard; not in the SRD.
    }
    const f64 v = static_cast<f64>(initialLevel) * static_cast<f64>(guildMod) *
                      static_cast<f64>(scRatio) * 1.9 / static_cast<f64>(sl) -
                  0.0001;
    const i64 down = static_cast<i64>(v);
    return static_cast<i32>(down < initialLevel ? initialLevel : down);
}

i32 sheetCommonMinCost(i32 initialLevel, f32 guildMod, f32 scRatio) {
    // R33 K = ROUNDUP(C*Q*R*1.9/(C+1), 0) * 2 - 1. ROUNDUP rounds away from zero.
    if (initialLevel + 1 == 0) {
        return 0;  // UNKNOWN: divide-by-zero guard; not in the SRD.
    }
    const f64 v = static_cast<f64>(initialLevel) * static_cast<f64>(guildMod) *
                  static_cast<f64>(scRatio) * 1.9 / static_cast<f64>(initialLevel + 1);
    i64 up = static_cast<i64>(v);
    if (static_cast<f64>(up) != v) {
        up += (v > 0.0) ? 1 : -1;
    }
    return static_cast<i32>(up * 2 - 1);
}

// ---------------------------------------------------------------------------
// Sorceror I and J (R34)
// ---------------------------------------------------------------------------

f64 sorcererFormulaI(i32 base, i32 power, f64 c2) {
    // R34: I = G + G*LN(C2)/LN(5) + INT((C2/10)*(H-1))
    // INT truncates the linear term toward zero (SRD text).
    // UNKNOWN: blank G/H (both blank when G blank) are not representable in i32; computed as given.
    // UNKNOWN: LN of a non-positive C2 is undefined; 0 returned.
    if (c2 <= 0.0) {
        return 0.0;
    }
    const f64 g = static_cast<f64>(base);
    const f64 linear = static_cast<f64>(static_cast<i64>((c2 / 10.0) * static_cast<f64>(power - 1)));
    return g + g * std::log(c2) / std::log(5.0) + linear;
}

f64 sorcererFormulaJ(i32 base, i32 power, f64 c2) {
    // R34: J = G + G*LN(C2)/LN(10) + INT((C2/10)*(H-1))
    // Same UNKNOWN notes as sorcererFormulaI.
    if (c2 <= 0.0) {
        return 0.0;
    }
    const f64 g = static_cast<f64>(base);
    const f64 linear = static_cast<f64>(static_cast<i64>((c2 / 10.0) * static_cast<f64>(power - 1)));
    return g + g * std::log(c2) / std::log(10.0) + linear;
}

// ---------------------------------------------------------------------------
// Charm of Opening spreadsheet (R35)
// ---------------------------------------------------------------------------

i32 charmOpeningSheetCost(i32 slInit, f32 scRatio, f64 c2) {
    // R35: raw = floor(E4 * SL_init^2 * 1.9 / C2 - 0.0001)
    //      Spell Cost = SL_init if raw < SL_init else raw
    // SL_init is squared here (stated verbatim; difference unexplained in the SRD).
    if (c2 == 0.0) {
        return slInit;  // UNKNOWN: divide-by-zero guard; not in the SRD.
    }
    const f64 s = static_cast<f64>(slInit);
    const f64 v = static_cast<f64>(scRatio) * s * s * 1.9 / c2 - 0.0001;
    const i64 raw = vbInt(v);
    return static_cast<i32>(raw < slInit ? slInit : raw);
}

i32 charmOpeningInitialCost(i32 slInit, f32 scRatio) {
    // R35: SC_init = floor(E4 * SL_init * 1.9 - 0.0001)
    const f64 v = static_cast<f64>(scRatio) * static_cast<f64>(slInit) * 1.9 - 0.0001;
    return static_cast<i32>(vbInt(v));
}

i32 charmOpeningMinCost(i32 slInit, f32 scRatio) {
    // R35: Min Cost = ceil(E4 * SL_init^2 * 1.9 / (SL_init + 1)) * 2 - 1
    // Uses SL_init only, not the current SL. The guild-level input is not defined in the SRD.
    if (slInit + 1 == 0) {
        return 0;  // UNKNOWN: divide-by-zero guard; not in the SRD.
    }
    const f64 s = static_cast<f64>(slInit);
    const f64 v = static_cast<f64>(scRatio) * s * s * 1.9 / (s + 1.0);
    i64 up = vbInt(v);
    if (static_cast<f64>(up) < v) {
        up += 1;  // ceiling
    }
    return static_cast<i32>(up * 2 - 1);
}

// ---------------------------------------------------------------------------
// Archive availability and item level (R36, R37, R20; not engine-verified)
// ---------------------------------------------------------------------------

bool archiveSpellAvailable(const Ctx& ctx, const Character& caster, const SpellDef& spell) {
    // R36 (inferred, not engine-verified). All of:
    //   - stat requirements met (R4 step 3 path, statWithMods);
    //   - some joined guild holds the spell (rate > 0 and ceiling >= level, as a proxy);
    //   - that guild's SL reaches the spell's learning level.
    // UNKNOWN: per-guild learning levels (R38) are not in the supplied game data;
    // the base spell level is used as the learning level.
    const u32 school = static_cast<u32>(spell.category);
    const i32 reqCount = static_cast<i32>(spell.required.size());
    for (i32 i = 0; i < kStatCount && i < reqCount; ++i) {
        if (statWithMods(caster, static_cast<StatIndex>(i)) < spell.required[static_cast<u32>(i)]) {
            return false;
        }
    }
    // R37: the required level in only one of the guilds is enough.
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = caster.guilds[static_cast<u32>(g)];
        if (!m.joined || m.level <= 0) {
            continue;
        }
        const GuildDef& def = ctx.data.guilds[static_cast<u32>(g)];
        if (def.spellRate[school] <= 0.0f) {
            continue;
        }
        if (def.spellMax[school] < spell.level) {
            continue;
        }
        if (guildSpellLevel(m.level) >= spell.level) {
            return true;
        }
    }
    return false;
}

i32 archiveItemSpellLevel(const Character& who, i32 itemLevel) {
    // R20 applied to the archive. UNKNOWN: the archive rule is not in this SRD part.
    // Positive item level is used for the effect. Otherwise the active guild's spell level
    // (R20 item 2, with the minimum of 1 from R21).
    if (itemLevel > 0) {
        return itemLevel;
    }
    return guildSpellLevel(who.activeLevel());
}

// ---------------------------------------------------------------------------
// Spell points (recalc and restore)
// ---------------------------------------------------------------------------

void restoreSpellPointsInTown(Character& who) {
    // UNKNOWN: rule not in this SRD part. Assumed: restore current SP to maximum.
    who.vitals.curSpellPoints = who.vitals.maxSpellPoints;
}

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

MonsterSpellBand monsterSpellBand(i32 levelFound) {
    // UNKNOWN: band rule not in this SRD part. Assumed: low 1, top = levelFound (minimum 1).
    MonsterSpellBand band;
    band.low = 1;
    band.top = levelFound < 1 ? 1 : levelFound;
    return band;
}

bool hostileCastingSuppressed(const CastSituation& situation) {
    // R49.6 (per the CastSituation note): anti-magic or extinguisher at the position
    // suppresses hostile casting.
    // UNKNOWN: inferred from the header note on antiMagicSquare; R49.6 text not in this part.
    return situation.antiMagicSquare;
}

bool monsterSpellSkipped(Ctx& ctx, bool isMageClass) {
    // R49: flat skip check (kMonsterCastSkipPercent), then an extra skip check for
    // non-mage classes (kMonsterNonMageSkipPercent). Short-circuits on the first skip.
    if (ctx.rng.percent(kMonsterCastSkipPercent)) {
        return true;
    }
    if (!isMageClass && ctx.rng.percent(kMonsterNonMageSkipPercent)) {
        return true;
    }
    return false;
}

i32 pickMonsterSpellSchool(Ctx& ctx, u32 spellFlagMask) {
    // R49: school sampled with at most kMonsterSchoolAttemptCap attempts; kNoSchool if none.
    // UNKNOWN: the mask layout (bit n = school n) and the behaviour after the cap are not
    // in this part. An empty mask returns kNoSchool without drawing (UNKNOWN choice).
    if (spellFlagMask == 0u) {
        return kNoSchool;
    }
    for (i32 attempt = 0; attempt < kMonsterSchoolAttemptCap; ++attempt) {
        const i32 school = static_cast<i32>(ctx.rng.below(kSpellSchoolCount));
        if (((spellFlagMask >> static_cast<u32>(school)) & 1u) != 0u) {
            return school;
        }
    }
    return kNoSchool;
}

Vec<i32> monsterSpellPool(const Ctx& ctx, i32 school, MonsterSpellBand band) {
    // UNKNOWN: pool contents are not in this SRD part. Assumed: spells whose category
    // equals the school and whose level lies in [band.low, band.top]. Ids are indices.
    Vec<i32> pool;
    const i32 n = static_cast<i32>(ctx.data.spells.size());
    for (i32 i = 0; i < n; ++i) {
        const SpellDef& s = ctx.data.spells[static_cast<u32>(i)];
        if (s.category != school) {
            continue;
        }
        if (s.level < band.low || s.level > band.top) {
            continue;
        }
        pool.push_back(i);
    }
    return pool;
}

}  // namespace mordor
