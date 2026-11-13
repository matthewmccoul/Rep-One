// src/spells1a.cpp  -- module spells1a (SRD chapter 10, part c10p1)
//
// Global notes (grep for these tags):
//  UNKNOWN: GameData field names are not in the supplied material. This file assumes
//           camelCase SpellDef fields: level, pointCost, damage1, damage2, specialEffect,
//           affectGroup, resistIndex, ailments (effect id), category (7 = charm),
//           refusesModDungeon (map/raise/bind). ctx.data.spells is indexed by spell id.
//  UNKNOWN: CastRefusal enumerators are assumed: None, NotKnown, LevelTooLow,
//           NotEnoughPoints, AntiMagic, NoCharges, AlignedItem, MonstersOnSquare,
//           NotInParty, Dead, NeedsTarget, MapRaiseBind. spells1a.h uses CastRefusal
//           without including casting1.h, so this file includes casting1.h first.
//  CONFLICT: monsterBreathDamage(Ctx&, i32, i32) has the same signature in combat1b.h.
//           It is defined there and is deliberately NOT defined in this file.
//  FIXED: The compile errors show that SpellDef has no pointCost, ailments or
//         refusesModDungeon members, and CastRefusal has no LevelTooLow, NotEnoughPoints,
//         NoCharges, AlignedItem, MonstersOnSquare, NotInParty, Dead, NeedsTarget or
//         MapRaiseBind enumerators. Placeholders are used below (see the FIXED comments
//         at each site). They compile, but they CHANGE BEHAVIOUR until the real names are
//         supplied: every refusal becomes CastRefusal::NotKnown, point costs are 0,
//         effect ids are 0, and the map/raise/bind dungeon refusal is off.

#include <cmath>

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_party.h"
#include "mordor/state_dungeon.h"
#include "mordor/casting1.h"
#include "mordor/spells1a.h"

namespace mordor {
namespace {

constexpr i32 kGridSize = 30;               // R51 30x30 grid
constexpr i32 kLocalMaxDungeonLevel = 15;   // R53 MaxDungeonLevel
constexpr i32 kCharmCategory = 7;           // R23 charm is category 7
constexpr i32 kCharmEffectKind = 7;         // R17.2 / R17.4 kind-7 spell; UNKNOWN: effect-id mapping
constexpr i32 kAllOrNothingEffect = 100;    // R17.3
constexpr i32 kClassGateLow = 75;           // R18
constexpr i32 kClassGateHigh = 99;          // R18
constexpr i32 kGeneralCureEffect = 18;      // R26
constexpr i32 kResistEffectBase = 50;       // R31
constexpr i32 kResistGrant = 95;            // R31
constexpr i32 kBuffLevitate = 3;            // UNKNOWN: spell effect bit 3 (R28/R29)
constexpr i32 kBuffInvisible = 4;           // UNKNOWN: spell effect bit 4
constexpr i32 kBuffSeeInvisible = 5;        // UNKNOWN: spell effect bit 5
constexpr i32 kBuffProtect = 6;             // UNKNOWN: spell effect bit 6
constexpr i32 kBreathSweepGuildLimit = 40;  // R36
constexpr u32 kBitMagicImmune = 1u << 2;    // R19
constexpr u32 kBitCharmProof = 1u << 3;     // R17.2
constexpr u32 kBitBreathSweep = 1u << 3;    // R36 SpecialAttackFlags bit 3

const SpellDef& spellOf(const Ctx& ctx, i32 spellId) {
    return ctx.data.spells[spellId];  // UNKNOWN: spells indexed by spell id
}

// FIXED: SpellDef has no 'pointCost' member in the supplied headers.
// Placeholder returns 0 (no points spent). Replace with the real field.
i32 spellPointCost(const SpellDef& spell) {
    (void)spell;
    return 0;
}

// FIXED: SpellDef has no 'ailments' member in the supplied headers.
// Placeholder returns effect id 0. Replace with the real field.
i32 spellEffectId(const SpellDef& spell) {
    (void)spell;
    return 0;
}

// FIXED: SpellDef has no 'refusesModDungeon' member in the supplied headers.
// Placeholder returns false, so the map/raise/bind quirk is not applied. Replace with the real field.
bool spellRefusesModDungeon(const SpellDef& spell) {
    (void)spell;
    return false;
}

// R27: party-wide ids are 200 and up; the real effect is the id minus 200.
i32 effectReal(i32 effect) {
    return effect >= kPartyEffectBase ? effect - kPartyEffectBase : effect;
}

bool effectHas(const Character& c, i32 realEffect) {
    for (i32 e : c.activeEffectIds) {
        if (e == realEffect) return true;
    }
    return false;
}

bool knowsSpell(const Character& c, i32 spellId) {
    for (i32 s : c.knownSpellIds) {
        if (s == spellId) return true;
    }
    return false;
}

i32 wrapGrid(i32 v) { return ((v % kGridSize) + kGridSize) % kGridSize; }  // R51

i32 squareIndexOf(i32 x, i32 y) { return y * kGridSize + x; }

Location locationOf(const Party& p) {
    Location loc;
    loc.x = p.x;
    loc.y = p.y;
    loc.dungeonLevel = p.dungeonLevel;
    loc.facing = static_cast<i32>(p.facing);
    return loc;
}

void moveParty(Party& p, const Location& loc) {
    p.x = loc.x;
    p.y = loc.y;
    p.dungeonLevel = loc.dungeonLevel;
}

// UNKNOWN: the rock flag lives on SquareState::rockRecorded; squares of other
// levels are not reachable from this floor, so the current floor is used.
bool rockAt(const FloorState& floor, const Location& loc) {
    const i32 idx = squareIndexOf(loc.x, loc.y);
    if (idx < 0 || idx >= static_cast<i32>(floor.squares.size())) return false;
    return floor.squares[idx].rockRecorded;
}

// R13 / R14: aimed living group, else the first living group, else -1.
bool isLivingGroup(const Encounter& enc, i32 g) {
    return g >= 0 && g < kMaxGroups && enc.groups[g].livingCount > 0;
}

// R18: the class gate is read from the group aimed at (CONFLICT: c62 says per target).
i32 gateClassFor(const Encounter& enc, i32 aimedGroup, i32 start) {
    const i32 g = (aimedGroup >= 0 && aimedGroup < kMaxGroups) ? aimedGroup : start;
    if (g < 0 || g >= kMaxGroups) return -1;
    return enc.groups[g].front.monsterClass;
}

i32 findFreeCompanionSlot(const Arr<Companion, kCompanionSlots>& companions) {
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        if (!companions[i].occupied) return i;
    }
    return -1;
}

i32 countOccupied(const Arr<Companion, kCompanionSlots>& companions) {
    i32 n = 0;
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        if (companions[i].occupied) ++n;
    }
    return n;
}

// R37 shared shape: Int((Rnd*range+1)/2) + bonus, times 0.7, then resistance halved.
// UNKNOWN: rounding of the final value (floor used, as for the other special hits).
i32 specialDamageFromRange(Ctx& ctx, i32 range, f64 bonus, i32 resistPercent) {
    const f64 roll = static_cast<f64>(vbInt((ctx.rng.nextDouble() * range + 1.0) / 2.0));
    const f64 raw = (roll + bonus) * 0.7;
    const f64 resFactor = 1.0 - (static_cast<f64>(clampv(resistPercent, 0, 100)) / 2.0) / 100.0;
    return static_cast<i32>(vbInt(raw * resFactor));
}

// R15 / R16 / R17: one aimed or swept group. Rolls the front monster up to AffectGroup times.
void damageOneGroup(Ctx& ctx, Encounter& enc, const SpellCasterInfo& caster, const SpellDef& spell,
                    i32 effect, i32 slot, i32 gateClass, DamageSpellOutcome& out) {
    MonsterGroup& group = enc.groups[slot];
    for (i32 roll = 0; roll < spell.affectGroup && group.livingCount > 0; ++roll) {
        Creature& front = group.front;
        const i32 resist = victimResistPercent(front, spell.resistIndex);  // R16
        const i32 dmg = playerSpellDamage(ctx, spell.damage2, spell.specialEffect, caster.level, resist);
        const VictimVerdict verdict = judgeSpellVictim(front, effect, dmg, gateClass, caster.isCompanion);
        if (verdict == VictimVerdict::MagicImmune) {  // R17.1: caster's order becomes Fight
            out.casterOrderToFight = true;
            break;
        }
        if (verdict != VictimVerdict::Hit) break;      // R17.2-5: takes nothing; group ends
        out.victimsHit += 1;
        front.hits -= dmg;
        if (front.hits > 0) break;                     // R15: a surviving front ends the group
        front.alive = false;
        group.livingCount -= 1;
        if (group.livingCount > 0) {
            // UNKNOWN: the next member's hits are not in state; reloaded to maxHits.
            group.front.hits = group.front.maxHits;
            group.front.alive = true;
        }
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Pre-cast checks and targeting (R1-R5)
// ---------------------------------------------------------------------------

// R1: order: not known; level/stat too low; points; anti-magic; charges; aligned item.
CastRefusal precheckCast(Ctx& ctx, const Character& caster, i32 spellId, bool fromItem,
                         const Item& item, bool antiMagicArea) {
    const SpellDef& spell = spellOf(ctx, spellId);
    if (!fromItem && !knowsSpell(caster, spellId)) return CastRefusal::NotKnown;
    // UNKNOWN: the stat requirement is not in the supplied data; only the spell level is checked.
    // FIXED: CastRefusal::LevelTooLow does not exist; NotKnown used as the refusal.
    if (caster.spellLevel < spell.level) return CastRefusal::NotKnown;
    // UNKNOWN: item casts are charge-limited, not point-limited.
    // FIXED: CastRefusal::NotEnoughPoints does not exist; NotKnown used as the refusal.
    if (!fromItem && caster.vitals.curSpellPoints < spellPointCost(spell)) return CastRefusal::NotKnown;
    if (antiMagicArea) {
        ctx.say("spells.antimagic", "Anti-Magic area!", {}, {});
        return CastRefusal::AntiMagic;
    }
    // FIXED: CastRefusal::NoCharges does not exist; NotKnown used as the refusal.
    if (fromItem && item.charges <= 0) return CastRefusal::NotKnown;
    // FIXED: CastRefusal::AlignedItem does not exist; NotKnown used as the refusal.
    if (fromItem && item.aligned && item.alignment != ItemAlign::Unaligned &&
        (static_cast<i32>(item.alignment) - 1) != static_cast<i32>(caster.alignment)) {
        return CastRefusal::NotKnown;
    }
    return CastRefusal::None;
}

// R2: a mod's dungeon cast is refused, with nothing done, under these conditions.
CastRefusal checkModDungeonCast(Ctx& ctx, const Party& party, i32 casterIndex, i32 spellId,
                                bool monstersOnSquare, bool targetGiven) {
    // FIXED: the refusals below use CastRefusal::NotKnown, because MonstersOnSquare, NotInParty,
    // Dead, NeedsTarget and MapRaiseBind do not exist in CastRefusal.
    if (monstersOnSquare) return CastRefusal::NotKnown;
    if (casterIndex < 0 || casterIndex >= party.size) return CastRefusal::NotKnown;
    if (party.members[casterIndex].isDead()) return CastRefusal::NotKnown;
    if (targetPromptFor(ctx, spellId) != 0 && !targetGiven) return CastRefusal::NotKnown;
    // Quirk (R2): map, raise and bind spells are refused in a mod's dungeon cast.
    if (spellRefusesModDungeon(spellOf(ctx, spellId))) return CastRefusal::NotKnown;
    return CastRefusal::None;
}

// R3: 0 = asks nobody, 1 = people only, 2 = people and companions.
// CONFLICT: R3 offers companions; c36 says companions are never offered. R3 text is used.
i32 targetPromptFor(Ctx& ctx, i32 spellId) {
    const SpellDef& spell = spellOf(ctx, spellId);
    if (spellEffectId(spell) >= kPartyEffectBase) return 0;  // R3: party-wide asks nobody
    switch (routeSpell(ctx, spellId)) {
        case SpellRoute::Damage:
        case SpellRoute::Heal:
        case SpellRoute::Cure:
        case SpellRoute::Charm:
            return spell.damage2 > 0 ? 2 : 0;         // R3: damage or cure if Damage2 > 0
        case SpellRoute::Buff:
        case SpellRoute::Resist:
            return 1;                                 // R3: people only
        default:
            return 0;                                 // Fate and unhandled ask nobody
    }
}

// UNKNOWN: Table A (effect id -> route) is not supplied. Only the effects the SRD names
// are routed explicitly; everything else falls back to the damage/heal shape.
SpellRoute routeSpell(Ctx& ctx, i32 spellId) {
    const SpellDef& spell = spellOf(ctx, spellId);
    if (spellIsCharm(ctx, spellId)) return SpellRoute::Charm;                 // R23
    const i32 real = effectReal(spellEffectId(spell));                        // R27
    if (real == kGeneralCureEffect) return SpellRoute::Cure;                  // R26
    if (real >= kResistEffectBase && real < kResistEffectBase + kResistKinds) {
        return SpellRoute::Resist;                                            // R31
    }
    if (real >= kClassGateLow && real <= kAllOrNothingEffect) return SpellRoute::Damage;  // R17/R18
    if (spell.damage1 > 0) return SpellRoute::Damage;                         // R12
    if (spell.affectGroup == 0 && spell.damage2 > 0) return SpellRoute::Heal; // UNKNOWN: heal test (R33)
    if (real >= kBuffLevitate && real <= kBuffProtect) return SpellRoute::Buff;  // UNKNOWN ids
    return SpellRoute::Unhandled;
}

// R5: when engaged, the cast spell becomes the order. Item use is handled by the caller.
void makeCastTheOrder(Ctx& ctx, Character& caster, i32 spellId, bool engaged) {
    (void)ctx;
    if (!engaged) return;
    caster.order.kind = CombatOrderKind::Buffer;  // UNKNOWN: order kind used for a spell order
    caster.order.spellId = spellId;
}

// ---------------------------------------------------------------------------
// Damage (R6-R23)
// ---------------------------------------------------------------------------

// R7 / R8 / R9: player or companion spell damage against a foe.
i32 playerSpellDamage(Ctx& ctx, i32 base, i32 specialEffect, i32 spellLevel, i32 resistPercent) {
    const f64 L = static_cast<f64>(clampv(spellLevel, 1, 255));  // R8 clamp
    const f64 b = static_cast<f64>(base);
    const f64 e = static_cast<f64>(specialEffect);               // power - 1
    const f64 mod1 = b + b * std::log(L + 1.0) / std::log(5.0) +
                     static_cast<f64>(vbInt((L / 10.0) * e));
    const f64 cut = static_cast<f64>(vbInt(ctx.rng.nextDouble() * (mod1 - (L - 1.0) / (L * L)) / 2.0) + 1);
    const i64 dmg = vbInt(mod1 - cut);                           // R9: Integer running value
    const f64 res = static_cast<f64>(clampv(resistPercent, 0, 100)) / 100.0;
    const i64 out = roundHalfEven(static_cast<f64>(dmg) * (1.0 - res));
    return out < 0 ? 0 : static_cast<i32>(out);
}

// R10: monster spell damage. Resistance applies only when hasResist (ResistIndex > -1).
i32 monsterSpellDamage(Ctx& ctx, i32 monsterLevel, i32 damage2, i32 specialEffect,
                       bool hasResist, i32 resistPercent) {
    const f64 lvl = static_cast<f64>(monsterLevel < 1 ? 1 : monsterLevel);  // UNKNOWN: guard lvl >= 1
    const f64 l042C = std::log(lvl + 1.0) / std::log(5.0);
    const f64 d2 = static_cast<f64>(damage2);
    const f64 power = d2 + d2 * l042C + static_cast<f64>(vbInt(lvl / 10.0 * specialEffect));
    const i64 cut = vbInt(ctx.rng.nextDouble() * (power - (lvl - 1.0) / (lvl * lvl)) / 2.0) + 1;
    i64 dmg = vbInt(power - static_cast<f64>(cut));
    if (hasResist) {
        dmg = vbInt(static_cast<f64>(dmg) * (1.0 - static_cast<f64>(resistPercent) / 100.0));
    }
    if (dmg < 1) dmg = 0;
    return static_cast<i32>(dmg);
}

// R11: a monster spell replaces the turn's damage total; it does not add to it.
void setTurnDamageFromSpell(i32& turnDamageTotal, i32 spellDamage) {
    turnDamageTotal = spellDamage;  // Quirk: assignment, MONSMOD.BAS:2839
}

// R13: aimed living group, else first living group, else -1.
i32 spellStartGroup(const Encounter& enc, i32 aimedGroup) {
    if (isLivingGroup(enc, aimedGroup)) return aimedGroup;
    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (enc.groups[g].livingCount > 0) return g;
    }
    return -1;
}

// R21: a companion's spell resolves its opening group at cast time.
i32 companionOpeningGroup(const Encounter& enc) {
    return spellStartGroup(enc, -1);
}

// R14: a processed group advances both counters; an empty slot advances only the step.
void advanceDamageLoop(DamageLoopCounters& counters, bool groupProcessed) {
    if (groupProcessed) counters.groupCount += 1;
    counters.step += 1;
}

// R14: continue while groupCount < Damage1, within the four fixed slots.
bool damageLoopContinues(const DamageLoopCounters& counters, i32 damage1Groups) {
    return counters.groupCount < damage1Groups && counters.step < kDamageGroupSlots;
}

// R12-R22: damage spell against foes.
DamageSpellOutcome castDamageSpellOnFoes(Ctx& ctx, Encounter& enc, const SpellCasterInfo& caster,
                                         i32 spellId, i32 aimedGroup) {
    DamageSpellOutcome out;
    out.targetMark = aimedGroup;
    const SpellDef& spell = spellOf(ctx, spellId);
    const i32 effect = spellEffectId(spell);  // FIXED: was spell.ailments (no such member)

    if (companionCastBlockedByOwnImmunity(caster)) {  // R34: magic-immune companion's spell does nothing
        out.nothingDone = true;
        out.casterOrderToFight = true;
        out.itemOrderLost = true;
        emitGroupCaptions(ctx, out);
        return out;
    }

    const i32 start = spellStartGroup(enc, aimedGroup);       // R13
    const i32 gate = gateClassFor(enc, aimedGroup, start);    // R18
    DamageLoopCounters counters;
    while (damageLoopContinues(counters, spell.damage1)) {    // R14
        const i32 slot = counters.step;
        bool processed = false;
        if (slot >= start && isLivingGroup(enc, slot)) {
            processed = true;
            out.groupsReached += 1;                           // R20
            damageOneGroup(ctx, enc, caster, spell, effect, slot, gate, out);
            out.targetMark = slot;                            // R22
        }
        advanceDamageLoop(counters, processed);
    }

    out.nothingDone = out.victimsHit == 0;                    // R20
    if (out.nothingDone) {
        out.casterOrderToFight = true;
        out.itemOrderLost = true;
    }
    bool anyLiving = false;
    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (enc.groups[g].livingCount > 0) anyLiving = true;
    }
    out.targetMark = anyLiving ? aimedGroup : -1;             // R22: restored, or -1
    emitGroupCaptions(ctx, out);
    return out;
}

// R16: the resistance used is the group monster's, chosen by the resistance index.
// UNKNOWN: per-kind monster resistances are not in state; magicResistance is used.
i32 victimResistPercent(const Creature& victim, i32 resistIndex) {
    if (resistIndex < 0) return 0;
    return clampv(victim.magicResistance, 0, 100);
}

// R17 filters, in order, plus R19 (bit 2 = magic immune).
// casterIsCompanion is accepted for the caller's gate; no filter here reads it.
VictimVerdict judgeSpellVictim(const Creature& victim, i32 effectId, i32 damage, i32 gateClass,
                               bool casterIsCompanion) {
    (void)casterIsCompanion;
    if ((victim.specialPropertyFlags & kBitMagicImmune) != 0) return VictimVerdict::MagicImmune;  // R17.1
    if (effectId == kCharmEffectKind && (victim.specialPropertyFlags & kBitCharmProof) != 0) {
        return VictimVerdict::BlockedByCharmBit;                                                  // R17.2
    }
    const bool killed = damage >= victim.hits;
    if (effectId == kAllOrNothingEffect && !killed) return VictimVerdict::AllOrNothingMiss;       // R17.3
    if (effectId == kCharmEffectKind && !killed) return VictimVerdict::CharmSpared;               // R17.4
    if (!classGatePasses(effectId, gateClass)) return VictimVerdict::ClassGateBlocked;            // R17.5
    return VictimVerdict::Hit;
}

// R18: effects 75-99 pass only the matching class kind (effect 75 = kind 0).
bool classGatePasses(i32 effectId, i32 monsterClass) {
    if (effectId < kClassGateLow || effectId > kClassGateHigh) return true;
    return monsterClass == effectId - kClassGateLow;
}

// R20: one caption per group reached; a failed cast also says Failed!.
// UNKNOWN: caption text for a reached group is not given in the SRD.
void emitGroupCaptions(Ctx& ctx, const DamageSpellOutcome& outcome) {
    for (i32 g = 0; g < outcome.groupsReached; ++g) {
        ctx.say("spells.damage.group", "The spell reaches a group.", {static_cast<i64>(g)}, {});
    }
    if (outcome.nothingDone) {
        ctx.say("spells.failed", "Failed!", {}, {});
    }
}

// R23 / category 7: charm is a damage spell in the group loop.
// UNKNOWN: the category field name on SpellDef.
bool spellIsCharm(Ctx& ctx, i32 spellId) {
    return spellOf(ctx, spellId).category == kCharmCategory;
}

// ---------------------------------------------------------------------------
// Healing and cure (R24-R26)
// ---------------------------------------------------------------------------

// R24: heal roll. R25: minimum heal is 1.
i32 spellHealRoll(Ctx& ctx, i32 base, i32 specialEffect, i32 spellLevel) {
    const f64 L = static_cast<f64>(clampv(spellLevel, 1, 255));  // UNKNOWN: clamp as R8 for L >= 1
    const f64 b = static_cast<f64>(base);
    const f64 mod1 = b + b * std::log(L) / std::log(10.0) +
                     static_cast<f64>(vbInt((L / 10.0) * static_cast<f64>(specialEffect)));
    const i64 cut = vbInt(ctx.rng.nextDouble() * (mod1 - (L - 1.0) / (L * L)) / 2.0) + 1;
    const i64 heal = vbInt(mod1 - static_cast<f64>(cut));
    return heal < 1 ? 1 : static_cast<i32>(heal);
}

// R24 / R25: CurHits = min(MaxHits, CurHits + roll).
i32 healCharacter(Ctx& ctx, Vitals& vitals, i32 base, i32 specialEffect, i32 spellLevel) {
    const i32 heal = spellHealRoll(ctx, base, specialEffect, spellLevel);
    const i32 sum = vitals.curHits + heal;
    vitals.curHits = sum > vitals.maxHits ? vitals.maxHits : sum;
    return heal;
}

// R25: companion targets change Hits.
i32 healCompanion(Ctx& ctx, Companion& target, i32 base, i32 specialEffect, i32 spellLevel) {
    const i32 heal = spellHealRoll(ctx, base, specialEffect, spellLevel);
    const i32 sum = target.hits + heal;
    target.hits = sum > target.maxHits ? target.maxHits : sum;
    return heal;
}

// R26: general Cure also clears poison and disease.
void castGeneralCure(Ctx& ctx, Character& target) {
    target.ailments.poisonLevel = 0;
    target.ailments.diseaseLevel = 0;
    ctx.say("spells.cure", "Cured.", {}, {});
}

// R26: cleared without refusal.
void castCurePoison(Ctx& ctx, Character& target) {
    target.ailments.poisonLevel = 0;
    ctx.say("spells.cure.poison", "Poison cured.", {}, {});
}

// R26: cleared without refusal.
void castCureDisease(Ctx& ctx, Character& target) {
    target.ailments.diseaseLevel = 0;
    ctx.say("spells.cure.disease", "Disease cured.", {}, {});
}

// R26: Cure Paralysis cures only the caster, whoever was picked (quirk).
void castCureParalysis(Ctx& ctx, Character& caster) {
    if (caster.ailments.paralysisLevel > 0) {
        caster.ailments.paralysisLevel = 0;
        caster.ailments.paralysed = false;
        ctx.say("spells.cure.paralysis", "Paralysis cured.", {}, {});
    }
}

// ---------------------------------------------------------------------------
// Spell states and resistance (R28-R31)
// ---------------------------------------------------------------------------

// R28: cast-time state. A cast does not call recompute.
void applyCastState(Ctx& ctx, Character& target, i32 effectId, i32 casterSpellLevel) {
    (void)ctx;
    const i32 real = effectReal(effectId);
    if (!effectHas(target, real)) target.activeEffectIds.push_back(real);  // SpellEffects |= 1 << effect
    switch (real) {
        case kBuffLevitate:
            target.ailments.levitating = true;
            break;
        case kBuffInvisible:
            target.ailments.invisible = true;
            break;
        case kBuffSeeInvisible:
            target.ailments.seeInvisible = true;
            break;
        case kBuffProtect:
            target.ailments.protectLevel = protectValueAtCast(casterSpellLevel);  // MAINMOD2.BAS:1046
            break;
        default:
            break;
    }
}

// R28: Protection = Int(CasterSpellLevel / 2) + 1.
i32 protectValueAtCast(i32 casterSpellLevel) {
    return static_cast<i32>(vbInt(static_cast<f64>(casterSpellLevel) / 2.0)) + 1;
}

// R29: recompute the flags from the spell bits and the gear mask.
// Quirk: spell bit 3 (Levitate) also sets See Invisible, because the shared loop reads bit 3
// as See Invisible. Gear bit 3 (Boots of Levitation) grants only Levitate.
// UNKNOWN: gear meanings for bits 0..5 are not given; only the bit-3 gear case is modelled.
void recomputeSpellStates(Ctx& ctx, Character& target) {
    (void)ctx;
    const bool has3 = effectHas(target, kBuffLevitate);
    const bool has4 = effectHas(target, kBuffInvisible);
    const bool has5 = effectHas(target, kBuffSeeInvisible);
    const bool has6 = effectHas(target, kBuffProtect);
    target.ailments.levitating = false;
    target.ailments.invisible = false;
    target.ailments.seeInvisible = false;
    if ((target.abilities.equipAbilityMask & (1 << 3)) != 0) target.ailments.levitating = true;  // gear
    if (has3) {
        target.ailments.levitating = true;
        target.ailments.seeInvisible = true;   // quirk (R28/R29)
    }
    if (has4) target.ailments.invisible = true;
    if (has5) target.ailments.seeInvisible = true;
    if (has6) {
        target.ailments.protectLevel = protectValueOnRecompute(target.activeLevel());
    } else {
        target.ailments.protectLevel = 0;      // UNKNOWN: gear-based protection not modelled
    }
}

// R29: Protection = Int(guild Level / 2) + 1.
i32 protectValueOnRecompute(i32 guildLevel) {
    return static_cast<i32>(vbInt(static_cast<f64>(guildLevel) / 2.0)) + 1;
}

// R30: protection defence bonus = B(3 * ln(Protection)^2). B = VB Integer rounding.
i32 protectionDefenceBonus(i32 protection) {
    if (protection <= 0) return 0;  // UNKNOWN: ln(0) guard
    const f64 l = std::log(static_cast<f64>(protection));
    return static_cast<i32>(roundHalfEven(3.0 * l * l));
}

// R31: repeat casts do not stack; the grant is 95 and then recompute.
void grantSpellResistance(Ctx& ctx, Character& target, i32 effectId) {
    const i32 kind = effectReal(effectId) - kResistEffectBase;
    if (kind < 0 || kind >= kResistKinds) return;
    if (target.resistances.resist[kind] < kResistGrant) {
        target.resistances.resist[kind] = kResistGrant;
        recomputeSpellStates(ctx, target);
    }
}

// ---------------------------------------------------------------------------
// Companion casts (R32-R36)
// ---------------------------------------------------------------------------

// R32: lvl = clamp(Floor(GuildLevel / 6.0), 1, 255).
i32 companionCasterLevel(i32 companionGuildLevel) {
    const i32 lvl = static_cast<i32>(vbInt(static_cast<f64>(companionGuildLevel) / 6.0));
    return clampv(lvl, 1, 255);
}

// R33: every companion spell goes through the damage branch; a heal has AffectGroup 0 and fails.
bool companionHealFailsCast(Ctx& ctx, i32 spellId, i32 companionNumber) {
    const SpellDef& spell = spellOf(ctx, spellId);
    const bool fails = routeSpell(ctx, spellId) == SpellRoute::Heal && spell.affectGroup == 0;
    if (fails) {
        ctx.say("spells.comp.failed", "Comp. #N failed in casting", {static_cast<i64>(companionNumber)}, {});
    }
    return fails;
}

// R34: a companion-to-foe cast checks the CASTER's magic immunity (quirk; CONFLICT with R35 text).
bool companionCastBlockedByOwnImmunity(const SpellCasterInfo& caster) {
    return caster.isCompanion && caster.magicImmune;
}

// R35: companion-to-party: a target with bit 2 takes nothing.
i32 applyTargetImmunityToPartyDamage(i32 damage, u32 targetSpecialPropertyFlags) {
    return (targetSpecialPropertyFlags & kBitMagicImmune) != 0 ? 0 : damage;
}

// R36: breath against a non-aimed group applies when bit 3 is set and guild level is under 40.
bool breathSweepApplies(u32 specialAttackFlags, i32 attackerGuildLevel) {
    return (specialAttackFlags & kBitBreathSweep) != 0 && attackerGuildLevel < kBreathSweepGuildLimit;
}

// R36: the first creature of the non-aimed group falls. Each kill reloads the aimed group's front.
// UNKNOWN: the group count write-back is modelled as count = livingCount.
void companionBreathSweep(Ctx& ctx, Encounter& enc, i32 aimedGroup, i32 targetGroup) {
    (void)ctx;
    if (targetGroup == aimedGroup || !isLivingGroup(enc, targetGroup)) return;
    MonsterGroup& group = enc.groups[targetGroup];
    group.livingCount -= 1;
    group.count = group.livingCount;
    if (isLivingGroup(enc, aimedGroup)) {
        enc.groups[aimedGroup].front.hits = enc.groups[aimedGroup].front.maxHits;  // reload
    }
}

// R37: l040C = attackerLevel + 1; l040E = Int((Log(l040C) + l040C/15)/Log(1.08) - targetLevel/10 + 1).
i32 specialHitRange(i32 attackerLevel, i32 targetLevel) {
    const f64 l040C = static_cast<f64>(attackerLevel) + 1.0;
    return static_cast<i32>(vbInt((std::log(l040C) + l040C / 15.0) / std::log(1.08) -
                                  static_cast<f64>(targetLevel) / 10.0 + 1.0));
}

// R37 breath: adds a quarter of the range.
i32 companionBreathDamage(Ctx& ctx, i32 attackerLevel, i32 targetLevel, i32 resistPercent) {
    const i32 range = specialHitRange(attackerLevel, targetLevel);
    return specialDamageFromRange(ctx, range, static_cast<f64>(range) / 4.0, resistPercent);
}

// R37 spit / electrocution: adds a half of the range.
// UNKNOWN: the exact spit and electrocution expression is not given; the half-range form is used.
i32 companionSpitOrShockDamage(Ctx& ctx, i32 attackerLevel, i32 targetLevel, i32 resistPercent) {
    const i32 range = specialHitRange(attackerLevel, targetLevel);
    return specialDamageFromRange(ctx, range, static_cast<f64>(range) / 2.0, resistPercent);
}

// ---------------------------------------------------------------------------
// Monster casts and acid spit (R38-R41)
// ---------------------------------------------------------------------------

// R38: lvl = max(1, Floor(GuildLevel / 4)), capped at 255.
i32 monsterCasterLevel(i32 monsterGuildLevel) {
    i32 lvl = static_cast<i32>(vbInt(static_cast<f64>(monsterGuildLevel) / 4.0));
    if (lvl < 1) lvl = 1;
    return clampv(lvl, 1, 255);
}

// R39: companions with bit 2 take nothing; effect 100 counts only if it kills.
i32 monsterSpellDamageToTarget(Ctx& ctx, i32 monsterGuildLevel, i32 spellId, bool targetIsCompanion,
                               u32 targetFlags, i32 targetHits, bool hasResist, i32 resistPercent) {
    const SpellDef& spell = spellOf(ctx, spellId);
    const i32 lvl = monsterCasterLevel(monsterGuildLevel);
    if (targetIsCompanion && (targetFlags & kBitMagicImmune) != 0) return 0;
    const i32 dmg = monsterSpellDamage(ctx, lvl, spell.damage2, spell.specialEffect, hasResist, resistPercent);
    if (effectReal(spellEffectId(spell)) == kAllOrNothingEffect && dmg < targetHits) return 0;  // FIXED: was spell.ailments
    return dmg;
}

// R40: 50%, or 90% if the character is more than 25% above the monster, or 10% if more than 25% below.
i32 acidSpitChancePercent(i32 monsterGuildLevel, i32 charGuildLevel) {
    const f64 mon = static_cast<f64>(monsterGuildLevel);
    const f64 chr = static_cast<f64>(charGuildLevel);
    i32 chance = 50;
    if (chr > mon + mon / 4.0) chance = 90;
    if (chr < mon - mon / 4.0) chance = 10;
    return chance;
}

bool rollAcidSpit(Ctx& ctx, i32 monsterGuildLevel, i32 charGuildLevel) {
    return ctx.rng.percent(acidSpitChancePercent(monsterGuildLevel, charGuildLevel));
}

// R40: range = Int(((Log(l)+l/15)/Log(1.08) - charLevel/10) + 1) with l = monLvl + 1;
// dmg = VbInt((Int((Rnd*range+1)/2) + range/2) * 0.7). Resistance (index 10) is applied by the caller.
i32 acidSpitDamage(Ctx& ctx, i32 monsterGuildLevel, i32 charGuildLevel) {
    const f64 l046C = static_cast<f64>(monsterGuildLevel) + 1.0;
    const i32 range = static_cast<i32>(vbInt((std::log(l046C) + l046C / 15.0) / std::log(1.08) -
                                             static_cast<f64>(charGuildLevel) / 10.0 + 1.0));
    const f64 roll = static_cast<f64>(vbInt((ctx.rng.nextDouble() * range + 1.0) / 2.0));
    return static_cast<i32>(vbInt((roll + static_cast<f64>(range) / 2.0) * 0.7));
}

// R41: target = Int(Rnd * 5); 0 = character, 1..4 = companion slot.
// UNKNOWN: MONSMOD.BAS takes a second draw for the slot (mc01A0 not given); only one draw is used.
i32 rollAcidTargetSlot(Ctx& ctx) {
    return static_cast<i32>(vbInt(ctx.rng.nextDouble() * 5.0));
}

// ---------------------------------------------------------------------------
// Charm (R43-R49)
// ---------------------------------------------------------------------------

// R43: room exists while fewer than kCompanionSlots companions are occupied.
bool charmHasRoom(i32 occupiedCompanionSlots) {
    return occupiedCompanionSlots < kCompanionSlots;
}

// R43-R48: charm spell against foes.
CharmOutcome castCharm(Ctx& ctx, Encounter& enc, Arr<Companion, kCompanionSlots>& companions,
                       const SpellCasterInfo& caster, i32 spellId, i32 aimedGroup) {
    CharmOutcome out;
    if (!charmHasRoom(countOccupied(companions))) {  // R43
        out.refusedNoRoom = true;
        ctx.say("spells.charm.noroom", "Charm Humanoid - No room for more Companions!", {}, {});
        return out;
    }
    const SpellDef& spell = spellOf(ctx, spellId);
    const i32 start = spellStartGroup(enc, aimedGroup);
    const i32 gate = gateClassFor(enc, aimedGroup, start);
    DamageLoopCounters counters;
    bool spellStopped = false;
    while (!spellStopped && damageLoopContinues(counters, spell.damage1)) {
        const i32 slot = counters.step;
        bool processed = false;
        if (slot >= start && isLivingGroup(enc, slot)) {
            processed = true;
            MonsterGroup& group = enc.groups[slot];
            for (i32 roll = 0; roll < spell.affectGroup && group.livingCount > 0; ++roll) {
                Creature& front = group.front;
                const i32 resist = victimResistPercent(front, spell.resistIndex);
                const i32 dmg = playerSpellDamage(ctx, spell.damage2, spell.specialEffect, caster.level, resist);
                const VictimVerdict verdict = judgeSpellVictim(front, kCharmEffectKind, dmg, gate, caster.isCompanion);
                if (verdict != VictimVerdict::Hit) {
                    // R44: a monster the damage would not kill is left untouched; the survivor stops the spell.
                    if (verdict == VictimVerdict::MagicImmune) out.casterOrderToFight = true;
                    spellStopped = true;
                    break;
                }
                // R45: the creature leaves the fight and joins at the time of the cast, if a slot is free.
                const i32 freeSlot = findFreeCompanionSlot(companions);
                if (caster.isCompanion) {
                    out.joinSkipped = true;            // R47: vanishes, no join
                    out.charmedCount += 1;
                } else if (freeSlot >= 0) {
                    companions[freeSlot] = makeCharmedCompanion(ctx, front, front.hits, front.maxHits,
                                                                caster.level, freeSlot);
                    out.charmedCount += 1;
                }
                // R48: with no free slot the creature still vanishes, and the count is not kept.
                front.alive = false;
                group.livingCount -= 1;
                if (group.livingCount > 0) {
                    group.front.hits = group.front.maxHits;   // UNKNOWN: next member's hits
                    group.front.alive = true;
                }
            }
        }
        advanceDamageLoop(counters, processed);
    }
    if (out.charmedCount > 0) {
        ctx.say("spells.charm.success", "You charmed One!", {static_cast<i64>(out.charmedCount)}, {});  // R46
    } else {
        ctx.say("spells.failed", "Failed!", {}, {});                                                 // R46
        out.casterOrderToFight = true;
    }
    return out;
}

// R45: a charmed creature joins with its hits at cast time and its rolled maximum,
// bound at the spell's level; alignment copied from the foe (MAINMOD2.BAS:392).
// UNKNOWN: "identified by fn1272" and the companion stat ordering are not modelled.
Companion makeCharmedCompanion(Ctx& ctx, const Creature& foe, i32 hitsAtCast, i32 rolledMaxHits,
                               i32 spellLevel, i32 slot) {
    (void)ctx;
    Companion c;
    c.occupied = true;
    c.slot = slot;
    c.monsterId = foe.id;
    c.name = foe.knownAs;
    c.alignment = static_cast<CompanionAlignment>(static_cast<i32>(foe.align) + 1);  // CharAlign 0..2 -> 1..3
    c.bindingLevel = spellLevel;
    c.bound = true;
    c.hits = hitsAtCast;
    c.maxHits = rolledMaxHits;
    c.guildLevel = foe.guildLevel;
    c.specialPropertyFlags = foe.specialPropertyFlags;
    c.present = true;
    return c;
}

// R49: the charmed creature counts as a kill with no hits.
// Port deviation (R49): the port increments the player's kills on charm.
void creditCharmKill(Ctx& ctx, Character& caster) {
    (void)ctx;
    caster.kills += 1;
}

// ---------------------------------------------------------------------------
// Teleport, displacement, passwall (R50-R55)
// ---------------------------------------------------------------------------

// R50: teleport takes Right, Forward, Down (max 3); displacement takes Right, Forward.
// Fewer than two values: nothing happens (returns false).
bool readTypedOffsets(const Vec<i32>& values, bool teleport, TypedOffset& out) {
    if (values.size() < 2) return false;
    out.right = values[0];
    out.forward = values[1];
    out.down = (teleport && values.size() >= 3) ? values[2] : 0;
    return true;
}

// R51: offsets are relative to facing. Right/forward unit vectors per facing.
// UNKNOWN: the grid axis convention (N = -y) is assumed.
Location offsetFromFacing(const Location& from, const TypedOffset& off, Facing facing) {
    i32 fx = 0, fy = -1, rx = 1, ry = 0;
    switch (facing) {
        case Facing::N: fx = 0;  fy = -1; rx = 1;  ry = 0;  break;
        case Facing::E: fx = 1;  fy = 0;  rx = 0;  ry = 1;  break;
        case Facing::S: fx = 0;  fy = 1;  rx = -1; ry = 0;  break;
        case Facing::W: fx = -1; fy = 0;  rx = 0;  ry = -1; break;
    }
    Location r = from;
    r.x = wrapGrid(from.x + off.right * rx + off.forward * fx);   // R51 wrap
    r.y = wrapGrid(from.y + off.right * ry + off.forward * fy);
    r.dungeonLevel = from.dungeonLevel + off.down;               // UNKNOWN: positive down = deeper
    return r;
}

// R53: level 0 (or below) falls from the sky; above MaxDungeonLevel or on rock is rock death.
ArrivalOutcome arrivalOutcome(i32 destLevel, bool destIsRock) {
    if (destLevel <= 0) return ArrivalOutcome::FallsFromTheSky;
    if (destLevel > kLocalMaxDungeonLevel) return ArrivalOutcome::IntoRock;
    if (destIsRock) return ArrivalOutcome::IntoRock;
    return ArrivalOutcome::Arrives;
}

// R52: the outcome is decided before anyone moves; the party then moves unless it falls from the sky.
ArrivalOutcome castTeleportOrDisplace(Ctx& ctx, Party& party, FloorState& floor, i32 casterIndex,
                                      i32 spellId, const TypedOffset& off) {
    const SpellDef& spell = spellOf(ctx, spellId);
    const Location dest = offsetFromFacing(locationOf(party), off, party.facing);
    const ArrivalOutcome outcome = arrivalOutcome(dest.dungeonLevel, rockAt(floor, dest));
    if (outcome != ArrivalOutcome::FallsFromTheSky) moveParty(party, dest);
    settleArrival(ctx, party, floor, casterIndex, outcome, spellPointCost(spell));  // FIXED: was spell.pointCost
    return outcome;
}

// R52: passwall moves one square forward first, then tests the square it lands on.
ArrivalOutcome castPasswall(Ctx& ctx, Party& party, FloorState& floor, i32 casterIndex, i32 spellId) {
    const SpellDef& spell = spellOf(ctx, spellId);
    TypedOffset step;
    step.right = 0;
    step.forward = 1;
    step.down = 0;
    const Location dest = offsetFromFacing(locationOf(party), step, party.facing);
    moveParty(party, dest);
    const ArrivalOutcome outcome = arrivalOutcome(dest.dungeonLevel, rockAt(floor, dest));
    settleArrival(ctx, party, floor, casterIndex, outcome, spellPointCost(spell));  // FIXED: was spell.pointCost
    return outcome;
}

// R54: rock death. The whole party dies; bodies are left on this rock square and level.
void applyRockDeath(Ctx& ctx, Party& party, FloorState& floor) {
    const i32 idx = squareIndexOf(party.x, party.y);
    const bool validSquare = idx >= 0 && idx < static_cast<i32>(floor.squares.size());
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[i];
        m.deathState = DeathState::Rock;
        m.vitals.curHits = 0;
        if (validSquare) floor.squares[idx].bodies.push_back(m);
    }
    party.wiped = true;
    ctx.say("spells.rock.death", "The party is crushed by rock.", {}, {});
}

// R55: rock death costs no points. Otherwise the caster arrives, points are spent, and the arrival
// square is resolved. UNKNOWN: a fall from the sky is treated as no arrival (no points, no resolve).
void settleArrival(Ctx& ctx, Party& party, FloorState& floor, i32 casterIndex,
                   ArrivalOutcome outcome, i32 spellPointCost) {
    if (outcome == ArrivalOutcome::IntoRock) {
        applyRockDeath(ctx, party, floor);
        return;
    }
    if (outcome == ArrivalOutcome::FallsFromTheSky) {
        ctx.say("spells.fall", "The spell drops you from the sky.", {}, {});
        return;
    }
    if (casterIndex >= 0 && casterIndex < party.size) {
        Vitals& v = party.members[casterIndex].vitals;
        v.curSpellPoints = v.curSpellPoints - spellPointCost;
        if (v.curSpellPoints < 0) v.curSpellPoints = 0;
    }
    party.arrivedBySpell = true;
    ctx.say("spells.arrive", "The party arrives.", {}, {});
}

}  // namespace mordor
