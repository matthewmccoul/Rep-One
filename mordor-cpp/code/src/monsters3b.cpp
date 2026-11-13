// monsters3b.cpp
// Implements the functions declared in include/mordor/monsters3b.h.
// Rule numbers refer to the SRD part c11p3 (R31-R70).

#include "mordor/monsters3b.h"

#include "mordor/math_util.h"

namespace mordor {

// R31: the engine uses CInt for group-size rounding. VB CInt rounds half to even.
// The audit's B(U*InGroup/2) rounding is UNKNOWN and is not used.
i32 groupSizeCInt(f64 value) {
    return static_cast<i32>(roundHalfEven(value));
}

// R32: attempts per group = min(living count, 5).
// UNKNOWN: whether the wiki's "up to 5 attack attempts" is the same cap at a different scope.
i32 attackAttemptsPerGroup(i32 livingCount) {
    return minv(livingCount, kAttackAttemptCap);
}

// R45: the engine uses max(1, base Hits).
// UNKNOWN: whether "base figure" (c5) differs from this when the base is 0.
i32 memberBaseHits(i32 baseHits) {
    return maxv(kMemberBaseHitsFloor, baseHits);
}

// R46: most = Int(Hits * 0.5 / 2) + 1 (audit bound, c66).
// The bound is not attained at every value; the exact maximum is UNKNOWN.
i32 monsterSwingBound(i32 hits) {
    const f64 scaled = static_cast<f64>(hits) * kSwingBoundFactor / kSwingBoundDivisor;
    return static_cast<i32>(vbInt(scaled)) + kSwingBoundBonus;
}

// R50: a lair is placed through the area's LairId with a 75% roll (c7).
// UNKNOWN: whether the roll is made only when LairId is nonzero. This implementation
// makes no roll for an area without a LairId, so the RNG is not consumed in that case.
bool rollLairPlacement(Ctx& ctx, const Area& area) {
    if (area.lairId == 0) {
        return false;
    }
    return ctx.rng.percent(kLairPlacePercent);
}

// R?? respawn (c9, c31): 5 s per level-pair, evaluated as (5*(level/2.0))*60.
// CONFLICT: the text says "150 s per level pair", but the formula evaluates to
// 150 * level. The engine formula is used here, so the result is 150 * level seconds.
i64 respawnLevelSeconds(i32 level) {
    const f64 half = static_cast<f64>(level) / 2.0;
    return vbInt(static_cast<f64>(kRespawnSecondsPerArea) * half * 60.0);
}

// R?? respawn (c9, c13, c31): subtract 5 s per empty area below the header count.
// CONFLICT: the test set (c66) subtracts 5 * (total - tally) instead. The engine prevails.
// UNKNOWN: the sign convention for the return value. It is returned as a non-positive
// adjustment to be added to the respawn time. The clamp at zero for emptiedCount above
// headerCount is an assumption ("below the header count").
// UNKNOWN: the full sub0A91 respawn formula (only the 5-second term is given).
i64 respawnEmptyAreaSeconds(i32 headerCount, i32 emptiedCount) {
    i32 below = headerCount - emptiedCount;
    if (below < 0) {
        below = 0;
    }
    return -static_cast<i64>(kRespawnSecondsPerArea) * static_cast<i64>(below);
}

// R?? join chance (c23 audit): threshold = max(0, 10*naturalCha - 10*(floor-1)).
// CONFLICT: the wiki (c97) and fidelity (c25) forms differ from this in comparison
// (<= vs <), clamp, natural vs effective CHA, and LevelFound vs guild floor vs mLvl.
// The audit form is used here. UNKNOWN which is exact.
i32 joinChanceThreshold(i32 naturalCha, i32 monsterFloor) {
    const i32 raw = kJoinChancePerCha * naturalCha - kJoinChancePerFloor * (monsterFloor - 1);
    return maxv(0, raw);
}


// R?? engagement (c18, c7): the engine clamps Log(CHA) to a minimum of 1.
// CONFLICT: the audit (c25) gives no clamp. The engine clamp is used. UNKNOWN which applies.
f64 clampLogCha(f64 logCha) {
    // R22: engine clamps Log(CHA) to a minimum of 1 [c18].
    // UNKNOWN: audit gives no clamp [c25]; engine clamp used.
    // Explicit comparison: maxv() did not return the larger argument (2.5 -> not 2.5).
    if (logCha < kLogChaFloor) {
        return kLogChaFloor;
    }
    return logCha;
}

// R?? neutral gate (c7): the engine uses elif lead.AlignmentValue == 1.
// CONFLICT: the 40% gate and any viewer-alignment check are not given in the header's
// signature. The engine's check on the lead's alignment only is implemented.
// UNKNOWN which applies to non-good viewers (c5, c25).
bool neutralGateMet(const Character& lead) {
    return lead.alignment == Align::Neutral;
}

// R?? companion spawn mode (c24 audit, c31 fidelity).
// Audit: mode 4 -> 4, mode 5 -> 5, mode 6 -> 4, mode > 7 -> mode - 7, mode 7 keeps the window.
// CONFLICT: fidelity (c31) says modes 0-2 are floor windows and modes 4+ name a table index.
// The audit is more specific and is used here.
// UNKNOWN: the return value for modes 0-3 (not given in the audit). kCompanionKeepWindow
// is returned for them and for mode 7.
i32 companionModeTableIndex(i32 spawnMode) {
    if (spawnMode == 7) {
        return kCompanionKeepWindow;
    }
    if (spawnMode > 7) {
        return spawnMode - 7;
    }
    if (spawnMode == 6) {
        return 4;
    }
    if (spawnMode == 4 || spawnMode == 5) {
        return spawnMode;
    }
    // UNKNOWN: modes 0-3 (see above).
    return kCompanionKeepWindow;
}

// R?? newbie guard (c7, c24, c31, c34): the engine applies the guard to the lead only.
// A monster with guild level above kNewbieMonsterGuildCap is excluded for a newbie lead.
// CONFLICT: the test (c66) excludes GL > 2 in general. The engine's lead-only scope is used.
// UNKNOWN: the definition of "newbie". This implementation uses lead.level <= 1.
bool newbieGuardExcludes(const Character& lead, i32 monsterGuildLevel) {
    const bool newbie = lead.level <= 1;
    return newbie && monsterGuildLevel > kNewbieMonsterGuildCap;
}

}  // namespace mordor
