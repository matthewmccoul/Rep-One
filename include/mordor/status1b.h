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






// ---- R42 to R44 four-second ailment tick helpers ----








// ---- R45 city gate and death clean-up ----




// ---- R46 to R51 death, rescue and raise ----





// R49 PruneTheDead: removes dead members from the party
void pruneTheDead(Ctx& ctx, Party& party);



// R51 death without a fight (poison, trap, old age): party leaves the body for raise,
// a lone character goes to the death screen. Covered by partyDeathFlow and loneDeathFlow.




// ---- R55 companion raise ----



// ---- R56 to R57 companion drift and leave ----









// ---- R58 to R60 cures and death spells ----

// R58 cure routines are in R10; disease halving and town rest cures are in R11. Not declared here.

// R59 cure spell table: constants above (Control L3, Cure Paralysis L4, Cure Disease L5, Cure L8).
// Cure Paralysis target: CONFLICT (see kCureParalysisLevel).

// R60 death-effect spells (archive): Cause Wounds, Cause Heavy Wounds, Power, Harm,
// Cause Fatal Wounds, Word of Death kill; Draining Touch drains (leaves a lifeless shell).
// Spell ids are not supplied here; see DeathSpellEffect in status2.h.

}  // namespace mordor
