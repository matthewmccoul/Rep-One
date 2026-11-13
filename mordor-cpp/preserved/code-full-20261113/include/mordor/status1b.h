#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"
#include "mordor/status2.h"

namespace mordor {

// ---- constants and tables ----

// R42 ambient sound chance per tick when nothing is in front
// UNKNOWN: behaviour when something is in front is not supplied
inline constexpr i32 kAmbientSoundPercent = 10;
// R41 the chest trap count (name list has eleven entries including "Not Trapped")
inline constexpr i32 kChestTrapCount = 10;
// R38 resistance index for poison (ResistPoison(5), Table B)
inline constexpr i32 kResistPoisonIndex = 5;
// R38 resistance index for disease (ResistDisease(4), Table B)
inline constexpr i32 kResistDiseaseIndex = 4;
// R56 opposed-alignment defection roll, Evil companion (percent)
// CONFLICT: the SRD does not say whether these are chances or thresholds; UNKNOWN
inline constexpr i32 kDefectChanceEvil = 75;
// R56 opposed-alignment defection roll, Neutral companion (percent)
inline constexpr i32 kDefectChanceNeutral = 15;
// R57 content drift minimum
inline constexpr f64 kContentDriftFloor = 0.5;
// R57 bound drift minimum
inline constexpr f64 kBoundDriftFloor = 1.0;
// R57 content drift scale: 15 * missingHP / maxHP
inline constexpr f64 kDriftHpScale = 15.0;

// R53 Table J: ordinary death value
// CONFLICT: Table J writes 0 for ordinary death; DeathState::Alive is 0 in state_common.h.
// Ordinary death is mapped to DeathState::Dead here.
inline constexpr DeathState kDeathOrdinary = DeathState::Dead;
// R53 Table J: stone (value 2), survives sub048C
inline constexpr DeathState kDeathStone = DeathState::Stoned;
// R53 Table J: rock death
// UNKNOWN: DeathRock value not supplied; DeathState::Rock (8) assumed from state_common.h
inline constexpr DeathState kDeathRock = DeathState::Rock;

// R59 cure spell levels (wiki table)
// Control (heading garbled; text says "will remove Poison")
inline constexpr i32 kCureControlLevel = 3;
// Cure Paralysis (tagged COMBAT)
// CONFLICT: target is the caster per c30/c34, "target-only" per wiki; resolved by audit, UNKNOWN here
inline constexpr i32 kCureParalysisLevel = 4;
// Cure Disease
inline constexpr i32 kCureDiseaseLevel = 5;
// Cure (heals hits, removes poison and disease)
inline constexpr i32 kCureLevel = 8;
// R59 classes for Control and Cure Paralysis (Pal/Vil/Mag/Hea)
inline constexpr Arr<Guild, 4> kControlClasses = {
    Guild::Paladin, Guild::Villain, Guild::Mage, Guild::Healer};
inline constexpr Arr<Guild, 4> kCureParalysisClasses = {
    Guild::Paladin, Guild::Villain, Guild::Mage, Guild::Healer};
// R59 classes for Cure Disease (Pal/Mag/Hea)
inline constexpr Arr<Guild, 3> kCureDiseaseClasses = {
    Guild::Paladin, Guild::Mage, Guild::Healer};
// R59 class for Cure (Hea)
inline constexpr Guild kCureClass = Guild::Healer;

// ---- result structs ----

// R42 result of one water or quicksand hit on a member
struct HazardResult {
    i32 hitsLost = 0;
    bool itemDestroyed = false;
    i32 destroyedSlot = -1;
};

// R54 result of a rock death
struct RockDeathResult {
    i32 casterSlot = -1;
    i32 killed = 0;
    bool completes = false;
};

// R56 result of one companion tick
struct CompanionTickResult {
    bool drifted = false;
    bool leaveRolled = false;
    bool left = false;
    bool defected = false;
    bool hostileGroup = false;
};

// R55 outcome of a companion raise attempt
enum class CompanionRaiseResult : i32 {
    Destroyed = 0,
    RaisedOneHit = 1,
    RaisedFull = 2
};

// ---- R38 to R41 chest traps ----

// R38 trap 1 Poison Gas: each member in order rolls ResistPoison(5);
// under half the resistance percentage, poisoned at party.dungeonLevel. No message.
void chestPoisonGasTrap(Ctx& ctx, Party& party);

// R38 trap 2 Disease Spores: each member in order rolls ResistDisease(4);
// under half the resistance percentage, diseased at party.dungeonLevel. No message.
void chestDiseaseSporesTrap(Ctx& ctx, Party& party);

// R39 trap 6 Fate: reroll current HP and SP between 1 and maximum
void chestFateTrap(Ctx& ctx, Character& opener);

// R40 trap 8 Blackout: clear every spell effect on the opener
void chestBlackoutTrap(Ctx& ctx, Character& opener);

// R38-R41 dispatch over Table G on the party opener.
// R41 other traps: UNKNOWN in this topic; dispatch does nothing for them.
void applyChestTrap(Ctx& ctx, Party& party, ChestTrap trap);

// ---- R42 to R44 four-second ailment tick helpers ----

// R42 ambient sound roll, made first in the tick; the leader must be alive.
// Returns true when the sound fires.
bool ambientSoundRoll(Ctx& ctx, bool nothingInFront);

// R42b water or quicksand hit on one member; no effect while levitating.
// Quicksand may destroy a random destructible pack item (R43).
HazardResult waterOrQuicksandHit(Ctx& ctx, Character& m, i32 hazardLevel, bool quicksand);

// R42f old-age check, using the member's ageDays and the race maximum age.
// UNKNOWN: formula for Rules.DiesOfOldAge is not supplied.
bool oldAgeCheck(Ctx& ctx, const Character& c, i32 raceMaxAgeDays);

// R43 quicksand destruction roll: Rnd(100) < (level/25)*100.
// UNKNOWN: whether the chance check is applied in the port (c45 shows none, c25 gives it).
// UNKNOWN: integer or real division for level/25.
bool quicksandDestroyRoll(Ctx& ctx, i32 hazardLevel);

// R44 first identification pass on one creature in front.
// Identification constants are UNKNOWN. Darkness (FOG) makes it harder.
// Invisible monsters need See Invisible.
bool identifyFirstPass(Ctx& ctx, const Character& ch, Creature& foe, bool dark);

// R44 second identification pass (rename) on one creature in front.
// Fog cuts the second-pass skill to three quarters.
bool identifySecondPass(Ctx& ctx, const Character& ch, Creature& foe, bool dark);

// R44 identification pass over each group in front of the party
void identificationPass(Ctx& ctx, const Character& ch, Encounter& front, bool dark);

// ---- R45 city gate and death clean-up ----

// R45 original sub048C: sub0D1D (mode 1), Ailments[0..6] = 0, spell resistances = 0, Recompute.
// Stone (DeathState::Stoned) is not cleared.
// CONFLICT: c2 EndEveryAilment calls Unparalyse (restores the stat part); c30/c34 say the gate
// clears ParalysisLevel without giving the stat part back. Port path UNKNOWN.
// UNKNOWN: sub0D1D body is not supplied.
void cityGateClear(Ctx& ctx, Character& c);

// R45 release companions owned by the party slot.
// UNKNOWN: Companion.owner is assumed to be the party slot index.
void releaseFollowers(Ctx& ctx, Vec<Companion>& companions, i32 ownerSlot);

// R45 Sub0930Head: ReleaseFollowers, then endEveryAilment (status2.h)
void sub0930Head(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot);

// ---- R46 to R51 death, rescue and raise ----

// R46 Kill: CurHits = 0, IsDead, Deaths++, log the death; then Sub0930 unless suppressed.
// Death paths in the party window and the single-character window follow R47 and R48.
void killCharacter(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot,
                   DeathCause why, bool suppressSub0930 = false);

// R47 party death flow (party size > 1): save, prune the dead, refresh and redraw. No rescue offer.
// Save and redraw are host side effects, not declared here.
void partyDeathFlow(Ctx& ctx, Party& party, i32 slot);

// R48 lone death flow (party size of 1): save, then the rescue offer.
// The rescue offer is a text screen and is out of scope; the core emits an event only.
void loneDeathFlow(Ctx& ctx, Character& who);

// R49 Sub0930: Sub0930Head, save, prune if the party is larger than 1, set dead,
// rescue offer, then a raise offer if the death is pending a raise.
// Port deviation: "UBound > 0" means a party death in the party window does not offer rescue.
void sub0930(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot);

// R49 PruneTheDead: removes dead members from the party
void pruneTheDead(Ctx& ctx, Party& party);

// R49 SetDead and R50 body-death step alone (chest trap, companion mishap, list UPDATE
// for a member found at nought, "has disappeared from view!"). Does not clear ailments.
void setDead(Ctx& ctx, Character& who, DeathState state);

// R49 DieOutsideCombat(why) = Kill(member, why)
void dieOutsideCombat(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot, DeathCause why);

// R51 death without a fight (poison, trap, old age): party leaves the body for raise,
// a lone character goes to the death screen. Covered by partyDeathFlow and loneDeathFlow.

// R52 days until found by the rescuer.
// UNKNOWN: constants for Rescue.DaysUntilFound are not supplied.
i32 daysUntilFound(Ctx& ctx, i32 level, i32 area, bool easy);

// R52 WaitForRescue: age increases by the days found; the character is carried home to town.
// Original quirk: a raised body keeps poison until the next city arrival or a cure.
void waitForRescue(Ctx& ctx, Character& who, i32 level, i32 area, bool easy);

// R54 rock death for the caster at casterSlot. Victims are the caster and all later members;
// earlier members are included only when the caster is in slot 0 or 1.
// CONFLICT: R54 sets CauseOfDeath = DeathRock; DeathCause in status2.h has no supplied rock member.
// Original quirk: caster in slot 2 or later hangs in the original; port kills from the caster
// onward and stops. Port deviation: a body in rock is written to the level-0 area, not carried.
RockDeathResult rockDeath(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 casterSlot);

// ---- R55 companion raise ----

// R55 raise success chance: min(kCompanionRaiseCap, kCompanionRaiseBase + monsterCon) (pure)
// UNKNOWN: "monster Con" beyond a single source is not defined.
i32 companionRaiseChance(i32 monsterCon);

// R55 companion raise attempt. Failure destroys the companion outright.
// Only Resurrect (22) restores full hits; other raises leave one hit.
// UNKNOWN: exact fields cleared when a companion is destroyed.
CompanionRaiseResult companionRaise(Ctx& ctx, Companion& comp, RaiseKind kind, i32 monsterCon);

// ---- R56 to R57 companion drift and leave ----

// R56 companion tick, per living companion: drift roll, leave roll, forced leave at binding 0,
// and opposed-alignment defection. A defection can create a hostile group using current HP.
// CONFLICT: defection percentages as chances or thresholds UNKNOWN.
// UNKNOWN: trigger for "companions leave no foes" (MONSMOD.BAS:1029-1033).
CompanionTickResult companionTick(Ctx& ctx, Companion& comp, const Character& owner);

// R56 opposed-alignment defection roll: 75% for Evil, 15% for Neutral.
// The caller decides when alignment is opposed.
bool opposedDefectionRoll(Ctx& ctx, CompanionAlignment alignment);

// R56 drift check: passes when U * 200 < drift.
// UNKNOWN: range of U is not supplied; nextDouble() assumed.
bool driftCheck(Ctx& ctx, f64 drift);

// R56 leave check: passes when U * 200 < leaveValue.
bool leaveCheck(Ctx& ctx, f64 leaveValue);

// R57 content drift: max(0.5, 15 * missingHP / maxHP) (pure)
f64 contentDrift(i32 missingHits, i32 maxHits);

// R57 bound drift: max(1, 2 * monsterGuildLevel / ln(3) - ownerTopGuildLevel * abs(binding / 2) / ln(12)) (pure)
// UNKNOWN: whether binding / 2 is integer or real division.
f64 boundDrift(i32 monsterGuildLevel, i32 ownerTopGuildLevel, i32 binding);

// R57 leave value: clamp((v / 2 + 15 * missingHP / maxHP) / 2, 0, kChanceToLeaveMax) (pure)
// v = 24 * ln(M + 1) - 2 * B(4.5 * abs(binding)) - 4 * ln(ownerTopGuildLevel / 10 + 1) - naturalCha / 2
// naturalCha is the owner's natural CHA (stats without mods).
f64 companionLeaveValue(i32 monsterGuildLevel, i32 ownerTopGuildLevel, i32 binding,
                        i32 naturalCha, i32 missingHits, i32 maxHits);

// R57 B(x) rounding function (pure)
// UNKNOWN: rounding mode of B() is not stated; roundHalfEven assumed.
i64 bindingRound(f64 x);

// ---- R58 to R60 cures and death spells ----

// R58 cure routines are in R10; disease halving and town rest cures are in R11. Not declared here.

// R59 cure spell table: constants above (Control L3, Cure Paralysis L4, Cure Disease L5, Cure L8).
// Cure Paralysis target: CONFLICT (see kCureParalysisLevel).

// R60 death-effect spells (archive): Cause Wounds, Cause Heavy Wounds, Power, Harm,
// Cause Fatal Wounds, Word of Death kill; Draining Touch drains (leaves a lifeless shell).
// Spell ids are not supplied here; see DeathSpellEffect in status2.h.

}  // namespace mordor
