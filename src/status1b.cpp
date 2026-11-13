// status1b.cpp
// Implements include/mordor/status1b.h.
// Source rules: SRD part c08p1 (R38-R55 chest traps, ailment tick, death/rescue/raise,
// companion drift and leave, cure notes).
//
// NOTE: companionRaiseChance(i32) is declared in companions2a.h and defined there.
// It is declared again in status1b.h with the same signature, but is NOT defined here.
// This module calls it.

#include <algorithm>
#include <cmath>

#include "mordor/status1b.h"
#include "mordor/math_util.h"
#include "mordor/death1a.h"
#include "mordor/guilds1.h"
#include "mordor/items1a.h"
#include "mordor/companions2a.h"

namespace mordor {
namespace {

inline u32 ix(i32 v) { return static_cast<u32>(v); }

i32 statOf(const Character& c, StatIndex s) {
    return c.abilities.stats[ix(static_cast<i32>(s))];
}

void saveChar(Ctx& ctx, const Character& c) {
    // SaveChar: persistence is outside the core; the core emits an event only.
    ctx.say("char.save", "Character saved.", {}, {c.name});
}

void offerRescue(Ctx& ctx, const Character& who) {
    // OfferRescue: host decides presentation; core emits an event only.
    ctx.say("rescue.offer", "Rescue offered.", {}, {who.name});
}

}  // namespace

// ---------------------------------------------------------------------------
// Chest traps (R38-R41)
// ---------------------------------------------------------------------------

// R38: trap 1. Each party member in order rolls against resistance index 5
// (ResistPoison). Afflicted when rng*100 < ResistancePercent/2. No message.
// UNKNOWN: ResistancePercent is taken as resistances.resist[kind] (a percent).
void chestPoisonGasTrap(Ctx& ctx, Party& party) {
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[ix(i)];
        const f64 pct = static_cast<f64>(m.resistances.resist[ix(kResistPoisonIndex)]);
        if (ctx.rng.nextDouble() * 100.0 < pct / 2.0) {
            m.ailments.poisonLevel = party.dungeonLevel;  // Poisoned = true; PoisonLevel = dungeonLevel
        }
    }
}

// R38: trap 2. Same as trap 1 with resistance index 4 (ResistDisease).
void chestDiseaseSporesTrap(Ctx& ctx, Party& party) {
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[ix(i)];
        const f64 pct = static_cast<f64>(m.resistances.resist[ix(kResistDiseaseIndex)]);
        if (ctx.rng.nextDouble() * 100.0 < pct / 2.0) {
            m.ailments.diseaseLevel = party.dungeonLevel;  // Diseased = true; DiseaseLevel = dungeonLevel
        }
    }
}

// R39: trap 6 (Fate). Reroll current HP then current SP, each 1..max.
// CurHits = (int)(rng*MaxHits)+1 ; CurSpellPoints = (int)(rng*MaxSpellPoints)+1
void chestFateTrap(Ctx& ctx, Character& opener) {
    opener.vitals.curHits =
        static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<f64>(opener.vitals.maxHits))) + 1;
    opener.vitals.curSpellPoints =
        static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<f64>(opener.vitals.maxSpellPoints))) + 1;
}

// R40: trap 8 (Blackout) clears every spell effect on the opener.
void chestBlackoutTrap(Ctx& ctx, Character& opener) {
    (void)ctx;
    opener.activeEffectIds.clear();
}

// Dispatch on chest trap kind. The opener is party.members[party.opener].
// R41: traps other than 1, 2, 6, 8 are UNKNOWN in the source; they do nothing here.
// UNKNOWN: Explosion, Fire, ItemDestruction, Trap7, Trap9, Trap10 have no supplied mechanics.
void applyChestTrap(Ctx& ctx, Party& party, ChestTrap trap) {
    switch (trap) {
        case ChestTrap::PoisonGas:
            chestPoisonGasTrap(ctx, party);
            break;
        case ChestTrap::DiseaseSpores:
            chestDiseaseSporesTrap(ctx, party);
            break;
        case ChestTrap::Fate:
            chestFateTrap(ctx, party.members[ix(party.opener)]);
            break;
        case ChestTrap::Blackout:
            chestBlackoutTrap(ctx, party.members[ix(party.opener)]);
            break;
        default:
            // R41 UNKNOWN: no mechanics supplied.
            break;
    }
}

// ---------------------------------------------------------------------------
// Four-second ailment tick helpers (R42, R43)
// ---------------------------------------------------------------------------

// R42 step 1: ambient sound. Flat 10% per tick when nothing is in front.
// UNKNOWN: the original behaviour is not stated; the port uses this flat chance.
bool ambientSoundRoll(Ctx& ctx, bool nothingInFront) {
    if (!nothingInFront) {
        return false;
    }
    return ctx.rng.percent(kAmbientSoundPercent);
}

// R42 step 2b: water or quicksand. If not levitating, CurHits -= hazardLevel.
// Quicksand may destroy a random pack item (R43 roll first, then slot pick).
// The core cannot see the pack. itemDestroyed=true means "destroy destroyedSlot if
// Inventory.pack[destroyedSlot].destructible"; the caller applies the destruction.
// UNKNOWN: whether the R43 quicksand chance is applied (here it is).
// Drowning shows no message (quirk; no event emitted).
HazardResult waterOrQuicksandHit(Ctx& ctx, Character& m, i32 hazardLevel, bool quicksand) {
    HazardResult r;
    if (m.ailments.levitating) {
        return r;
    }
    m.vitals.curHits -= hazardLevel;
    r.hitsLost = hazardLevel;
    if (quicksand && quicksandDestroyRoll(ctx, hazardLevel)) {
        // slot = rng.Next(PackSlots-1)+1
        r.destroyedSlot = static_cast<i32>(ctx.rng.below(kPackSlots - 1)) + 1;
        r.itemDestroyed = true;
    }
    return r;
}

// R42 step 2f: old-age check.
// UNKNOWN: Rules.DiesOfOldAge formula not supplied. Simplest sensible choice:
// deterministic death at or past the race maximum age.
bool oldAgeCheck(Ctx& ctx, const Character& c, i32 raceMaxAgeDays) {
    (void)ctx;
    return c.ageDays >= raceMaxAgeDays;
}

// R43: quicksand destruction chance. Rnd(100) < (level/25)*100.
// UNKNOWN: integer versus real division of level/25 is not stated; real division used.
bool quicksandDestroyRoll(Ctx& ctx, i32 hazardLevel) {
    const f64 chance = (static_cast<f64>(hazardLevel) / 25.0) * 100.0;
    return ctx.rng.nextDouble() * 100.0 < chance;
}

// R44 first pass: if !Identified && !unseen && rng*100 < RecogniseChance(skill, dark).
// UNKNOWN: constants of IdentifySkill / RecogniseChance are not supplied (see items/guilds).
// UNKNOWN: "invisible needs See Invisible" has no Creature flag to test; not applied.
bool identifyFirstPass(Ctx& ctx, const Character& ch, Creature& foe, bool dark) {
    const i32 skill = identifySkill(ch.topGuildLevel,
                                    statOf(ch, StatIndex::Int),
                                    statOf(ch, StatIndex::Wis));
    if (foe.identified || foe.unseen) {
        return false;
    }
    const i32 chance = recogniseChance(static_cast<f64>(skill), dark);
    if (ctx.rng.nextDouble() * 100.0 < static_cast<f64>(chance)) {
        foe.identified = true;
        return true;
    }
    return false;
}

// R44 second pass: if !unseen && Believed != Def && rng(100)+1 < RenameChance(skill, dark).
// UNKNOWN: "Believed" is taken as believedRef and "Def" as the defined id.
// UNKNOWN: fog cuts the second-pass skill to three quarters (applied here);
//          renameChance also receives the dark flag, so the fog effect may be counted twice.
// UNKNOWN: effect of success is taken as believedRef = id (believed becomes defined).
bool identifySecondPass(Ctx& ctx, const Character& ch, Creature& foe, bool dark) {
    // R44: second pass runs only if !unseen && Believed != Def.
    // Def is the monster's defined identity (foe.ref), not the instance id.
    if (foe.unseen || foe.believedRef == foe.ref) {
        return false;
    }
    const f64 base = static_cast<f64>(identifySkill(ch.topGuildLevel,
                                                    statOf(ch, StatIndex::Int),
                                                    statOf(ch, StatIndex::Wis)));
    // R44: fog (dark) cuts the second-pass skill to three quarters.
    const f64 skill = dark ? base * 0.75 : base;
    const i32 chance = renameChance(skill, dark);
    // R44: rng(100)+1 < RenameChance(skill, dark).
    if (ctx.rng.below(100) + 1 < chance) {
        foe.believedRef = foe.ref;
        return true;
    }
    return false;
}

// R42 step 2a: identification pass over each group in front (first then second pass).
void identificationPass(Ctx& ctx, const Character& ch, Encounter& front, bool dark) {
    for (i32 g = 0; g < kMaxGroups; ++g) {
        MonsterGroup& grp = front.groups[ix(g)];
        if (grp.count <= 0) {
            continue;
        }
        identifyFirstPass(ctx, ch, grp.front, dark);
        identifySecondPass(ctx, ch, grp.front, dark);
    }
}

// ---------------------------------------------------------------------------
// Death, rescue and raise (R45-R54)
// ---------------------------------------------------------------------------

// R45 EndEveryAilment: Unparalyse; CurePoison; CureDisease; SpellResist[all]=0; Effects.Recompute.
// Stone (DeathState) is not cleared.
// CONFLICT: R45. The c2 path restores the stat part through Unparalyse; the c30/c34
// quirk clears ParalysisLevel without giving the stat part back. This port follows the
// quirk path: the stat part is not restored here. Whether this matches the original is UNKNOWN.
// Effects.Recompute is not defined in this module (no-op).
void cityGateClear(Ctx& ctx, Character& c) {
    (void)ctx;
    c.ailments.paralysed = false;        // Unparalyse
    c.ailments.paralysisLevel = 0;
    c.ailments.paralyzedRounds = 0;
    c.ailments.poisonLevel = 0;          // CurePoison
    c.ailments.diseaseLevel = 0;         // CureDisease
    for (i32 r = 0; r < kResistKinds; ++r) {
        c.resistances.resist[ix(r)] = 0; // SpellResist[all] = 0
    }
}

// R47/R49 ReleaseFollowers.
// UNKNOWN: release semantics. Companions owned by ownerSlot are marked released and unowned.
void releaseFollowers(Ctx& ctx, Vec<Companion>& companions, i32 ownerSlot) {
    (void)ctx;
    for (Companion& comp : companions) {
        if (comp.occupied && comp.owner == ownerSlot) {
            comp.released = true;
            comp.owner = -1;
        }
    }
}

// R49 Sub0930Head: ReleaseFollowers, then EndEveryAilment (R45).
void sub0930Head(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot) {
    releaseFollowers(ctx, companions, slot);
    cityGateClear(ctx, party.members[ix(slot)]);
}

// R46 Kill, the death routine:
//   1. CurHits = 0; IsDead = true; Deaths++; LogDeath.
//   2. Sub0930 unless suppressed.
// The DeathCause is not stored on Character. It is accepted for the signature only.
// UNKNOWN: "Ordinary" death is written when the member is not already dead, so a rock
// state set beforehand is kept.
void killCharacter(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot,
                   DeathCause why, bool suppressSub0930) {
    (void)why;
    Character& who = party.members[ix(slot)];
    who.vitals.curHits = 0;
    if (!who.isDead()) {
        who.deathState = kDeathOrdinary;
    }
    who.deaths += 1;
    logDeath(ctx, who);
    if (!suppressSub0930) {
        sub0930(ctx, party, companions, slot);
    }
}

// R47 party death flow (party size > 1): SaveChar; PruneTheDead; refresh and redraw.
// No rescue offer.
void partyDeathFlow(Ctx& ctx, Party& party, i32 slot) {
    saveChar(ctx, party.members[ix(slot)]);
    pruneTheDead(ctx, party);
    ctx.say("ui.redraw", "Party display refreshed.", {}, {});
}

// R48 lone death flow (party size of 1): SaveChar; OfferRescue.
void loneDeathFlow(Ctx& ctx, Character& who) {
    saveChar(ctx, who);
    offerRescue(ctx, who);
}

// R49 Sub0930(who):
//   Sub0930Head; save(who)
//   if party.Size > 1: PruneTheDead; return
//   if !IsDead: SetDead(who)
//   SaveChar; OfferRescue
//   if IsDead && PendingRaise: Sub0930RaiseOffer(who)
void sub0930(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot) {
    sub0930Head(ctx, party, companions, slot);
    Character& who = party.members[ix(slot)];
    saveChar(ctx, who);
    if (party.size > 1) {
        pruneTheDead(ctx, party);
        return;
    }
    if (!who.isDead()) {
        setDead(ctx, who, kDeathOrdinary);
    }
    loneDeathFlow(ctx, who);
    if (who.isDead() && who.pendingRaise) {
        // Sub0930RaiseOffer: UNKNOWN in supplied text; emit the offer event.
        ctx.say("raise.offer", "Raise offered.", {}, {who.name});
    }
}

// R47 PruneTheDead: dead members leave the party list. Their bodies stay on the square
// (copied to morgueList) and are saved. Survivors are compacted in order.
void pruneTheDead(Ctx& ctx, Party& party) {
    (void)ctx;
    const i32 oldSize = party.size;
    i32 w = 0;
    for (i32 r = 0; r < oldSize; ++r) {
        if (party.members[ix(r)].isDead()) {
            party.morgueList.push_back(party.members[ix(r)]);
        } else {
            if (w != r) {
                party.members[ix(w)] = party.members[ix(r)];
            }
            ++w;
        }
    }
    for (i32 r = w; r < oldSize; ++r) {
        party.members[ix(r)] = Character{};
    }
    party.size = w;
    if (w == 0) {
        party.wiped = true;
    }
}

// SetDead: records the death state. Ordinary (Dead) and Stoned (2) are both accepted.
// Stone survives EndEveryAilment (R45).
void setDead(Ctx& ctx, Character& who, DeathState state) {
    (void)ctx;
    who.deathState = state;
}

// Outside combat death: DieOutsideCombat(why) = Kill(_ch, why).
void dieOutsideCombat(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot,
                      DeathCause why) {
    killCharacter(ctx, party, companions, slot, why, false);
}

// R52: Rescue.DaysUntilFound.
// UNKNOWN: constants not supplied. Simplest sensible choice: 1 day when easy, else 2.
i32 daysUntilFound(Ctx& ctx, i32 level, i32 area, bool easy) {
    // R18: days until a dead body is found. Mirrors Rescue.DaysUntilFound.
    if (level < 1) return 0;
    const double L = static_cast<double>(level);
    // Lain-there threshold: Log(level)^2 * 100 + 21 days (natural log).
    const double threshold = std::log(L) * std::log(L) * 100.0 + 21.0;
    i32 days = 0;
    for (;;) {
        // Ceiling is 10000 until lain there longer than the threshold, then 1000.
        double ceiling = static_cast<double>(days) < threshold ? 10000.0 : 1000.0;
        // Level 1, area 18 or lower: someone trips over the body almost at once.
        if (level == 1 && area <= 18) ceiling = 100.0;
        days++;

        // Daily find numerator 16 - level, floored at a tenth.
        double chance = 16.0 - L;
        if (chance < 0.1) chance = 0.1;
        // Easy-to-find square (Floor.RESCUE) adds 16 - level again. See ch15 R31 (CONFLICT note).
        if (easy) chance += 16.0 - L;

        if (ctx.rng.nextDouble() * ceiling <= chance) return days;
        // Reference guard: the original would spin here; nothing else can end the loop.
        if (days > 2000000) return days;
    }
}

// R52 WaitForRescue: days = DaysUntilFound; age increases by days; carried home to town.
// Quirk: a body saved with poison is cleaned on wait (c32).
// UNKNOWN: town area id is taken as -1 and the dungeon level as 0.
void waitForRescue(Ctx& ctx, Character& who, i32 level, i32 area, bool easy) {
    const i32 days = daysUntilFound(ctx, level, area, easy);
    who.ageDays += days;
    who.ailments.poisonLevel = 0;
    who.location.dungeonLevel = 0;
    who.location.area = -1;
    ctx.say("rescue.found", "Carried home to town.", {}, {who.name});
}

// R54 Rock death (caster in party).
//   k = caster slot; victims = members[k..end]
//   completes = (k <= 1); if completes, also members[0..k-1]
//   each victim: DeathRock; Kill; if not lost, record ROCK
//   if completes: log "You walked into solid rock!"; record ROCK
// Port deviation: a caster in slot 2 or later kills from the caster onward and stops.
// Sub0930 is suppressed per victim; the party is pruned once after the loop.
// UNKNOWN: DeathCause for rock is not a named value; DeathCause{} is passed.
RockDeathResult rockDeath(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 casterSlot) {
    RockDeathResult res;
    res.casterSlot = casterSlot;
    if (casterSlot < 0 || casterSlot >= party.size) {
        return res;
    }
    res.completes = (casterSlot <= 1);

    Vec<i32> victims;
    for (i32 i = casterSlot; i < party.size; ++i) {
        victims.push_back(i);
    }
    if (res.completes) {
        for (i32 i = 0; i < casterSlot; ++i) {
            victims.push_back(i);
        }
    }

    for (i32 idx : victims) {
        Character& m = party.members[ix(idx)];
        m.deathState = kDeathRock;  // CauseOfDeath = DeathRock
        killCharacter(ctx, party, companions, idx, DeathCause{}, true);
        res.killed += 1;
        if (!m.isLost()) {
            ctx.say("rock.record", "Rock recorded on automap.", {}, {m.name});
        }
    }

    if (res.completes) {
        ctx.say("rock.walked", "You walked into solid rock!", {}, {});
        ctx.say("rock.record", "Rock recorded on automap.", {}, {});
    }

    pruneTheDead(ctx, party);
    return res;
}

// ---------------------------------------------------------------------------
// Companion raise, drift and leave (R55-R57)
// ---------------------------------------------------------------------------

// R55 companion raise:
//   chance = min(98, 85 + monsterCon)   (defined in companions2a.h, not here)
//   failure destroys the companion outright
//   only Resurrect (22) restores full hits; other raises leave one hit
CompanionRaiseResult companionRaise(Ctx& ctx, Companion& comp, RaiseKind kind, i32 monsterCon) {
    const i32 chance = companionRaiseChance(monsterCon);
    if (!ctx.rng.percent(chance)) {
        comp.occupied = false;
        comp.isDead = true;
        comp.hits = 0;
        comp.pendingRaise = false;
        return CompanionRaiseResult::Destroyed;
    }
    comp.isDead = false;
    comp.deathState = CompanionDeathState::Normal;
    comp.pendingRaise = false;
    if (kind == RaiseKind::Resurrect) {
        comp.hits = comp.maxHits;
        return CompanionRaiseResult::RaisedFull;
    }
    comp.hits = 1;
    return CompanionRaiseResult::RaisedOneHit;
}

// R56 companion tick, per living companion:
//   1. binding below 100 rolls to drift one point toward zero.
//   2. a successful drift roll is followed by a leave roll.
//   3. binding zero always leaves.
//   4. opposed alignment forces a defection roll (75% Evil, 15% Neutral).
// Caller applies leave and defection to the world; this only reports them.
// UNKNOWN: "opposed" is taken as companion alignment differing from the owner's alignment.
// UNKNOWN: naturalCHA is taken as owner.abilities.stats[CHA].
CompanionTickResult companionTick(Ctx& ctx, Companion& comp, const Character& owner) {
    CompanionTickResult r;
    if (!comp.occupied || comp.isDead) {
        return r;
    }
    const i32 missing = comp.maxHits - comp.hits;

    if (comp.bindingLevel == 0) {
        r.left = true;  // R56.3
    } else if (comp.bindingLevel < kBindingMax) {
        const f64 drift = comp.bound
                              ? boundDrift(comp.guildLevel, owner.topGuildLevel, comp.bindingLevel)
                              : contentDrift(missing, comp.maxHits);
        r.drifted = driftCheck(ctx, drift);
        if (r.drifted) {
            if (comp.bindingLevel > 0) {
                comp.bindingLevel -= 1;
            } else {
                comp.bindingLevel += 1;
            }
            r.leaveRolled = true;
            const f64 leaveValue = companionLeaveValue(
                comp.guildLevel, owner.topGuildLevel, comp.bindingLevel,
                statOf(owner, StatIndex::Cha), missing, comp.maxHits);
            r.left = leaveCheck(ctx, leaveValue);
        }
    }

    const i32 ownerAlign = static_cast<i32>(owner.alignment) + 1;  // Align 0..2 -> CompanionAlignment 1..3
    const bool opposed = static_cast<i32>(comp.alignment) != ownerAlign;  // UNKNOWN: opposition rule
    if (opposed && opposedDefectionRoll(ctx, comp.alignment)) {
        r.defected = true;
        r.hostileGroup = true;
        ctx.say("companion.defect", "Companion turns against the party.", {}, {comp.name});
    }
    return r;
}

// R56 defection roll. 75% for Evil, 15% for Neutral, none for Good.
// UNKNOWN: whether these are chances or thresholds; percent() chance used.
bool opposedDefectionRoll(Ctx& ctx, CompanionAlignment alignment) {
    switch (alignment) {
        case CompanionAlignment::Evil:
            return ctx.rng.percent(kDefectChanceEvil);
        case CompanionAlignment::Neutral:
            return ctx.rng.percent(kDefectChanceNeutral);
        default:
            return false;
    }
}

// R57: drift check passes when U*200 < drift. U is taken as nextDouble() (range UNKNOWN).
bool driftCheck(Ctx& ctx, f64 drift) {
    return ctx.rng.nextDouble() * 200.0 < drift;
}

// R57: leave check passes when U*200 < leaveValue.
bool leaveCheck(Ctx& ctx, f64 leaveValue) {
    return ctx.rng.nextDouble() * 200.0 < leaveValue;
}

// R57 content drift: max(0.5, 15*missingHP/maxHP).
f64 contentDrift(i32 missingHits, i32 maxHits) {
    // R57 content drift: forwards to driftContentStep with hits = maxHits - missingHits.
    // driftContentStep returns max(0.5, 15 * missing / max) for maxHits > 0 and 0.5 for maxHits <= 0.
    return driftContentStep(maxHits - missingHits, maxHits);
}

// R57 bound drift:
//   max(1, 2*M/ln(3) - ownerHighestGuild*abs(binding/2)/ln(12))
// UNKNOWN: binding/2 is taken as real division.
f64 boundDrift(i32 monsterGuildLevel, i32 ownerTopGuildLevel, i32 binding) {
    const f64 m = static_cast<f64>(monsterGuildLevel);
    const f64 t = static_cast<f64>(ownerTopGuildLevel);
    const f64 half = std::fabs(static_cast<f64>(binding) / 2.0);
    const f64 v = 2.0 * m / std::log(3.0) - t * half / std::log(12.0);
    return std::max(kBoundDriftFloor, v);
}

// R57 leave value:
//   b = B(4.5*abs(binding))                       (B = bindingRound, mode UNKNOWN)
//   v = 24*ln(M+1) - 2b - 4*ln(ownerHighestGuild/10 + 1) - naturalCHA/2
//   leaveValue = clamp((v/2 + 15*missingHP/maxHP)/2, 0, 90)
// UNKNOWN: naturalCHA/2 is taken as real division.
f64 companionLeaveValue(i32 monsterGuildLevel, i32 ownerTopGuildLevel, i32 binding,
                        i32 naturalCha, i32 missingHits, i32 maxHits) {
    const f64 mLevel = static_cast<f64>(monsterGuildLevel);
    const f64 topLevel = static_cast<f64>(ownerTopGuildLevel);
    const f64 b = static_cast<f64>(bindingRound(4.5 * std::fabs(static_cast<f64>(binding))));
    const f64 v = 24.0 * std::log(mLevel + 1.0)
                  - 2.0 * b
                  - 4.0 * std::log(topLevel / 10.0 + 1.0)
                  - static_cast<f64>(naturalCha) / 2.0;
    const f64 missingTerm =
        maxHits > 0 ? 15.0 * static_cast<f64>(missingHits) / static_cast<f64>(maxHits) : 0.0;
    const f64 raw = (v / 2.0 + missingTerm) / 2.0;
    return std::min(std::max(raw, 0.0), static_cast<f64>(kChanceToLeaveMax));
}

// Rounding function B(x). UNKNOWN: rounding mode not stated.
// Simplest sensible choice: round half to even (CInt-like).
i64 bindingRound(f64 x) {
    return roundHalfEven(x);
}

}  // namespace mordor
