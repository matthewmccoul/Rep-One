#pragma once
#include "mordor/combat3.h"

// combat2a: SRD part c07p2 (T07), chest traps and player crit/backstab procs.
// NOTE: The supplied SRD text ends at R10. Later T07 rules (Asmodeus fight end,
// quest settlement, combat XP, monster spells) are not declared here.
// CONFLICT: Rule numbers R1-R10 in this section are reused by the quick reference
// for other rules (R7 swings, R10 extra swings). Citations here are T07-local.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants ----
inline constexpr i32 kChestTrapMax = 10;            // R2 highest trap number (ChestTrap::Trap10)
inline constexpr i32 kChestTrapWholePartyMax = 4;   // R2 traps 1..4 affect the whole party
inline constexpr i32 kCritModBonus = 5;             // R10 character crit adds +5 to DamMod
inline constexpr i32 kBackstabModBonus = 2;         // R10 character backstab adds +2 to DamMod
inline constexpr i32 kCritChanceScale = 100;        // R7 proc roll is drawn from 0..99

// ---- types ----

// R2 scope of a chest trap.
enum class TrapScope : i32 {
    None = 0,        // trap 0 (Not Trapped): no effect
    WholeParty = 1,  // traps 1..4: every party member
    OpenerOnly = 2   // traps 5..10: the opener only
};

// R6-R8 outcome of the two independent procs rolled on one blow.
struct BlowProcs {
    bool crit = false;      // R7 crit proc succeeded
    bool backstab = false;  // R8 backstab proc succeeded
};

// ---- R1 chest trap selection ----

// R1 trap number chosen when the opener triggers a chest (monOpenChest).
// UNKNOWN: selection method is not in the supplied material. Implementer uses the
// simplest sensible draw, ctx.rng.below(kChestTrapMax + 1), and flags it.
ChestTrap rollChestTrap(Ctx& ctx);

// ---- R2 trap scope ----


// ---- R3 trap damage ----


// ---- R4-R5 applying trap damage ----



// ---- P1 chest trap resolution ----

// P1 resolve a chest trap after the opener (party.opener) triggers it.
// Applies R2 scope, R3 damage to affected members, R4-R5 hit updates, and emits trap text.
// UNKNOWN: effects of traps 6-10 are not in the supplied material. Trap 5 names the destroyed
// item, whose source (opener's pack or chest) is UNKNOWN. Trap 9 mapping is unconfirmed.
// Only trap 5 and trap 9 print text in the original.
void resolveChestTrap(Ctx& ctx, Party& party, ChestTrap trap);
// Traps 1 and 2: derived-resistance test for every member; returns a bit mask of the afflicted members.
u32 chestAfflictParty(Ctx& ctx, Party& party, DamageKind kind);

// ---- R6-R8 player crit and backstab chances ----

// R7 player crit chance in percent for one blow: guild-level term plus optional weapon term,
// clamped to a floor and a cap.
// UNKNOWN: floor and cap values and exact meaning and type of each parameter (Rules.CritChance).
f64 critChance(i32 critLevel, i32 critMod, i32 targetGuildLevel, i32 weaponFloor, i32 weaponCrit);

// R8 player backstab chance in percent for one blow; same shape as critChance.
// UNKNOWN: floor and cap values and exact parameter meanings (Rules.BackstabChance).
f64 backstabChance(i32 backLevel, i32 backMod, i32 targetGuildLevel, i32 weaponFloor, i32 weaponBackstab);

// R7-R8 one proc roll: true when the draw is below the chance (percent). Consumes one rng draw.
bool rollProc(Ctx& ctx, f64 chancePercent);

// ---- R6-R9 blow procs ----

// R6-R9 roll crit (R7) then backstab (R8) for one blow. Both are rolled before the hit gate;
// the caller runs this before the gate. Neither proc guarantees damage.
// UNKNOWN: the hit gate rule is not in the supplied material (R9).
BlowProcs rollBlowProcs(Ctx& ctx, f64 critChancePercent, f64 backstabChancePercent);

// ---- R10 modifier increments ----

// R10 DamMod increment from the procs: +5 for crit, +2 for backstab, summed when both succeed.
// Monster bonuses (+2 crit, +1 backstab) are not handled here.
i32 blowModifierBonus(const BlowProcs& procs);


}  // namespace mordor
