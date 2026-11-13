// spells2.cpp: spell magnitude and range, sheet constants, targeting caps, damage and heal
// rolls, protection values, and non-damage spell effects.
// R-numbers refer to SRD Part B; c-numbers refer to the source chunk ids in Part A.
//
// BLOCKED: the 12 errors reported for this file are all "SpellDef has no member named ...".
// This file uses these SpellDef members, which game_data.h must declare:
//   type (compared to "Detrimental" / "Beneficial"), baseBlank (bool), base, power,
//   hpFlag (compared to "Y"), numM, numG, verbIndex (0-8).
// The member `effect` is not in the error list, so it is already declared.
// This file cannot supply these members: it has no data source for their values and must not
// invent them. Until game_data.h declares them, this file will not compile.
//
// NOTE: spells2.h uses SpellUse without including casting1.h, so casting1.h is included
// before spells2.h in this translation unit.
#include <cmath>

#include "mordor/casting1.h"
#include "mordor/spells2.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {
namespace {

// ASSUMED: the original spell-tool spreadsheet columns are not in the data file; they are derived from the fields SpellDef really has.
struct SpellSheet {
  Str type;
  bool baseBlank = false;
  i32 base = 0;
  i32 power = 0;
  Str hpFlag;
  i32 numM = 0;
  i32 numG = 0;
  i32 verbIndex = 0;
};
SpellSheet sheetOf(const SpellDef& s) {
  SpellSheet r;
  r.numM = s.affectMonster;
  r.numG = s.affectGroup;
  r.base = s.damage1;
  r.power = s.damage2;
  r.baseBlank = (s.damage1 == 0 && s.damage2 == 0);
  r.verbIndex = s.killEffect < 0 ? 0 : (s.killEffect > 8 ? 8 : s.killEffect);
  r.type = (s.damage1 > 0 || s.damage2 > 0) ? (s.effect == 0 ? "Detrimental" : "Beneficial") : "";
  return r;
}

// R16: charm casts take 4 targets in 1 group. CONFLICT: header kCharmCastPlan has 0/0; R16 values are used.
constexpr i32 kCharmTargets = 4;
constexpr i32 kCharmGroups = 1;
// R19: Sanctuary ages the caster "about one year". c21: current value is 365 days.
constexpr i32 kSanctuaryAgeDays = 365;
// UNKNOWN: "never set" sentinel for SanctuaryStore (R19: Sanctuary does nothing if never set).
constexpr i32 kSanctuaryUnsetLevel = -1;

// UNKNOWN: LN of a non-positive SL is undefined in the sheet; treated as 0 here.
double lnOf(double x) {
    return x > 0.0 ? std::log(x) : 0.0;
}

// ROUNDDOWN(x, 0): round toward zero (R4).
i64 truncToI64(double x) {
    return static_cast<i64>(std::trunc(x));
}

const SpellDef* spellAt(const Ctx& ctx, i32 index) {
    if (index < 0 || index >= static_cast<i32>(ctx.data.spells.size())) {
        return nullptr;
    }
    return &ctx.data.spells[static_cast<u32>(index)];
}

// R2 shape shared by DmgMod (LN 5) and HealMod (LN 10):
// G + G * LN(SL) / LN(base) + INT((SL / 10) * (H - 1))
double scaledMagnitude(double g, double h, double s, double logBase, double logShift = 0.0) {
    // Spreadsheet shape (ch10 R2): G + G*LN(SL)/LN(base) + Int((SL/10)*(H-1)). The engine's damage
    // formula (Rules.SpellDamage, ch10 R6) takes the log of SL+1: pass logShift = 1 for damage.
    return g + g * lnOf(s + logShift) / std::log(logBase)
           + static_cast<double>(vbInt((s / 10.0) * (h - 1.0)));
}

// Archive minimum (R10 and R13 share this shape):
// (Base + Base * LN(SL) / divisor + Int(SL * (Power - 1) / 10)) / 2
double archiveMinimum(i64 base, i64 power, i64 sl, double divisor) {
    const double g = static_cast<double>(base);
    const double s = static_cast<double>(sl);
    const double term1 = g * lnOf(s) / divisor;
    const double term2 = static_cast<double>(vbInt(s * (static_cast<double>(power) - 1.0) / 10.0));
    return (g + term1 + term2) / 2.0;
}

// Minimum + Random(1 to Minimum) (R10, R13).
// UNKNOWN: fractional Minimum. Random span is truncated, and the result is truncated to i64.
// The span is at least 1.
i64 rollAboveMinimum(Ctx& ctx, double minimum) {
    i64 span = vbInt(minimum);
    if (span < 1) {
        span = 1;
    }
    const i64 roll = static_cast<i64>(ctx.rng.below(static_cast<i32>(span))) + 1;
    return vbInt(minimum) + roll;
}

// UNKNOWN: map axis orientation is assumed (N = -y, E = +x, S = +y, W = -x).
// Right of facing is the clockwise neighbour (R25 / c25: right and forwards relative to facing).
struct FacingVectors {
    i32 fx;
    i32 fy;
    i32 rx;
    i32 ry;
};

FacingVectors facingVectors(Facing f) {
    switch (f) {
        case Facing::N:
            return FacingVectors{0, -1, 1, 0};
        case Facing::E:
            return FacingVectors{1, 0, 0, 1};
        case Facing::S:
            return FacingVectors{0, 1, -1, 0};
        case Facing::W:
        default:
            return FacingVectors{-1, 0, 0, -1};
    }
}

constexpr SpellVerb kVerbByIndex[9] = {
    SpellVerb::NONE,
    SpellVerb::FRIED,        // CONFLICT: test D maps verb index 1 to "Fried"; enum order has KILLED here.
    SpellVerb::KILLED,
    SpellVerb::FROZEN,
    SpellVerb::ELECTROCUTED,
    SpellVerb::CHOKED,
    SpellVerb::DISPELLED,
    SpellVerb::CHARMED,
    SpellVerb::SLEPT,
};

}  // namespace

// ---------------------------------------------------------------------------
// Lookup and magnitude (R1-R5)
// ---------------------------------------------------------------------------

// R1: exact-match lookup by name. Type "N/A" (anything other than Detrimental or Beneficial)
// becomes blank. Flag "Y" enables the HP note.
SpellRow spellRowByName(const Ctx& ctx, const Str& spellName) {
    SpellRow row{};
    row.index = -1;
    row.type = SpellEffectType::BLANK;
    row.baseBlank = true;
    row.base = 0;
    row.power = 0;
    row.hpFlag = false;
    const i32 count = static_cast<i32>(ctx.data.spells.size());
    for (i32 i = 0; i < count; ++i) {
        const SpellDef& s = ctx.data.spells[static_cast<u32>(i)];
        if (!(s.name == spellName)) {
            continue;
        }
        row.index = i;
        if (sheetOf(s).type == "Detrimental") {
            row.type = SpellEffectType::DETRIMENTAL;
        } else if (sheetOf(s).type == "Beneficial") {
            row.type = SpellEffectType::BENEFICIAL;
        }
        row.baseBlank = sheetOf(s).baseBlank;
        row.base = sheetOf(s).base;
        row.power = sheetOf(s).power;
        row.hpFlag = (sheetOf(s).hpFlag == "Y");
        break;  // VLOOKUP returns the first match.
    }
    return row;
}

// R2: DmgMod (I).
double damageMod(i64 base, i64 power, i64 sl) {
    // Engine damage (Rules.SpellDamage): the log term is LN(SL+1).
    return scaledMagnitude(static_cast<double>(base), static_cast<double>(power),
                           static_cast<double>(sl), kLnDamageBase, 1.0);
}

// R2: HealMod (J).
double healMod(i64 base, i64 power, i64 sl) {
    return scaledMagnitude(static_cast<double>(base), static_cast<double>(power),
                           static_cast<double>(sl), kLnHealBase);
}

// R3: blank when Base is blank. Quirk: the power term is 0 when H = 1 (c80).
MagnitudeResult spellMagnitude(const SpellRow& row, i64 sl) {
    MagnitudeResult r{};
    if (row.baseBlank) {
        r.blank = true;
        r.dmgMod = 0.0;
        r.healMod = 0.0;
        return r;
    }
    r.blank = false;
    r.dmgMod = damageMod(row.base, row.power, sl);
    r.healMod = healMod(row.base, row.power, sl);
    return r;
}

// R4: Min Dmg = ROUNDDOWN(I*0.5-1), Max Dmg = ROUNDDOWN(I*1-1), Detrimental only.
// Quirk: the source's M and O test L and N rather than F. Those tests are equivalent here.
ValueRange damageRange(SpellEffectType type, const MagnitudeResult& mag) {
    ValueRange v{false, 0, 0};
    if (mag.blank || type != SpellEffectType::DETRIMENTAL) {
        return v;
    }
    v.present = true;
    v.min = truncToI64(mag.dmgMod * 0.5 - 1.0);
    v.max = truncToI64(mag.dmgMod * 1.0 - 1.0);
    return v;
}

// R4: Min Heal = ROUNDDOWN(J*0.5-1), Max Heal = ROUNDDOWN(J*1-1), Beneficial only.
ValueRange healRange(SpellEffectType type, const MagnitudeResult& mag) {
    ValueRange v{false, 0, 0};
    if (mag.blank || type != SpellEffectType::BENEFICIAL) {
        return v;
    }
    v.present = true;
    v.min = truncToI64(mag.healMod * 0.5 - 1.0);
    v.max = truncToI64(mag.healMod * 1.0 - 1.0);
    return v;
}

// R5: the HP note shows when a damage range exists and the flag is "Y". The note is text only.
// Quirk: the flag check is also required (c74).
bool hpNoteShown(const ValueRange& dmgRange, const SpellRow& row) {
    return dmgRange.present && row.hpFlag;
}

// ---------------------------------------------------------------------------
// Sheet constants (R6, R7)
// ---------------------------------------------------------------------------

// R6: D = floor(Q*R*1.9 - 0.0001).
i64 sheetBaseValue(double q, double r) {
    return vbInt(q * r * kSheetFactor - kSheetEpsilon);
}

// R6: X = floor(C*Q*R*1.9/SL - 0.0001); E = C if X < C else X.
// UNKNOWN: SL <= 0 would divide by zero. The base C is returned in that case.
i64 sheetScaledBase(double c, double q, double r, i64 sl) {
    const i64 cInt = static_cast<i64>(c);
    if (sl <= 0) {
        return cInt;
    }
    const double x = c * q * r * kSheetFactor / static_cast<double>(sl) - kSheetEpsilon;
    const i64 X = vbInt(x);
    return (static_cast<double>(X) < c) ? cInt : X;
}

// R7: K = ROUNDUP(C*Q*R*1.9/(C+1), 0)*2 - 1. Its purpose is UNKNOWN.
i64 sheetColumnK(double c, double q, double r) {
    const double x = c * q * r * kSheetFactor / (c + 1.0);
    return static_cast<i64>(std::ceil(x)) * 2 - 1;
}

// ---------------------------------------------------------------------------
// Targeting and classification (R8, R9, R14)
// ---------------------------------------------------------------------------

// R8: monster cap (numM) and group cap (numG) for a combat spell.
// UNKNOWN: whether these caps apply to every combat spell is not stated here.
SpellTargetCaps spellTargetCaps(const Ctx& ctx, i32 spellIndex) {
    SpellTargetCaps caps{0, 0};
    const SpellDef* s = spellAt(ctx, spellIndex);
    if (s == nullptr) {
        return caps;
    }
    caps.numM = sheetOf(*s).numM;
    caps.numG = sheetOf(*s).numG;
    return caps;
}

// R8 and Part A table: effect ids 75-99 are class-filtered (charm and control).
// effect >= 200 is party-wide. Location and self spells are caster-only.
// UNKNOWN: pick-a-target heals and raises are mapped to CASTER_ONLY as a fallback.
SpellReach spellReach(const Ctx& ctx, i32 spellIndex) {
    const SpellDef* s = spellAt(ctx, spellIndex);
    if (s == nullptr) {
        return SpellReach::CASTER_ONLY;
    }
    const i32 e = s->effect;
    if (e >= 200) {
        return SpellReach::WHOLE_PARTY;
    }
    if (e >= 75 && e <= 99) {
        return SpellReach::CHARM_FILTER;
    }
    if (e == 0 || e == 100) {
        return SpellReach::COMBAT_MONSTERS;
    }
    return SpellReach::CASTER_ONLY;
}

// R9: effect verb. Kill (100) is KILLED and charm (80-99) is CHARMED. Otherwise the data verb
// index is used.
SpellVerb spellVerb(const Ctx& ctx, i32 spellIndex) {
    const SpellDef* s = spellAt(ctx, spellIndex);
    if (s == nullptr) {
        return SpellVerb::NONE;
    }
    if (s->effect == 100) {
        return SpellVerb::KILLED;
    }
    // CONFLICT: Part A puts charm at 80-99 while 75 is class-gated damage. Charm uses 80-99 here.
    if (s->effect >= 80 && s->effect <= 99) {
        return SpellVerb::CHARMED;
    }
    if (sheetOf(*s).verbIndex < 0 || sheetOf(*s).verbIndex > 8) {
        return SpellVerb::NONE;
    }
    return kVerbByIndex[sheetOf(*s).verbIndex];
}

// R14, R21: Heal family is NON_COMBAT except Cure Paralysis (COMBAT). Resist spells are NON_COMBAT.
// Damage is COMBAT when Detrimental. Charm and kill are COMBAT.
// UNKNOWN: the port's full cast-use rules are in T09 and not present here.
SpellUse spellUse(const Ctx& ctx, i32 spellIndex) {
    const SpellDef* s = spellAt(ctx, spellIndex);
    if (s == nullptr) {
        return SpellUse::NonCombat;
    }
    const i32 e = s->effect;
    if (e == 10) {
        return SpellUse::Combat;                 // R14 Cure Paralysis
    }
    if (e == 0) {
        return (sheetOf(*s).type == "Detrimental") ? SpellUse::Combat : SpellUse::NonCombat;
    }
    if (e == 1 || e == 2 || e == 18) {
        return SpellUse::NonCombat;              // R14 heal family
    }
    if (e >= 50 && e <= 74) {
        return SpellUse::NonCombat;              // R21 resist spells
    }
    if (e >= 75 && e <= 100) {
        return SpellUse::Combat;                 // charm, class-gated damage, kill
    }
    return SpellUse::NonCombat;
}

// ---------------------------------------------------------------------------
// Archive damage and heal (R10-R13)
// ---------------------------------------------------------------------------

// R10: Minimum = (Base + Base*LN(SL)/1.61 + Int(SL*(Power-1)/10)) / 2.
double archiveDamageMinimum(i64 base, i64 power, i64 sl) {
    return archiveMinimum(base, power, sl, kArchiveDamageDivisor);
}

// R10: Base Damage = Minimum + Random(1 to Minimum).
i64 archiveDamageRoll(Ctx& ctx, double minimum) {
    return rollAboveMinimum(ctx, minimum);
}

// R10: Final Damage = Base Damage * (1 - Resistance/100).
// The result is VB Integer, which rounds half to even (c61: 19.5 -> 20, 16.9 -> 17).
i64 resistedDamage(i64 damage, i32 resistancePct) {
    const double factor = 1.0 - static_cast<double>(resistancePct) / 100.0;
    return roundHalfEven(static_cast<double>(damage) * factor);
}

// R11: UNKNOWN which rule is the rule of record. Rule A (archive): damage >= monster HP kills,
// otherwise nothing happens. That is used here.
bool killSpellSucceeds(i64 rolledDamage, i64 monsterHits) {
    return rolledDamage >= monsterHits;
}

// R13: Minimum = (Base + Base*LN(SL)/2.3 + Int(SL*(Power-1)/10)) / 2.
double archiveHealMinimum(i64 base, i64 power, i64 sl) {
    return archiveMinimum(base, power, sl, kArchiveHealDivisor);
}

// R13: Healing = Minimum + Random(1 to Minimum).
i64 archiveHealRoll(Ctx& ctx, double minimum) {
    return rollAboveMinimum(ctx, minimum);
}

// ---------------------------------------------------------------------------
// Protection (R15)
// ---------------------------------------------------------------------------

// R15: item protection. Added Defense = 3 * LN(SL)^2.
// UNKNOWN: read as (LN SL)^2, as the text is written.
double itemProtectionDefence(i64 sl) {
    const double l = lnOf(static_cast<double>(sl));
    return 3.0 * l * l;
}

// R15: guild spell protection. Added Defense = 3 * LN(Int(SL/2) + 1)^2.
// The source calls this formula buggy (c106). It is reproduced as written.
double guildSpellProtectionDefence(i64 sl) {
    const double half = static_cast<double>(vbInt(static_cast<double>(sl) / 2.0));
    const double l = lnOf(half + 1.0);
    return 3.0 * l * l;
}

// ---------------------------------------------------------------------------
// Charm, bind, control (R16-R18)
// ---------------------------------------------------------------------------

// R16: charm up to kCharmTargets monsters in up to kCharmGroups groups, filtered by class.
// CONFLICT: class is read from the group (c27). The charm path under the 75-99 gate is UNKNOWN (c27, c62).
// UNKNOWN: companion storage is not part of this API, so charmed creatures are flagged in place.
// UNKNOWN: per-member state is not modelled. Only each group's front creature is charmed.
// UNKNOWN: no success roll is given in the SRD, so charm always succeeds when a match exists.
i32 charmMonsters(Ctx& ctx, Party& party, Encounter& enc, i32 monsterClassFilter) {
    if (party.size <= 0) {
        return 0;
    }
    i32 charmed = 0;
    i32 groupsTouched = 0;
    for (i32 g = 0; g < kMaxGroups && charmed < kCharmTargets && groupsTouched < kCharmGroups; ++g) {
        MonsterGroup& grp = enc.groups[static_cast<u32>(g)];
        Creature& front = grp.front;
        if (front.monsterClass != monsterClassFilter || !front.alive || front.charmed) {
            continue;
        }
        front.charmed = true;
        front.hostile = false;
        grp.state = GroupState::Peaced;
        ++charmed;
        ++groupsTouched;
        ctx.say("spell.charm.success", "You charmed One!", {}, {});
    }
    if (charmed == 0) {
        ctx.say("spell.charm.failed", "Failed", {}, {});
    }
    return charmed;
}

// R17: bind raises the companion's bind level. Bind does not work on companions who joined freely.
// The return value is true when the bind level rose. This overload is distinct from the other
// bindCompanion declarations in other headers.
bool bindCompanion(Ctx& ctx, Companion& companion, i32 bindLevelGain) {
    if (companion.joinedFreely) {
        ctx.say("spell.bind.refused", "Bind has no effect on this companion.", {}, {});
        return false;
    }
    const i32 before = companion.bindingLevel;
    i32 next = before + bindLevelGain;
    if (next > kBindingMax) {
        next = kBindingMax;
    }
    companion.bindingLevel = next;
    ctx.say("spell.bind.raised", "Bind level raised.", {}, {});
    return next > before;
}

// R18: failed Control Dragon. The targets "tend to respond adversely".
// UNKNOWN: the exact adverse effect is not specified. The group turns hostile.
void controlDragonFailed(Ctx& ctx, Encounter& enc, i32 groupIndex) {
    if (groupIndex < 0 || groupIndex >= kMaxGroups) {
        return;
    }
    MonsterGroup& grp = enc.groups[static_cast<u32>(groupIndex)];
    grp.state = GroupState::Hostile;
    grp.front.hostile = true;
    grp.front.charmed = false;
    ctx.say("spell.control.adverse", "The targets respond adversely.", {}, {});
}

// ---------------------------------------------------------------------------
// Sanctuary and movement (R19, R20)
// ---------------------------------------------------------------------------

// R19: Set Sanctuary stores the location.
void setSanctuary(Ctx& ctx, SanctuaryStore& store, const Location& at) {
    store.x = at.x;
    store.y = at.y;
    store.dungeonLevel = at.dungeonLevel;
    ctx.say("spell.sanctuary.set", "Sanctuary location set.", {}, {});
}

// R19: Sanctuary returns the party to the stored location. It does nothing if Set Sanctuary was never
// cast. The caster ages about one year.
// CONFLICT: manual says caster and party return (c83). Archive says the caster returns to that spot (c105).
// The manual is used (party moves).
bool castSanctuary(Ctx& ctx, Party& party, i32 casterIndex, const SanctuaryStore& store) {
    if (store.dungeonLevel == kSanctuaryUnsetLevel) {
        ctx.say("spell.sanctuary.none", "Nothing happens.", {}, {});
        return false;
    }
    if (casterIndex < 0 || casterIndex >= party.size) {
        return false;
    }
    party.x = store.x;
    party.y = store.y;
    party.dungeonLevel = store.dungeonLevel;
    Character& caster = party.members[static_cast<u32>(casterIndex)];
    caster.ageDays += kSanctuaryAgeDays;
    ctx.say("spell.sanctuary.cast", "Sanctuary returns the party.", {}, {});
    return true;
}

// R20: displacement moves the party by offsetRight to the right and offsetForward ahead of facing.
// Outside the dungeon, the whole party dies (R20). Rock detection needs map data that this module
// does not have, so ROCK_DEATH is never returned here.
// UNKNOWN: casterIndex is not needed for a party-wide move. Whether the caster alone moves is UNKNOWN.
// CONFLICT: whether a fall kills (c25 vs c31, c43, c63) is UNKNOWN. R20 is followed here.
DisplaceOutcome displaceWithin(Ctx& ctx, Party& party, i32 casterIndex, i32 offsetRight, i32 offsetForward) {
    static_cast<void>(casterIndex);
    if (!party.inDungeon) {
        for (i32 i = 0; i < party.size; ++i) {
            party.members[static_cast<u32>(i)].deathState = DeathState::Dead;
        }
        ctx.say("spell.displace.outside", "The party is destroyed.", {}, {});
        return DisplaceOutcome::OUTSIDE_DUNGEON;
    }
    const FacingVectors v = facingVectors(party.facing);
    party.x += offsetRight * v.rx + offsetForward * v.fx;
    party.y += offsetRight * v.ry + offsetForward * v.fy;
    ctx.say("spell.displace.moved", "The party is displaced.", {}, {});
    return DisplaceOutcome::MOVED;
}

// R20: Ethereal Portal moves two squares forward, through the wall in front.
// UNKNOWN: rock behind the wall (which would kill the party) cannot be detected in this module.
// The original hangs in mdrcharSetDead. The port stops instead (c31). This path does not hang.
DisplaceOutcome etherealPortal(Ctx& ctx, Party& party, i32 casterIndex) {
    return displaceWithin(ctx, party, casterIndex, 0, 2);
}

// R21: a Resist spell raises the matching resistance to 95.
// UNKNOWN: floor or cap or stacking is not stated. The value is raised only if it is below 95.
// UNKNOWN: the mapping from spell resist kind to the resistance slot is assumed to be identity.
void castResistSpell(Ctx& ctx, Resistances& resistances, SpellResistKind kind) {
    const i32 idx = static_cast<i32>(kind);
    if (idx < 0 || idx >= kResistKinds) {
        return;
    }
    i32& slot = resistances.resist[static_cast<u32>(idx)];
    if (slot < kResistGrantPercent) {
        slot = kResistGrantPercent;
    }
    ctx.say("spell.resist.granted", "Resistance raised.", {}, {});
}

}  // namespace mordor
