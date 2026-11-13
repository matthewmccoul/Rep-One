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

// [c7][c24] any-class roll: Int(Rnd*100)+1 < 2
bool rollAnyClass(Ctx& ctx);
// [c7][c24] lair-named monster is drawn 75% of the time
bool rollLairDraw(Ctx& ctx);
// [c7][c24] neutral lead is peaced 40% of the time
bool rollNeutralLeadPeaced(Ctx& ctx);
// [c19] quest class head start 40%
bool rollQuestClassHeadStart(Ctx& ctx);
// [c7][c62] target floor of encounter window: level, +1 on Stud square
i32 encounterTargetFloor(i32 dungeonLevel, bool onStud);
// [c7] widen LevelFound window when nothing is found
// UNKNOWN: step size is not stated; kSpreadStep (1) is the simplest choice.
void widenEncounterSpread(Ctx& ctx, Encounter& encounter);
// [c7][c19] level-gap gate percentage (90 / 50 / 10)
// UNKNOWN: which gap values map to which percentage is not stated in this part.
i32 levelGapGatePercent(i32 levelGap);
// [c25][c42] JoinOffer is read only from group 0
bool readJoinOffer(const Encounter& encounter);
// [c23] attempts per group = min(living count, kAttemptsPerGroupMax)
i32 attemptsPerGroup(i32 livingCount);
// [c5] member at 0 hits still counts as alive until its hits are written
bool countsAsAlive(const Creature& creature);
// [c56] character state that ends a monster's blow sequence
// UNKNOWN: which states end the sequence is not listed; any non-Alive state is assumed.
bool endsBlowSequence(DeathState state);
// [c56] companion mishap kills a lone character mid-round
// UNKNOWN: trigger condition and damage are not stated in this part.
void companionMishap(Ctx& ctx, Party& party, const Companion& companion);
// [c97] charm forces a monster to join if its damage exceeds remaining HP
bool charmForcesJoin(i32 damage, i32 remainingHits);
// [c4] fear releases companions that have hits > 0 and are not ForLife
// Caller releases with releaseCompanion() (guilds1.h).
bool fearReleases(const Companion& companion);
// [c7] newbie protection for level-1 characters in areas below kNewbieAreaLimit
bool newbieProtected(const Character& ch, i32 areaId);
// [c7][c13] monster is the lair of an area when its Id equals the area's LairId
bool isLairMonster(const Area& area, const Creature& creature);
// [c1] Asmodeus identity is by Id only
// CONFLICT: Id 307 is shared by Morgul and Shadow Ninja [c57][c100]; identity by Id is ambiguous for that Id.

// [c13] SpawnMask bit n = monster kind n
// FIXED: type MonsterKind exists nowhere in the headers; using plain i32 kind index (kind n = bit n).
bool spawnMaskAllowsKind(i32 spawnMask, i32 kind);
// [c100] trap-table floor value read as bitmask: bit (floor-1) set means floor allowed
// UNKNOWN: reading is consistent with all listed examples (31, 15, 1, 64, 29) but unverified.
bool floorMaskAllows(i32 floorMask, i32 dungeonLevel);
// [c106] pariah spawns only on levels kPariahMinLevel..kPariahMaxLevel
bool pariahLevelEligible(i32 dungeonLevel);

// ---- companion breath and special hits ----

// [c6] companion breath chance percent from lead and companion strength
// UNKNOWN: the strength measure is not stated; both values are assumed comparable on one scale.
i32 companionBreathChance(i32 leadStrength, i32 companionStrength);
// [c6] companion breath is rolled each round against companionBreathChance
bool companionBreathRoll(Ctx& ctx, i32 leadStrength, i32 companionStrength);
// [c23] clamp a raw special-hit chance into the band's lo..hi
// UNKNOWN: how raw relates to band.k is not stated in this part; clamp to lo..hi only.
i32 specialHitChance(SpecialHitKind kind, i32 raw);
// [c23] roll a crit or backstab with the band-limited chance
// CONFLICT: [c5] vs [c23] disagree on which flag bits (10/11) enable crit/backstab; this function does not read flags.
bool rollSpecialHit(Ctx& ctx, SpecialHitKind kind, i32 raw);

// ---- companion spawn ----

// [c24][c31] resolve a companion spawn mode into its search window
// CONFLICT: [c31] says modes 4 and above name a table index, but mode 6 maps to table index 4
// (per [c24] table) while the ">7 -> mode-7" rule does not cover 6. Table value is used.
CompanionSpawnWindow companionSpawnWindow(i32 mode);
// [c24][c31] choose a monster table index inside the window
// UNKNOWN: the selection procedure within the window is not described in this part.
i32 pickCompanionMonster(Ctx& ctx, const CompanionSpawnWindow& window, i32 nativeFloor, i32 previousMonsterIndex);

// ---- respawn and tally ----

// [c9][c24][c66] respawn delay in seconds for an emptied room:
// 1200 + 150*dungeonLevel - 5*shortfall, floor 120.
// CONFLICT: [c66] test rows use rooms emptied as shortfall; [c9] says per room of total - tally.
// Main table value (total - tally) is used; callers pass roomTotal - roomTally.
i64 respawnDelaySeconds(i32 dungeonLevel, i32 roomShortfall);
// [c9] standing group is stale when older than 2t
bool standingGroupStale(i64 nowSeconds, i64 stampSeconds, i64 respawnT);
// [c9][c24] seed a floor's tally from the header area count
// UNKNOWN: FloorState.tally vs roomTally is not distinguished in this part; tally is used.
void seedFloorTally(Ctx& ctx, FloorState& floor, i32 headerAreaCount);
// [c9] record an emptied room: stamp, respawn delay t, and tally increment
void markAreaEmptied(Ctx& ctx, FloorState& floor, Area& area, i64 nowSeconds);

// ---- library knowledge ----

// [c96] map a raw knowledge value to its state (>= 3 is FULLY_KNOWN)
LibraryKnowledge libraryKnowledgeFor(i32 stateValue);

}  // namespace mordor
