// casting3.cpp: cost, level, gate and charge helpers from SRD part c09p3.
//
// NOT DEFINED HERE (CONFLICT): the API INDEX assigns these exact signatures to
// casting1.h / casting2.h, so they are defined by those modules:
//   - i32 spellLevelFromGuildLevel(i32 guildLevel)   (declared in casting1.h, casting2.h)
//   - void etherealPortalIntoRock(Ctx&, Party&, i32) (declared in casting2.h)
// This file calls spellLevelFromGuildLevel() but does not define it.

#include "mordor/casting3.h"
#include "mordor/math_util.h"
#include "mordor/attributes.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Spell cost products (R7, R10, R11)
// ---------------------------------------------------------------------------

// R7 engine: rated x u4 x rate x 1.9 / SL.
// UNKNOWN: the R7 legend says `rate` appears twice, but the written formula
// has it once. The written formula is used.
// UNKNOWN: spellLevel == 0 is not defined in the SRD. Returns 0 to avoid divide by zero.
double costProductEngine(i64 rated, i64 u4, i64 rate, i32 spellLevel) {
    if (spellLevel == 0) {
        return 0.0;
    }
    return static_cast<double>(rated) * static_cast<double>(u4) *
           static_cast<double>(rate) * kCostFactor /
           static_cast<double>(spellLevel);
}

// R7 spreadsheets (c72): C x Q x R x 1.9 / SL.
// Spreadsheets differ from the engine (R7). Implemented as written for the sheet path.
double costProductSheet(i64 initialSpellLevel, i64 guildMod, i64 ratio, i32 spellLevel) {
    if (spellLevel == 0) {
        return 0.0;  // UNKNOWN: SL zero guard, not in SRD
    }
    return static_cast<double>(initialSpellLevel) * static_cast<double>(guildMod) *
           static_cast<double>(ratio) * kCostFactor /
           static_cast<double>(spellLevel);
}

// R7 archive (c106): StartSpellLevel x SpellPointRatio x BaseMod^2 x 1.9 / SL.
double costProductArchive(i64 startSpellLevel, i64 spellPointRatio, i64 baseMod, i32 spellLevel) {
    if (spellLevel == 0) {
        return 0.0;  // UNKNOWN: SL zero guard, not in SRD
    }
    const double mod = static_cast<double>(baseMod);
    return static_cast<double>(startSpellLevel) * static_cast<double>(spellPointRatio) *
           (mod * mod) * kCostFactor / static_cast<double>(spellLevel);
}

// R7 C# (c91): spell.Level / characterSpellLevel x spell.Cost x guildMod x 1.9.
// UNKNOWN: U18, operand types. Real (non-integer) division is assumed.
double costProductCSharp(i32 spellLevel, i32 characterSpellLevel, i64 spellCost, i64 guildMod) {
    if (characterSpellLevel == 0) {
        return 0.0;  // UNKNOWN: zero divisor guard, not in SRD
    }
    return (static_cast<double>(spellLevel) / static_cast<double>(characterSpellLevel)) *
           static_cast<double>(spellCost) * static_cast<double>(guildMod) * kCostFactor;
}

// R10 published (c61, tests): Int(product - 0.0001).
// CONFLICT: c27 records that this formula gives -1 for a zero ratio. That quirk is
// kept as documented. The R10 resolution to 0 applies to the engine path below.
i64 costFromProductPublished(double product) {
    return vbInt(product - 0.0001);
}

// R10 engine (c18, c23): if product is a positive whole number, price -= 1.
// Resolved to 0 for zero or negative product (c23, c27, c61).
// UNKNOWN: whether Rules.cs guards the zero case (U10). The guard is applied here.
i64 costFromProductEngine(double product) {
    if (product <= 0.0) {
        return 0;
    }
    i64 price = vbInt(product);
    if (static_cast<double>(price) == product) {
        price -= 1;
    }
    return price;
}

// R8 engine: clamp the cost up to the minimum. (Engine rule.)
i64 costAtLeastMinimum(i64 cost, i64 minimum) {
    return cost < minimum ? minimum : cost;
}

// R8 archive (c106): minimum = StartSpellLevel x BaseMod.
i64 archiveCostMinimum(i64 startSpellLevel, i64 baseMod) {
    return startSpellLevel * baseMod;
}

// R11: engine ratio-0 cost is 0 (c27, c61). Sheet overrides (D = E = K = 1, R = 0)
// are not used. UNKNOWN: U36, the effect of the hardcoded values.
i64 setSanctuaryCost() {
    return 0;
}

// R9: Guild level equivalent = Spell level x 2 - 1 (c106).
// UNKNOWN: R9, whether K is a cost or a level.
i32 guildLevelEquivalent(i32 spellLevel) {
    return spellLevel * 2 - 1;
}

// ---------------------------------------------------------------------------
// Item and level-0 spell levels and charges (R4, R5, R6)
// ---------------------------------------------------------------------------

// R6 engine: a negative item level uses the caster's current guild SL.
// UNKNOWN: U32 and the meaning of a non-negative item level. The item level is used as the spell level.
i32 itemSpellLevel(i32 itemLevel, i32 casterGuildSpellLevel) {
    return itemLevel < 0 ? casterGuildSpellLevel : itemLevel;
}

// R5 (c6, engine, item level 0): free ? 1 : Spellbook.SpellLevelFor(actor, s).
i32 zeroLevelItemSpellLevel(bool isFree, i32 spellBookSpellLevel) {
    return isFree ? 1 : spellBookSpellLevel;
}

// R5 (c44, engine port, item level 0 heal): SpellLevel(guild.Level), minimum 1.
// CONFLICT: R5. Scope of c6 vs c44 for heals is UNKNOWN. The c44 rule is used for heals.
i32 portZeroLevelHealLevel(i32 guildLevel) {
    const i32 sl = spellLevelFromGuildLevel(guildLevel);
    return sl < 1 ? 1 : sl;
}

// R4 (c43, c44 formula): if itemLevel == 0 then who.CurSpellPoints -= Cost(spell, who).
// The c44 prose (no SP for level-0 items) is treated as wrong (R4).
void chargeLevelZeroItemCast(Ctx& ctx, Character& who, i32 itemLevel, i64 cost) {
    (void)ctx;
    if (itemLevel == 0) {
        // UNKNOWN: no clamp at zero is specified in the SRD.
        who.vitals.curSpellPoints -= static_cast<i32>(cost);
    }
}

// ---------------------------------------------------------------------------
// Refusal order and gates (R1, R2, R13 monster low-level)
// ---------------------------------------------------------------------------

// R1: CONFLICT (unresolved). Chosen: c31 (MAINMOD2.BAS:433, MULTIMOD.BAS:467-479).
// A refused spell leaves None. On an antimagic square it leaves Fight, or Defend when there was no spell.
// Alternative (c29, c6, c61): always Fight. Not used here.
CombatOrderKind refusedCastOrder(bool antimagicSquare, bool hadSpell) {
    if (!antimagicSquare) {
        return CombatOrderKind::None;
    }
    return hadSpell ? CombatOrderKind::Fight : CombatOrderKind::Defend;
}

// R2: CONFLICT (unresolved). Engine preferred (c1): level-0 item gates apply to buffers and level-0 items.
// The audit claim (c34) that buffers skip the gates is not used.
bool castGatesApplyToBuffer(i32 itemLevel) {
    return itemLevel == 0;
}

// R32: the cast gate rounds half-to-even (c1, c56). The list gate uses the exact product (not this function).
i64 castGateRound(double product) {
    return roundHalfEven(product);
}

// R13 engine (c14): VbInt(LevelFound - LevelFound / 3.0).
// CONFLICT: R13. The audit form B(2 x floor / 3) (c23) has undefined B (U4). The engine form is used.
i32 monsterLowLevelEngine(i32 levelFound) {
    return static_cast<i32>(vbInt(static_cast<double>(levelFound) -
                                  static_cast<double>(levelFound) / kLowLevelDivisor));
}

// ---------------------------------------------------------------------------
// Combat state and monsters present (R14, R15, R16)
// ---------------------------------------------------------------------------

// R14, R15 engine (c1, C.4):
//   Combat: requires monsters present. Above kPeacedCastLevelLimit, requires engaged.
//   Either: above kPeacedCastLevelLimit, requires engaged when monsters are present.
//   NonCombat: refused only when engaged.
// UNKNOWN: U6. Either with no monsters is allowed here.
// CONFLICT: R15. The archive (c107) rule for non-combat casts is not used.
bool castAllowedByCombatState(CastScope scope, i32 baseLevel, bool monstersPresent, bool engaged) {
    switch (scope) {
        case CastScope::Combat:
            if (!monstersPresent) {
                return false;
            }
            if (!engaged && baseLevel > kPeacedCastLevelLimit) {
                return false;
            }
            return true;
        case CastScope::Either:
            if (monstersPresent && !engaged && baseLevel > kPeacedCastLevelLimit) {
                return false;
            }
            return true;
        case CastScope::NonCombat:
            return !engaged;
    }
    return false;
}

// R16 engine (c1): monstersPresent = fn0EEF() > 0.
bool monstersPresentEngine(i32 monsterCount) {
    return monsterCount > 0;
}

// R16 port (c43): (combat not over) or (waiting count > 0).
// UNKNOWN: whether this is equivalent to the engine form.
bool monstersPresentPort(bool combatOver, i32 waitingCount) {
    return (!combatOver) || waitingCount > 0;
}

// R31: CONFLICT. Implemented as the original rule (c34): asks only when M5B33 > 0,
// which is effect codes 0 and 18. The port always asks per c34 (stated deviation).
bool castAsksWho(i32 effectCode) {
    return effectCode == 0 || effectCode == 18;
}

// R30: refused before casting when there is no free companion slot and the charm kills the monster (c6).
// UNKNOWN: whether the last slot can be filled without refusal (c56).
bool charmRefusedNoSlot(i32 freeCompanionSlots, bool killsMonster) {
    return killsMonster && freeCompanionSlots <= 0;
}

// ---------------------------------------------------------------------------
// Protection and binding (R17, R18, R29)
// ---------------------------------------------------------------------------

// R18: base 3 for Sorceror and Wizard (c84, c107).
// CONFLICT: sheet C values differ (Sorceror C = 3, Wizard C = 15, c74, c79). Other guilds: UNKNOWN, 0 used.
i32 protectionBaseForGuild(Guild guild) {
    if (guild == Guild::Sorcerer || guild == Guild::Wizard) {
        return kProtectionBaseSorWiz;
    }
    return 0;  // UNKNOWN: base for other guilds not given in this part
}

// R17: the original sets only Ailments at cast (c34). The port overwrites at cast time.
// CONFLICT: R17, stated as a port deviation to verify. The protect level is stored directly.
// UNKNOWN: U34, protection effect magnitude.
void applyProtectionOnCast(Ctx& ctx, Character& who, i32 protectLevel) {
    (void)ctx;
    who.ailments.protectLevel = protectLevel;
}

// R29 engine (c21): Life Domination (150) sets ForLife.
// CONFLICT: archive (c97) says 150 sets binding to 100 (kBindArchiveBinding). Not applied.
// UNKNOWN: effect values other than 150 have no defined effect in this part.
void applyBindEffect(Ctx& ctx, Companion& target, i32 effectValue) {
    (void)ctx;
    if (effectValue == kBindLifeDomination) {
        target.forLife = true;
    }
}

// ---------------------------------------------------------------------------
// Max spell points (R33)
// ---------------------------------------------------------------------------

// R33 engine (c21, c25, c61): MaxSP = (INT + WIS) x 5.
// Archive (c107) gives no formula, so the engine formula is used.
i32 maxSpellPoints(const Character& c) {
    const i32 intel = statWithMods(c, StatIndex::Int);
    const i32 wis = statWithMods(c, StatIndex::Wis);
    return (intel + wis) * kMaxSpellPointMultiplier;
}

}  // namespace mordor
