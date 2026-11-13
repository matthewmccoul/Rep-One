// companions2b.cpp
// Implements include/mordor/companions2b.h.
// Rule ids such as [c8] refer to the SRD c12p2 source notes.
//
// Declarations in companions2b.h that are ALSO declared in another header
// are NOT defined here (one definition per signature):
//   clampChanceToLeave(i32)  -> companions2a.h
//   bindSpellApplies(const Companion&) -> companions1a.h
// CONFLICT: the same signatures are declared in two headers; the owning
// module defines them.

#include "mordor/companions2b.h"
#include "mordor/companions1b.h"  // FIXED: i32 overloads of companionCastLevel and the 4-arg stoneLands
#include "mordor/companions2a.h"  // FIXED: i32 overload of companionRaiseChance
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/events.h"

namespace mordor {

namespace {

// A companion slot is live when it is occupied and not dead or stoned.
bool isLiveCompanion(const Companion& c) {
    return c.occupied && !c.isDead && c.deathState == CompanionDeathState::Normal;
}

}  // namespace

// ---------------------------------------------------------------------------
// Binding and leaving
// ---------------------------------------------------------------------------

Currency bindingStepCost(Currency current, i32 bindingLevel) {
    // [c2] Cost factor is not the level bound to; steps get cheaper near the top.
    // CONFLICT: engine (c2) f = (a+4 > 12) ? 12-a : a+4 is preferred over the
    // doc form (c30). Negative binding gives a negative factor [c2].
    const i64 a = static_cast<i64>(bindingLevel);
    const i64 f = (a + 4 > 12) ? (12 - a) : (a + 4);
    // UNKNOWN: divisor/scale of the cost; cost = current * f is used here.
    return current * f;
}

i32 chanceToLeave(const Companion& comp) {
    // UNKNOWN: formula producing the raw chance is not in the SRD excerpt.
    // The stored raw value is clamped as the rules require [c2].
    return clampChanceToLeave(comp.chanceToLeave);
}

// ---------------------------------------------------------------------------
// Turns (kick-out / leaving)
// ---------------------------------------------------------------------------

i32 companionTurnPercent(CharAlign creatureAlign, Align masterAlign) {
    // [c8] Original reads the creature's alignment, not the companion's.
    const i32 c = static_cast<i32>(creatureAlign);
    const i32 m = static_cast<i32>(masterAlign);
    i32 gap = c - m;
    if (gap < 0) {
        gap = -gap;
    }
    // [c8][c27] A Good creature under a Good master never turns.
    // UNKNOWN: Evil/Evil and Neutral/Neutral are treated as the same gap-0 case.
    if (gap == 0) {
        return 0;
    }
    // [c27] A two-step gap always turns (measured 100%).
    if (gap >= 2) {
        return 100;
    }
    // [c8] Gap of one: 75% for Evil, 15% otherwise.
    // UNKNOWN: rate for a Good creature under a Neutral master.
    return (creatureAlign == CharAlign::Evil) ? 75 : 15;
}

bool companionTurnRoll(Ctx& ctx, CharAlign creatureAlign, Align masterAlign) {
    const i32 p = companionTurnPercent(creatureAlign, masterAlign);
    // UNKNOWN: whether the original consumes a random draw when p is 0 or 100.
    if (p <= 0) {
        return false;
    }
    if (p >= 100) {
        return true;
    }
    return ctx.rng.percent(p);
}

bool kickOutLeavesSilently(Ctx& ctx, CharAlign creatureAlign, Align masterAlign) {
    // [c8][c27] Kicked-out companions leave with no print if they do not turn.
    return !companionTurnRoll(ctx, creatureAlign, masterAlign);
}

// ---------------------------------------------------------------------------
// Friendly fire, breath and spit
// ---------------------------------------------------------------------------

bool rollFriendlyFire(Ctx& ctx, const Companion& comp) {
    // [c5] Every companion rolls friendly fire, not only spitters.
    (void)comp;
    // UNKNOWN: friendly-fire percentage is not in the SRD excerpt.
    // Placeholder uses the breath base roll of 50.
    return ctx.rng.percent(50);
}

bool breathStopped(BreathType type, bool breathedEarlierInRound) {
    // [c5][c28] Only a cold breath is stopped by an earlier breath in the round.
    // CONFLICT: the VB form `bit3 Or (bit4 And not Breathed)` is ambiguous.
    // Implemented per prose: fire, acid and electrical are never stopped.
    return type == BreathType::Cold && breathedEarlierInRound;
}

i32 breathRelation(i32 breatherPower, i32 targetPower) {
    // [c28] Relation is "much stronger" (+1), "much weaker" (-1) or in between (0).
    // UNKNOWN: threshold is taken as a ratio of a quarter (more than a quarter).
    const i64 b = static_cast<i64>(breatherPower);
    const i64 t = static_cast<i64>(targetPower);
    if (4 * b > 5 * t) {
        return 1;
    }
    if (4 * b < 3 * t) {
        return -1;
    }
    return 0;
}

i32 breathFireChancePercent(i32 relation) {
    // [c28][c64] Fire if roll < (50 | 80 | 20).
    // CONFLICT: test (c64) marks the in-between value UNKNOWN; docs (c28) give 50.
    if (relation > 0) {
        return 80;
    }
    if (relation < 0) {
        return 20;
    }
    return 50;
}

bool breathRollFires(Ctx& ctx, i32 relation) {
    // [c28] Roll against the relation-based chance.
    return ctx.rng.percent(breathFireChancePercent(relation));
}

i32 specialRangeExtra(SpecialAttackKind kind, i32 range) {
    // [c64] Breath adds a quarter of the range; spit and electrocution add a half.
    // UNKNOWN: how the extra is applied to the range; floor rounding is used.
    const double divisor = (kind == SpecialAttackKind::Breath) ? 4.0 : 2.0;
    return static_cast<i32>(vbInt(static_cast<double>(range) / divisor));
}

i32 drawSpitSlot(Ctx& ctx, i32 attackerCompanionCount, const Vec<bool>& victimSlotOccupied) {
    // [c8][c28] The draw counts the attacker's companions but indexes the
    // victim's list (original). Port: empty slot is a miss, retries capped at 1000.
    // UNKNOWN: whether a miss retries the draw or ends the spit.
    if (attackerCompanionCount <= 0) {
        return -1;
    }
    const i32 victimCount = static_cast<i32>(victimSlotOccupied.size());
    for (i32 tries = 0; tries < 1000; ++tries) {
        const i32 slot = static_cast<i32>(ctx.rng.below(attackerCompanionCount));
        if (slot >= 0 && slot < victimCount && victimSlotOccupied[static_cast<u32>(slot)]) {
            return slot;
        }
    }
    return -1;
}

Str companionMishapSuffix(const Companion& lastSlotVisited) {
    // [c6][c29][c65] The suffix is reset on each pass, so it names only the
    // last slot visited.
    // UNKNOWN: exact suffix behaviour beyond the literal; empty for an empty slot.
    if (!lastSlotVisited.occupied) {
        return Str("");
    }
    return Str(" and his companions");
}

bool mishapEndsPass(const MishapResult& result) {
    // [c28] A fatal mishap ends the routine before its line.
    // UNKNOWN: MishapResult field layout is not supplied to this module.
    // Conservative: a pass is never ended early.
    (void)result;
    return false;
}

// ---------------------------------------------------------------------------
// Stone and electrocution
// ---------------------------------------------------------------------------

bool stoneLands(Ctx& ctx, const Creature& stoner, const Creature& target) {
    // [c6][c28] The CON test reads the stoner's CON against the target.
    // UNKNOWN: the target stone-resistance input and levelFound term are
    // mapped onto the companion-slot formula in companions1a.h.
    return stoneLands(ctx, target.resistance, stoner.con, target.levelFound);
}

i32 nextStoneFoeIndex(const Vec<Creature>& foes, i32 fromIndex) {
    // [c28] Stone and electrocution follow-ups test Foes(M547A).Hits > 0 and so
    // can land on the next monster in the group.
    // UNKNOWN: scan forward for the first living foe; -1 if none.
    const i32 n = static_cast<i32>(foes.size());
    for (i32 i = fromIndex; i < n; ++i) {
        if (i >= 0 && foes[static_cast<u32>(i)].hits > 0) {
            return i;
        }
    }
    return -1;
}

void stoneCompanion(Ctx& ctx, Party& party, Companion& comp) {
    // [c6][c8] A stoned companion empties its slot before its carry is read,
    // so the carry is left on a hole (resolved by the caller).
    (void)party;
    comp.deathState = CompanionDeathState::Stone;
    comp.isDead = true;
    comp.hits = 0;
    comp.occupied = false;
    // UNKNOWN: whether the stoned caption is guarded by the watched character.
    ctx.say("companion.stoned", "Stoned one!", {}, {comp.name});
}

void sayElectrocutionCaption(Ctx& ctx, const Character& watched, i32 damage) {
    // [c6][c28] Not inside a caption guard: the watched character is told even
    // if someone else is hit.
    // CONFLICT: engine text (c6) preferred over the docs' "for N!" wording.
    ctx.say("companion.electrocuted", "You were accidently electrocuted", {damage}, {watched.name});
}

bool companionKillEndsOwnerRound(Ctx& ctx, Companion& killer, Character& victim) {
    // [c28] Each mdrcharSetDead is followed by Exit Sub, so the killer is the
    // owner's last companion to act this round. Returns true when the victim
    // was killed by this blow.
    // CONFLICT: companion kills are not counted to the owner's Kills (engine c5)
    // rather than counted (docs c36). Engine preferred.
    (void)ctx;
    (void)killer;
    if (victim.vitals.curHits > 0 || victim.isDead()) {
        return false;
    }
    victim.deathState = DeathState::Dead;
    // [c8] No text on an ordinary companion kill.
    return true;
}

// ---------------------------------------------------------------------------
// Blow
// ---------------------------------------------------------------------------

void companionBlowFoe(Ctx& ctx, Companion& attacker, Creature& foe, const MonsterDef& attackerDef) {
    // [c6][c35] Companion blow uses the monster's Att and Def, not per-companion
    // fields. [c8][c64] No text on an ordinary hit, miss or kill.
    // UNKNOWN: the blow roll and damage formula are not in the SRD excerpt
    // supplied to this module. This function performs no hit resolution yet.
    (void)ctx;
    (void)attacker;
    (void)foe;
    (void)attackerDef;
}

i32 companionBlowDefence(const Character& target) {
    // [c6][c28] Protection is not added to the character's Def for companion blows.
    return target.combat.defence;
}

// ---------------------------------------------------------------------------
// Spells
// ---------------------------------------------------------------------------

i32 companionCastLevel(const Companion& comp) {
    // [c29] Companions cast through mdrcharCastSpell at guild level / 6.
    return companionCastLevel(comp.guildLevel);
}

bool companionCastDraw(Ctx& ctx, const Companion& comp) {
    // [c29] The companion's cast draw gates every cast.
    // UNKNOWN: draw percentages. A level-1 caster is said to cap at 40% [c65];
    // other levels are given 100 here.
    const i32 level = companionCastLevel(comp);
    if (level <= 0) {
        return false;
    }
    const i32 chance = (level == 1) ? 40 : 100;
    return ctx.rng.percent(chance);
}

bool spellHealsNoOne(i32 m5b1d) {
    // [c29] Heals (M5B1D = 0) reach nobody.
    return m5b1d == 0;
}

// ---------------------------------------------------------------------------
// Carry and death
// ---------------------------------------------------------------------------

bool carryChoiceApplies(const Party& party) {
    // [c8] While a morgue body is carried, the carry choice is skipped.
    return party.carryingSlot != CarryingSlot::DeadCharacter;
}

void announceDropped(Ctx& ctx, const Companion& fallen) {
    // [c8][c30] "dropped" prints before any size test, so it shows even when
    // the new body is kept out.
    ctx.say("companion.dropped", "A companion was dropped", {fallen.slot}, {fallen.name});
}

void resolveCarryOnDeath(Ctx& ctx, Party& party, Companion& fallen) {
    // [c8][c30] Runs when a companion's Hits reach zero.
    if (!carryChoiceApplies(party)) {
        return;
    }
    // CONFLICT: engine (c8) prints "dropped" only when nothing is carried;
    // docs (c30) print on any later fallen companion. Engine preferred.
    if (party.carryingSlot == CarryingSlot::None) {
        announceDropped(ctx, fallen);
        party.carryingSlot = CarryingSlot::Companion;
        party.carryingCompanion = fallen.slot;
    }
    // UNKNOWN: size comparison when a companion is already carried; the
    // original's slot -1 error path [c30] is not reproduced.
}

void releaseAllOnDeath(Ctx& ctx, Party& party) {
    // UNKNOWN: "Death lets go of every companion and anything carried"
    // [c30]; whether this is character death is unresolved. Companions live
    // outside Party, so only the carry is released here.
    (void)ctx;
    party.carryingSlot = CarryingSlot::None;
    party.carryingCompanion = -1;
}

// ---------------------------------------------------------------------------
// Purchase and joining
// ---------------------------------------------------------------------------

i32 clampPurchaseHits(i32 rolledHits, i32 definitionHits) {
    // [c28][c51] Companion hits are capped at max on purchase.
    return (rolledHits > definitionHits) ? definitionHits : rolledHits;
}

void applyJoinerHitsFromFront(Ctx& ctx, Companion& joiner, const Creature& front) {
    // [c7][c65] All joiners from a fight arrive with the front monster's
    // hits and maximum, not their own.
    (void)ctx;
    joiner.hits = front.hits;
    joiner.maxHits = front.maxHits;
}

bool joinOfferVisibleFor(const Encounter& enc, i32 groupIndex) {
    // [c7] Only group 0's join offer is ever shown. Multi-group encounters never
    // show an offer.
    // UNKNOWN: "multi-group" taken as more than one group with count > 0.
    if (groupIndex != 0) {
        return false;
    }
    i32 groupsPresent = 0;
    for (u32 i = 0; i < static_cast<u32>(kMaxGroups); ++i) {
        if (enc.groups[i].count > 0) {
            ++groupsPresent;
        }
    }
    if (groupsPresent != 1) {
        return false;
    }
    return enc.groups[0].joinOffer;
}

bool joinOfferAdmits(CharAlign groupAlign, Align viewerAlign) {
    // [c7] The join-offer rule admits a group with alignment 1 or matching the viewer.
    // CONFLICT: wiki (c91) says Good monsters join only Good characters.
    // Engine (c7) preferred; "alignment 1" read as Neutral on the character scheme.
    const i32 g = static_cast<i32>(groupAlign);
    const i32 v = static_cast<i32>(viewerAlign);
    return g == 1 || g == v;
}

i32 joinChancePercent(i32 cha, i32 mLvl) {
    // UNKNOWN: the full join chance table (CHA 3-30, mLvl 1-15) is not supplied
    // [c91]. No join chance is applied until it is.
    (void)cha;
    (void)mLvl;
    return 0;
}


// ---------------------------------------------------------------------------
// Pens and slots
// ---------------------------------------------------------------------------

bool penHasRoomFor(const Pen& pen, CompanionAlignment align) {
    // [c9][c27] Pens hold three alignments. The cap of nine applies to the
    // unaligned count; aligned shelves may reach 24 [c27].
    // UNKNOWN: Pen layout is not supplied to this module. Conservative refusal.
    (void)pen;
    (void)align;
    return false;
}

void penFromIdKeys(Ctx& ctx, Pen& pen) {
    // [c9] Pen files keyed by Id are remapped on load (FromIdKeys).
    // UNKNOWN: Pen layout is not supplied to this module. No remap performed.
    (void)ctx;
    (void)pen;
}

void releaseCompanionSlot(Ctx& ctx, Companion& comp) {
    // [c8] A released slot keeps its last occupant's record. Only the slot
    // state changes; the name is not blanked here.
    (void)ctx;
    comp.released = true;
    comp.occupied = false;
}

bool dragSwapBlockedByDeath(const Companion& a, const Companion& b) {
    // [c8][c55] Drag swap is refused if either end is dead.
    const bool aDead = a.isDead || a.deathState != CompanionDeathState::Normal;
    const bool bDead = b.isDead || b.deathState != CompanionDeathState::Normal;
    return aDead || bDead;
}

void sayDragSwapRefused(Ctx& ctx) {
    // [c8][c55] Refusal text for dragging a dead companion.
    ctx.say("companion.dragSwapDead", "No use re-arranging a dead companion!", {}, {});
}

CompanionAlignment alignOnLoad(Ctx& ctx, i32 savedAlign, CharAlign creatureAlign) {
    // [c8] Port deviation: an Unaligned companion saved by older charm or join
    // code is given its creature's alignment on load.
    (void)ctx;
    if (savedAlign >= 1 && savedAlign <= 3) {
        return static_cast<CompanionAlignment>(savedAlign);
    }
    // CharAlign Good/Neutral/Evil = 0/1/2 maps to CompanionAlignment 1/2/3.
    return static_cast<CompanionAlignment>(static_cast<i32>(creatureAlign) + 1);
}

bool isSharedMorgulId(i32 monsterId) {
    // [c8][c9][c55] Only Id 307 is shared.
    return monsterId == 307;
}

// ---------------------------------------------------------------------------
// Spawn
// ---------------------------------------------------------------------------

bool spawnModeInData(const GameData& data, i32 mode) {
    // [c7] No monster in MDATA5 has spawn mode 5, 6 or 7.
    // UNKNOWN: modes 0-4 are assumed present; data is not scanned here.
    (void)data;
    return mode >= 0 && mode <= 4;
}

bool spawnTryGuardHit(i32 tries) {
    // [c7] The 300-try guard.
    return tries >= 300;
}

i32 advanceSpawnTries(i32 tries) {
    // [c7] The counter increments per try and resets only per group (by the caller).
    // UNKNOWN: placement of the 150-try reset, which shares this counter.
    return tries + 1;
}

// ---------------------------------------------------------------------------
// Kicks, right-click, raise, drift
// ---------------------------------------------------------------------------

i32 rightClickReferenceAlign() {
    // [c43][c55] Port reproduces the original: m02BE is never assigned, so it
    // reads 0 (Good).
    return 0;
}

i32 companionRaiseChance(const Companion& comp) {
    // [c25][c50] Raise chance is min(98, 85 + CON). Companion CON is Stats[3] [c28][c50].
    return companionRaiseChance(comp.stats[3]);
}

bool payAndRollRaise(Ctx& ctx, Purse& purse, Currency cost, Companion& comp) {
    // [c50] Gold is taken before the roll and is not refunded.
    // UNKNOWN: refusal rule when gold is short; the cost formula is supplied by the caller.
    const Currency total = purse.goldOnHand + purse.goldInBank;
    if (total < cost) {
        return false;
    }
    const Currency fromHand = (purse.goldOnHand < cost) ? purse.goldOnHand : cost;
    purse.goldOnHand -= fromHand;
    purse.goldInBank -= (cost - fromHand);

    const i32 chance = companionRaiseChance(comp);
    comp.pendingRaise = false;
    if (ctx.rng.percent(chance)) {
        comp.isDead = false;
        comp.deathState = CompanionDeathState::Normal;
        // UNKNOWN: hits set on a successful raise.
        comp.hits = 1;
        return true;
    }
    // [c25][c50] Failure destroys the companion.
    comp.occupied = false;
    ctx.say("companion.raiseFailed", "The raise failed and the companion is lost", {}, {comp.name});
    return false;
}

i32 distinctLiveCompanionIds(const Vec<Companion>& companions) {
    // [c43] Port's groups count is the distinct live Def.Id values (monsterId).
    const i32 n = static_cast<i32>(companions.size());
    i32 count = 0;
    for (i32 i = 0; i < n; ++i) {
        const Companion& ci = companions[static_cast<u32>(i)];
        if (!isLiveCompanion(ci)) {
            continue;
        }
        bool seen = false;
        for (i32 j = 0; j < i; ++j) {
            const Companion& cj = companions[static_cast<u32>(j)];
            if (isLiveCompanion(cj) && cj.monsterId == ci.monsterId) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            ++count;
        }
    }
    return count;
}

f32 companionDriftFloor(const Companion& comp) {
    // [c8] Floor 0.5 for content companions, so full health gives 0.5.
    // CONFLICT: docs (c28) say nothing at full health; engine (c8) preferred.
    // UNKNOWN: which companions are non-content and their floor.
    (void)comp;
    return 0.5f;
}

void driftMoodOncePerRound(Ctx& ctx, Companion& comp) {
    // [c8] Drift runs once per combat round.
    // UNKNOWN: the mood store is not a field of Companion; no state is changed.
    (void)ctx;
    (void)comp;
}

}  // namespace mordor
