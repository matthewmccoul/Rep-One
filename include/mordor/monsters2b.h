#pragma once
// monsters2b: SRD part c11p2 (monster encounter rules, companion spawn, respawn, tally, library knowledge).
// This part has no R-numbers; rule references are the SRD source chunk tags [cN].
// Engagement base chance (monster align x character align) is already API hostileBase() in clock.h; not redeclared here.
// Quest rare-rejection and quest item constants are owned by guilds1.h / guilds2.h; not redeclared here.
// CONFLICT: [c19] quest rare rejection is 75%, but API kRareRejectPercent (guilds1.h) is 25. Not declared here.
// UNKNOWN: [c97] "ML" in bonus-HP rule and [c85] Skin/damage_bonus combination are not in this part; not declared.
// UNKNOWN: [c5][c99] Acid pre-attack monster attack semantics are not in this part; not declared.
// UNKNOWN: [c99] "cumulative selection" algorithm for monster Chance is not described in this part; not declared.
// UNKNOWN: [c59] store restock weights are out of monster-AI scope; not declared.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants ----

// [c7][c24] lair-named monster drawn for an area's LairId
inline constexpr i32 kLairDrawPercent = 75;
// [c7][c24] any-class roll is Int(Rnd*100)+1 < 2, i.e. hit when below(100) < 1
inline constexpr i32 kAnyClassRollSides = 100;
inline constexpr i32 kAnyClassRollHitBelow = 1;
// [c6] companion breath chance: base 50; 80 if lead much stronger; 20 if much weaker
inline constexpr i32 kBreathChanceBase = 50;
inline constexpr i32 kBreathChanceStronger = 80;
inline constexpr i32 kBreathChanceWeaker = 20;
// [c6] "more than a quarter" stronger or weaker
inline constexpr f64 kBreathStrengthBand = 0.25;
// [c19] level-gap gate percentages
inline constexpr i32 kLevelGapGateHigh = 90;
inline constexpr i32 kLevelGapGateMid = 50;
inline constexpr i32 kLevelGapGateLow = 10;
// [c7][c23] attempts per group is min(living count, this)
inline constexpr i32 kAttemptsPerGroupMax = 5;
// [c7] spread widening step (UNKNOWN amount in SRD; see widenEncounterSpread)
inline constexpr i32 kSpreadStep = 1;
// [c13] area id 0 means no area
// CONFLICT: API kStartArea = -1 (attributes.h) and Location.area defaults to -1; SRD says 0 means none.
inline constexpr i32 kAreaNone = 0;
inline constexpr i32 kNewbieAreaLimit = 18;
// [c106] pariah (mythical) spawn levels, areas listed only on source page
// UNKNOWN: listed area ids are not in this part; only the level window is declared.
inline constexpr i32 kPariahMinLevel = 12;
inline constexpr i32 kPariahMaxLevel = 15;
inline constexpr i32 kAsmodeusFloor = 15;
// [c62] dungeon floor count
inline constexpr i32 kDungeonFloorCount = 15;
inline constexpr i64 kRespawnPerShortfallSeconds = 5;
inline constexpr i64 kRespawnFloorSeconds = 120;
// [c9] standing group is stale when older than this multiple of t
inline constexpr i64 kStaleStandingGroupFactor = 2;
// [c31][c24] companion spawn mode selectors
inline constexpr i32 kSpawnModeNativeWindowWide = 1;
inline constexpr i32 kSpawnModeNativeWindowNarrow = 2;
inline constexpr i32 kSpawnModeRepeat = 3;
inline constexpr i32 kSpawnModeTableFour = 4;
inline constexpr i32 kSpawnModeTableFive = 5;
inline constexpr i32 kSpawnModeTableSix = 6;
inline constexpr i32 kSpawnModeKeep = 7;
inline constexpr i32 kSpawnModeTableOffset = 7;
// [c23] special-hit thresholds (k, lo, hi)
struct ThresholdBand {
    i32 k = 0;
    i32 lo = 0;
    i32 hi = 0;
};
inline constexpr ThresholdBand kCritBand{650, 2, 50};
inline constexpr ThresholdBand kBackstabBand{500, 4, 75};

// ---- enums and small result structs ----

inline constexpr i32 kMonsterKindCount = 18;

// [c23] which special hit
enum class SpecialHitKind : i32 {
    Crit = 0,
    Backstab = 1
};

// [c96] library knowledge of a monster or item record
// CONFLICT: [c88] MDATA13 scale is 1 very little / 2 more to learn / 3 almost all / 4 all known; this scale is [c96].
enum class LibraryKnowledge : i32 {
    UNKNOWN_STATE = 0,
    SEEN = 1,
    DEFEATED = 2,
    FULLY_KNOWN = 3
};

// [c24][c31] companion spawn search window selected by mode
enum class SpawnWindowKind : i32 {
    FloorOffsets = 0,
    RepeatPrevious = 1,
    TableIndex = 2,
    KeepPrevious = 3
};
struct CompanionSpawnWindow {
    SpawnWindowKind kind = SpawnWindowKind::FloorOffsets;
    i32 offsetLo = 0;
    i32 offsetHi = 0;
    i32 tableIndex = 0;
};

// ---- encounter selection and rolls ----

// [c7][c62] target floor of encounter window: level, +1 on Stud square
i32 encounterTargetFloor(i32 dungeonLevel, bool onStud);
// [c1] Asmodeus identity is by Id only
// CONFLICT: Id 307 is shared by Morgul and Shadow Ninja [c57][c100]; identity by Id is ambiguous for that Id.


// ---- companion breath and special hits ----


// ---- companion spawn ----


// ---- respawn and tally ----


// ---- library knowledge ----


}  // namespace mordor
