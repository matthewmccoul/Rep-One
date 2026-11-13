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

// R31 group-size rounding via engine CInt (audit's B rounding is UNKNOWN; argument form UNKNOWN [c7][c23])
i32 groupSizeCInt(f64 value);

// R32 attempts per group = min(living count, 5)
i32 attackAttemptsPerGroup(i32 livingCount);

// R45 member base hits: max(1, base Hits)
i32 memberBaseHits(i32 baseHits);

// R46 monster swing upper bound: Int(Hits * 0.5 / 2) + 1; bound not attained at every value
i32 monsterSwingBound(i32 hits);

// R50 lair placement roll (75%); whether area.lairId gates the roll is UNKNOWN [c7][c91]
bool rollLairPlacement(Ctx& ctx, const Area& area);

// Quick ref respawn (engine): 150 * level seconds
i64 respawnLevelSeconds(i32 level);

// Quick ref respawn area term: 5 seconds per empty area below header count
// UNKNOWN: operand meaning of headerCount/emptiedCount; full sub0A91 formula not supplied [c13][c24][c66]
i64 respawnEmptyAreaSeconds(i32 headerCount, i32 emptiedCount);

// Quick ref join chance threshold: max(0, 10*natCha - 10*(monsterFloor-1))
// UNKNOWN: natural vs effective CHA; LevelFound vs guild floor vs mLvl [c23][c25][c97]
i32 joinChanceThreshold(i32 naturalCha, i32 monsterFloor);

// Quick ref join roll: draws 1d3000 from ctx.rng; success when roll <= threshold
// CONFLICT: <= (audit, wiki) vs < (fidelity) [c23][c25][c97]; this uses <=
bool joinChanceRoll(Ctx& ctx, i32 naturalCha, i32 monsterFloor);

// Log(CHA) clamp to the engine minimum of 1 [c18]
// CONFLICT: audit gives no clamp [c25]
f64 clampLogCha(f64 logCha);

// Neutral-friendliness gate: lead's alignment is Neutral (engine "AlignmentValue==1" [c7])
// CONFLICT: whether a viewer-alignment check applies to non-good viewers is UNKNOWN [c5][c25]
bool neutralGateMet(const Character& lead);

// Companion spawn mode to table index mapping (audit [c24]): 4->4, 5->5, 6->4, >7 -> mode-7,
// mode 7 keeps window; returns kCompanionKeepWindow for window modes.
// CONFLICT: fidelity [c31] describes modes 0-2 as floor windows; mode 3 UNKNOWN.
i32 companionModeTableIndex(i32 spawnMode);

// Newbie guard: true when a monster of this guild level is excluded for the lead
// (guild level > kNewbieMonsterGuildCap). Trigger for the newbie state is UNKNOWN; caller checks it.
bool newbieGuardExcludes(const Character& lead, i32 monsterGuildLevel);

}  // namespace mordor
