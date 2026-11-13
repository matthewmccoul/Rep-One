#include "mordor/monsters2a.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

// [c9][c24][c66] Respawn terms: base 1200 s, 150 s per dungeon level (test set: per floor).
constexpr i32 kRespawnBaseSeconds = 1200;
constexpr i32 kRespawnPerLevelSeconds = 150;
// [c9][c66] Minimum respawn delay, seconds.
constexpr i32 kRespawnFloorSeconds = 120;
// [c19] Quest: class head start, 40%.
constexpr i32 kQuestHeadStartPercent = 40;
constexpr i64 kMsPerSecond = 1000;

}  // namespace

// [c6][c13] Kind-to-class-index mapping is assumed to be a numeric match
// with ClassCount 18.
// CONFLICT: the mapping is not confirmed; the wiki/trap table label for
// value 10 is "Water-Dweller" while the SRD kind table says "Aquatic".
// Numeric match is used.
i32 classIndexOfKind(MonsterKind kind) {
    return static_cast<i32>(kind);
}

// [c13] SpawnMask bit n = kind n.
bool spawnMaskAllowsKind(i32 spawnMask, MonsterKind kind) {
    const i32 k = classIndexOfKind(kind);
    if (k < 0 || k >= kMonsterClassCount) {
        return false;
    }
    return ((static_cast<u32>(spawnMask) >> static_cast<u32>(k)) & 1u) != 0u;
}

// [c7][c24] Lair named monster: 75%.
// UNKNOWN: the exact comparison used by the percent roll (< or <=) is not stated;
// IRng::percent is used as the engine's percent roll.
bool rollLairNamed(Ctx& ctx) {
    return ctx.rng.percent(kLairNamedPercent);
}





// [c19] Quest rare monster = Chance < 10.
bool isRareMonsterChance(i32 chance) {
    return chance < kRareChanceBelow;
}


// [c23][c99] Attempts per group = min(living count, 5).
// UNKNOWN: negative living counts are passed through unchanged (SRD gives min only).
i32 attemptsForGroup(i32 livingCount) {
    return livingCount < kAttemptsPerGroupCap ? livingCount : kAttemptsPerGroupCap;
}

// [c4] Fear release: hits > 0 and not ForLife.
bool fearReleases(const Character& c) {
    return c.vitals.curHits > 0 && !c.forLife;
}

// [c23] Crit chance clamped to [lo, hi] = [2, 50].
i32 clampCritChance(i32 chance) {
    if (chance < kCritLo) {
        return kCritLo;
    }
    if (chance > kCritHi) {
        return kCritHi;
    }
    return chance;
}

// [c23] Backstab chance clamped to [lo, hi] = [4, 75].
i32 clampBackstabChance(i32 chance) {
    if (chance < kBackstabLo) {
        return kBackstabLo;
    }
    if (chance > kBackstabHi) {
        return kBackstabHi;
    }
    return chance;
}

// [c24][c31] Mode table for companion spawn selection.
// CONFLICT: the mode table says mode 6 selects table index 4 (not 6), and the
// fidelity note says "modes 4 and above name a table index". The table is followed.
CompanionSpawnSelection companionSpawnSelection(i32 mode) {
    CompanionSpawnSelection sel;
    if (mode == 0) {
        // native floor offsets -3..-1
        sel.floorOffsetMin = -3;
        sel.floorOffsetMax = -1;
    } else if (mode == 1) {
        // native floor offsets -3..0
        sel.floorOffsetMin = -3;
        sel.floorOffsetMax = 0;
    } else if (mode == 2) {
        // native floor offset 0
        sel.floorOffsetMin = 0;
        sel.floorOffsetMax = 0;
    } else if (mode == 3) {
        // repeat previous monster
        sel.repeatPrevious = true;
    } else if (mode == 4 || mode == 5) {
        // table index = mode
        sel.tableIndex = mode;
    } else if (mode == 6) {
        // table index 4 per the mode table
        // CONFLICT: see function comment; table index 4 used for mode 6.
        sel.tableIndex = 4;
    } else if (mode == 7) {
        // keeps previous window
        sel.keepPrevious = true;
    } else if (mode > 7) {
        // table index mode-7
        sel.tableIndex = mode - 7;
    }
    // Negative modes are not in the table; default (no selection) is returned.
    return sel;
}

// [c9][c24][c66] Respawn delay: 1200 + 150 * dungeonLevel - 5 * emptiedRooms
// seconds, floored at 120 s; returned in milliseconds.
// Check values from [c66]: floor 1 / 0 rooms = 1350; floor 1 / 10 rooms = 1300;
// floor 15 / 0 rooms = 3450.
// UNKNOWN: the per-level (150) and per-room (5) terms come from the test set,
// not confirmed engine values; the 5 s term is inferred [c9][c66].
// UNKNOWN: whether emptiedRooms is the shortfall (total - tally) or emptied rooms;
// the test table uses rooms emptied, so that reading is used.
i64 respawnDelayMs(i32 dungeonLevel, i32 emptiedRooms) {
    i64 seconds = static_cast<i64>(kRespawnBaseSeconds)
                + static_cast<i64>(kRespawnPerLevelSeconds) * static_cast<i64>(dungeonLevel)
                - static_cast<i64>(kRespawnPerEmptiedRoomSeconds) * static_cast<i64>(emptiedRooms);
    if (seconds < static_cast<i64>(kRespawnFloorSeconds)) {
        seconds = static_cast<i64>(kRespawnFloorSeconds);
    }
    return seconds * kMsPerSecond;
}


// [c100] Trap table Floor value as a bit mask: bit (level-1) set = placed on
// that floor (e.g. 31 = floors 1-5; 64 = floor 7; 1 = floor 1).
// UNKNOWN: this bit reading is inferred from the trap table, not stated in the SRD.
// UNKNOWN: floor-0 rows are probably not floor-placed [c100]; a zero mask places nothing.
// Levels outside 1..31 cannot be represented and return false.
bool floorMaskPlaces(i32 floorMask, i32 dungeonLevel) {
    if (dungeonLevel < 1 || dungeonLevel > 31) {
        return false;
    }
    return ((static_cast<u32>(floorMask) >> static_cast<u32>(dungeonLevel - 1)) & 1u) != 0u;
}

}  // namespace mordor
