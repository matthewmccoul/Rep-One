// monsters2b.cpp
// Implements the functions declared in include/mordor/monsters2b.h.
// Rule numbers from the SRD part c11p2 are cited by their source chunk where the
// text gives no R-number. Items marked UNKNOWN or CONFLICT are guesses or
// unresolved SRD statements; see each comment.

#include "mordor/monsters2b.h"

#include "mordor/game_data.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

// ---- Rolls ----------------------------------------------------------------

// Any-class roll (1%): Int(Rnd*100)+1 < 2 [c7][c24].
bool rollAnyClass(Ctx& ctx) {
    const i32 roll = static_cast<i32>(ctx.rng.below(kAnyClassRollSides)) + 1;
    // roll < 2 is the same as roll <= kAnyClassRollHitBelow (= 1).
    return roll <= kAnyClassRollHitBelow;
}

// Lair named monster drawn 75% of the time [c7][c24].
bool rollLairDraw(Ctx& ctx) {
    return ctx.rng.percent(kLairDrawPercent);
}

// Neutral lead peaced: 40% [c7][c24].
bool rollNeutralLeadPeaced(Ctx& ctx) {
    constexpr i32 kNeutralLeadPeacedPercent = 40;
    return ctx.rng.percent(kNeutralLeadPeacedPercent);
}

// Quest class head start: 40% [c19].
bool rollQuestClassHeadStart(Ctx& ctx) {
    constexpr i32 kQuestClassHeadStartPercent = 40;
    return ctx.rng.percent(kQuestClassHeadStartPercent);
}

// ---- Encounter window -----------------------------------------------------

// m0282: target floor of the encounter window: level, +1 on STUD [c7].
i32 encounterTargetFloor(i32 dungeonLevel, bool onStud) {
    return dungeonLevel + (onStud ? 1 : 0);
}

// Spread: widening of the LevelFound window when nothing is found [c7].
// Each widening adds kSpreadStep.
void widenEncounterSpread(Ctx& ctx, Encounter& encounter) {
    encounter.spread += kSpreadStep;
    ctx.say("monsters2b.spread_widened", "Encounter spread widened",
            {static_cast<i64>(encounter.spread)}, {});
}

// Level-gap gate: 90 / 50 / 10 [c19].
// UNKNOWN: the SRD part gives the three gate values but not which level gap
// selects which value. Assumed: gap <= 0 -> 90, gap <= 2 -> 50, otherwise 10.
i32 levelGapGatePercent(i32 levelGap) {
    if (levelGap <= 0) {
        return kLevelGapGateHigh;
    }
    if (levelGap <= 2) {
        return kLevelGapGateMid;
    }
    return kLevelGapGateLow;
}

// JoinOffer is read only from group 0 [c25][c42].
bool readJoinOffer(const Encounter& encounter) {
    return encounter.groups[0].joinOffer;
}

// Attempts per group: min(living count, 5) [c23].
i32 attemptsPerGroup(i32 livingCount) {
    return minv(livingCount, kAttemptsPerGroupMax);
}

// AtNought: a member at 0 hits that still counts as alive until its hits are
// written [c5].
bool countsAsAlive(const Creature& creature) {
    return creature.alive || creature.atNought;
}

// Character state that ends a monster's blow sequence [c56].
// UNKNOWN: the SRD part does not list which states end the sequence.
// Assumed: any state other than Alive.
bool endsBlowSequence(DeathState state) {
    return state != DeathState::Alive;
}

// ---- Companions -----------------------------------------------------------

// Mishap: companion effect that kills a lone character mid-round [c56].
// UNKNOWN: the trigger and whether the kill is certain are not stated in the SRD
// part. Assumed: when the party is solo and the leader is alive, the leader dies.
void companionMishap(Ctx& ctx, Party& party, const Companion& companion) {
    // Reference CompanionMishap.Victim: the victim is drawn as CInt(Rnd * (members - 1)) + 1 and a draw that
    // picks the owner means no mishap. For a party of one the draw is always 1, the owner, so a lone
    // character is never the victim. The earlier version killed the lone character unconditionally, which
    // the reference does not do. The real mishap path is rollMishap (companions2a.cpp).
    (void)ctx;
    (void)party;
    (void)companion;
}

// Charm: a spell that can force a monster to join if its damage exceeds the
// remaining HP [c97].
// CONFLICT: this exact signature (i32, i32) is also declared in companions2b.h,
// and companions1a.h declares an i64 overload. This file provides the single
// definition for the i32 signature; companions2b.h must not define it again.
bool charmForcesJoin(i32 damage, i32 remainingHits) {
    return damage > remainingHits;
}

// Fear (trap 9) releases non-ForLife companions with hits > 0 [c4].
bool fearReleases(const Companion& companion) {
    return companion.hits > 0 && !companion.forLife;
}

// Newbie (protection): level-1 characters in areas below 18 [c7].
// UNKNOWN: whether areaId 0 (none, per the term "Area id 0 means none") is
// protected is not stated. Assumed: the plain comparison applies, so 0 is protected.
bool newbieProtected(const Character& ch, i32 areaId) {
    return ch.level == 1 && areaId < kNewbieAreaLimit;
}

// Lair: a monster whose Id is an area's LairId [c7][c61].
// UNKNOWN: a LairId of 0 is assumed to mean no lair (area id 0 means none [c13]).
bool isLairMonster(const Area& area, const Creature& creature) {
    return area.lairId != 0 && creature.id == area.lairId;
}

// Asmodeus is identified by Id only: Monster Id 277 [c1].
bool isAsmodeus(const Creature& creature) {
    constexpr i32 kAsmodeusMonsterId = 277;
    return creature.id == kAsmodeusMonsterId;
}

// SpawnMask bit n = kind n [c13].
// UNKNOWN: MonsterKind is not declared in any header given to this module. It is
// assumed to be an enum whose values are 0..kMonsterKindCount-1 (Humanoid=0 ..
// Indigini=17) and is provided by a foundation header.
// FIXED: MonsterKind exists in no header, so the parameter uses i32 (the closest
// existing plain type). The kind value is the same bit index as before.
bool spawnMaskAllowsKind(i32 spawnMask, i32 kind) {
    const i32 bit = static_cast<i32>(kind);
    if (bit < 0 || bit >= kMonsterKindCount) {
        return false;
    }
    return (spawnMask & (1 << bit)) != 0;
}

// Trap table floor value may encode a floor bitmask [c100].
// UNKNOWN: the encoding is unverified. Assumed: bit (level-1) set means the
// monster is allowed on that dungeon level. Kobold 31 -> floors 1-5 and
// Phoenix 64 -> floor 7 are consistent with this.
bool floorMaskAllows(i32 floorMask, i32 dungeonLevel) {
    if (dungeonLevel < 1 || dungeonLevel > kDungeonFloorCount) {
        return false;
    }
    return ((floorMask >> (dungeonLevel - 1)) & 1) != 0;
}

// Pariah: mythical monster spawned only in levels 12..15 [c106].
bool pariahLevelEligible(i32 dungeonLevel) {
    return dungeonLevel >= kPariahMinLevel && dungeonLevel <= kPariahMaxLevel;
}

// Companion breath chance [c6]: base 50; 80 when the lead is more than a quarter
// stronger; 20 when more than a quarter weaker.
// UNKNOWN: the reference value for "a quarter" is not stated. Assumed: the
// companion's strength is the reference.
i32 companionBreathChance(i32 leadStrength, i32 companionStrength) {
    const f64 lead = static_cast<f64>(leadStrength);
    const f64 comp = static_cast<f64>(companionStrength);
    if (lead > comp * (1.0 + kBreathStrengthBand)) {
        return kBreathChanceStronger;
    }
    if (lead < comp * (1.0 - kBreathStrengthBand)) {
        return kBreathChanceWeaker;
    }
    return kBreathChanceBase;
}

// Rolls the companion breath chance [c6].
bool companionBreathRoll(Ctx& ctx, i32 leadStrength, i32 companionStrength) {
    return ctx.rng.percent(companionBreathChance(leadStrength, companionStrength));
}

// Crit / backstab chance from raw value and threshold band [c23].
// UNKNOWN: the SRD part gives k, lo and hi but not the formula. Assumed:
// chance = raw * 100 / k, clamped to [lo, hi].
i32 specialHitChance(SpecialHitKind kind, i32 raw) {
    const ThresholdBand& band =
        (kind == SpecialHitKind::Crit) ? kCritBand : kBackstabBand;
    const i64 scaled =
        vbInt(static_cast<f64>(raw) * 100.0 / static_cast<f64>(band.k));
    return static_cast<i32>(clampv(scaled, static_cast<i64>(band.lo),
                                   static_cast<i64>(band.hi)));
}

// Rolls the special hit chance [c23].
bool rollSpecialHit(Ctx& ctx, SpecialHitKind kind, i32 raw) {
    return ctx.rng.percent(specialHitChance(kind, raw));
}

// Companion spawn mode table [c24][c31].
CompanionSpawnWindow companionSpawnWindow(i32 mode) {
    CompanionSpawnWindow w;
    if (mode == 0) {
        w.kind = SpawnWindowKind::FloorOffsets;
        w.offsetLo = -3;
        w.offsetHi = -1;
    } else if (mode == kSpawnModeNativeWindowWide) {
        w.kind = SpawnWindowKind::FloorOffsets;
        w.offsetLo = -3;
        w.offsetHi = 0;
    } else if (mode == kSpawnModeNativeWindowNarrow) {
        w.kind = SpawnWindowKind::FloorOffsets;
        w.offsetLo = 0;
        w.offsetHi = 0;
    } else if (mode == kSpawnModeRepeat) {
        w.kind = SpawnWindowKind::RepeatPrevious;
    } else if (mode == kSpawnModeTableFour) {
        w.kind = SpawnWindowKind::TableIndex;
        w.tableIndex = 4;
    } else if (mode == kSpawnModeTableFive) {
        w.kind = SpawnWindowKind::TableIndex;
        w.tableIndex = 5;
    } else if (mode == kSpawnModeTableSix) {
        // CONFLICT: the table gives mode 6 -> table index 4, not 6 or 6-7.
        // This is the documented quirk and is kept as written.
        w.kind = SpawnWindowKind::TableIndex;
        w.tableIndex = 4;
    } else if (mode == kSpawnModeKeep) {
        w.kind = SpawnWindowKind::KeepPrevious;
    } else if (mode > kSpawnModeKeep) {
        w.kind = SpawnWindowKind::TableIndex;
        w.tableIndex = mode - kSpawnModeTableOffset;
    } else {
        // UNKNOWN: negative modes are not in the table. Assumed: keep previous.
        w.kind = SpawnWindowKind::KeepPrevious;
    }
    return w;
}

// Picks a companion monster index.
// UNKNOWN: the selection procedure is only partly given. Assumed:
//   RepeatPrevious / KeepPrevious -> previousMonsterIndex
//   TableIndex                    -> window.tableIndex
//   FloorOffsets                  -> weighted cumulative pick by chance over
//                                    monsters whose levelFound lies in
//                                    [nativeFloor+offsetLo, nativeFloor+offsetHi];
//                                    if no candidate, previousMonsterIndex.
// Field names levelFound and chance on MonsterDef are assumed from the wiki
// column names [c85].
i32 pickCompanionMonster(Ctx& ctx, const CompanionSpawnWindow& window,
                         i32 nativeFloor, i32 previousMonsterIndex) {
    if (window.kind == SpawnWindowKind::RepeatPrevious ||
        window.kind == SpawnWindowKind::KeepPrevious) {
        return previousMonsterIndex;
    }
    if (window.kind == SpawnWindowKind::TableIndex) {
        return window.tableIndex;
    }

    const i32 lo = nativeFloor + window.offsetLo;
    const i32 hi = nativeFloor + window.offsetHi;
    const i32 count = static_cast<i32>(ctx.data.monsters.size());

    i64 total = 0;
    for (i32 i = 0; i < count; ++i) {
        const MonsterDef& m = ctx.data.monsters[static_cast<u32>(i)];
        if (m.levelFound < lo || m.levelFound > hi || m.chance <= 0) {
            continue;
        }
        total += m.chance;
    }
    if (total <= 0) {
        return previousMonsterIndex;
    }

    const i64 r = static_cast<i64>(ctx.rng.below(static_cast<i32>(total)));
    i64 cum = 0;
    for (i32 i = 0; i < count; ++i) {
        const MonsterDef& m = ctx.data.monsters[static_cast<u32>(i)];
        if (m.levelFound < lo || m.levelFound > hi || m.chance <= 0) {
            continue;
        }
        cum += m.chance;
        if (r < cum) {
            return i;
        }
    }
    return previousMonsterIndex;
}

// ---- Respawn and rooms ----------------------------------------------------

// Respawn delay t [c9][c24][c31][c66]:
//   base 1200 s + 150 per dungeon level - 5 s per room of shortfall,
//   floored at 120 s.
// Checked against the test values: L1/0 -> 1350, L1/10 -> 1300, L15/0 -> 3450.
i64 respawnDelaySeconds(i32 dungeonLevel, i32 roomShortfall) {
    constexpr i64 kRespawnBaseSeconds = 1200;
    constexpr i64 kRespawnPerLevelSeconds = 150;
    const i64 delay = kRespawnBaseSeconds +
                      kRespawnPerLevelSeconds * static_cast<i64>(dungeonLevel) -
                      kRespawnPerShortfallSeconds * static_cast<i64>(roomShortfall);
    return maxv(delay, kRespawnFloorSeconds);
}

// Stale standing group: older than 2t [c9][c66].
bool standingGroupStale(i64 nowSeconds, i64 stampSeconds, i64 respawnT) {
    return (nowSeconds - stampSeconds) > kStaleStandingGroupFactor * respawnT;
}

// Tally is seeded from the header area count [c9][c24][c66].
// UNKNOWN: roomTotal is also seeded from the header area count so that the
// shortfall (total - tally) used in markAreaEmptied is defined.
void seedFloorTally(Ctx& ctx, FloorState& floor, i32 headerAreaCount) {
    (void)ctx;
    floor.tally = headerAreaCount;
    floor.roomTotal = headerAreaCount;
}

// Marks an area cleared: stamp = seconds at clearing, respawn t from the
// shortfall [c9][c66].
// UNKNOWN: emptiedAt is assumed to hold the tally after the increment. The
// shortfall is assumed to be roomTotal - tally, floored at 0.
void markAreaEmptied(Ctx& ctx, FloorState& floor, Area& area, i64 nowSeconds) {
    floor.tally += 1;
    area.emptiedAt = floor.tally;
    area.stamp = nowSeconds;
    const i32 shortfall = maxv(0, floor.roomTotal - floor.tally);
    area.respawnT = static_cast<i32>(
        respawnDelaySeconds(floor.dungeonLevel, shortfall));
    ctx.say("monsters2b.area_emptied", "Area cleared; respawn delay set",
            {static_cast<i64>(area.respawnT), static_cast<i64>(floor.tally)}, {});
}

// ---- Library ---------------------------------------------------------------

// Library knowledge states [c96]: 0 Unknown, 1 Seen, 2 Defeated, 3+ Fully Known.
// Values below 0 are treated as Unknown.
LibraryKnowledge libraryKnowledgeFor(i32 stateValue) {
    if (stateValue <= 0) {
        return LibraryKnowledge::UNKNOWN_STATE;
    }
    if (stateValue == 1) {
        return LibraryKnowledge::SEEN;
    }
    if (stateValue == 2) {
        return LibraryKnowledge::DEFEATED;
    }
    return LibraryKnowledge::FULLY_KNOWN;
}

}  // namespace mordor
