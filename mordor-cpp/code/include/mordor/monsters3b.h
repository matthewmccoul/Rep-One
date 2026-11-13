#pragma once
// monsters3b: SRD part c11p3 (combat and attacks, member hits and group state,
// lairs and placement, respawn quick reference, join and companion quick reference).
// Most rules in this part are UNKNOWN or CONFLICT. Routines are declared only where
// the text gives a formula or an engine-preferred behaviour.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"

namespace mordor {

// ---- constants (from the SRD text)

// R32 attack attempts per group: min(living count, 5)
inline constexpr i32 kAttackAttemptCap = 5;

// R46 monster swing bound: most = Int(Hits * 0.5 / 2) + 1 (audit [c66])
inline constexpr f64 kSwingBoundFactor = 0.5;
inline constexpr f64 kSwingBoundDivisor = 2.0;
inline constexpr i32 kSwingBoundBonus = 1;

// R45 member base hits floor: max(1, base Hits) (engine [c7])
// CONFLICT: engine says "base figure" [c5]; whether base 0 becomes 1 is UNKNOWN.
inline constexpr i32 kMemberBaseHitsFloor = 1;


// Quick ref respawn area term: 5 seconds per empty area below header count (engine [c9][c13][c31])
// UNKNOWN: full sub0A91 respawn formula; test form 5 * (total - tally) [c66] conflicts.
inline constexpr i64 kRespawnSecondsPerArea = 5;

// Quick ref join chance: 1d3000 <= max(0, 10*natural CHA - 10*(floor-1)) (audit [c23])
// CONFLICT: wiki [c97] uses rand(1,3000) <= CHA*10-(mLvl-1)*10; fidelity [c25] uses <.
inline constexpr i32 kJoinChancePerCha = 10;
inline constexpr i32 kJoinChancePerFloor = 10;
inline constexpr i32 kJoinChanceRollMax = 3000;

// R50 lair placement roll through the area's LairId: 75% (engine [c7])
inline constexpr i32 kLairPlacePercent = 75;

// Log(CHA) clamp minimum (engine [c18]). CONFLICT: audit [c25] gives no clamp.
inline constexpr f64 kLogChaFloor = 1.0;


// Newbie guard: monsters with guild level above this are excluded for the lead (engine [c7][c24][c31][c34]).
// CONFLICT: test [c66] applies it generally; engine applies it to the lead only (engine prevails).
// UNKNOWN: the trigger condition that makes a character a newbie.
inline constexpr i32 kNewbieMonsterGuildCap = 2;

// Companion spawn mode result meaning "keep the floor window" (fidelity [c31], audit [c24]).
inline constexpr i32 kCompanionKeepWindow = -1;

// NOTE: The engage-chance table (Good vs Neutral 15; Good vs Evil 100; Evil vs Evil 25;
// Evil vs other 35 [c18][c25]) is covered by hostileBase(...) and hostileRoll(...) in clock.h
// (API INDEX). Not redeclared here.
// CONFLICT: wiki [c97] alignment gate vs engine table; engine prevails (see clock.h).

// ---- declarations, grouped by rule number














}  // namespace mordor
