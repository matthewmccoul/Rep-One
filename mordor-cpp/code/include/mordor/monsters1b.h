#pragma once
// SRD part c11p1: Monsters, Member Step-Up through Quick Reference.
// Declarations only. Bodies are implemented in src/monsters1b*.cpp.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

namespace mordor {

struct GameState;  // defined by the game state module (see API INDEX clock.h / progression2.h)

// ---- constants ----
// R38 monster attack resolution
inline constexpr i32 kMeleeAttemptsMax = 5;
// R42 joining
inline constexpr i32 kJoinBindingLevel = -10;
// R43 friendly join
inline constexpr i32 kFriendlyJoinRollRange = 3000;
inline constexpr i32 kJoinGroupCap = 4;
// R46 Asmodeus
inline constexpr i32 kAsmodeusMonsterId = 277;
// R49 room occupancy
inline constexpr i32 kStaleWindowFactor = 2;
inline constexpr i64 kMsPerSecond = 1000;
// R50 respawn delay
inline constexpr i32 kRespawnBaseSeconds = 1200;
inline constexpr i32 kRespawnPerLevelSeconds = 150;
inline constexpr i32 kRespawnPerAreaSeconds = 5;
inline constexpr i32 kRespawnMinSeconds = 120;
// R53 quest pick
inline constexpr i32 kQuestClassHeadStartPercent = 40;
inline constexpr i32 kQuestGapHighPercent = 90;
inline constexpr i32 kQuestGapMidPercent = 50;
inline constexpr i32 kQuestGapLowPercent = 10;
// W7 / W9 paralysis stat shift
inline constexpr i32 kGateFreeAdShift = -32;
inline constexpr i32 kParalysisFullPart = 32;
// W13 library knowledge
inline constexpr i32 kKnowledgeScale = 10000;
inline constexpr u32 kItemDeathsSentinel = 0xFFFFu;
// W17 floor bitmask (hypothesis, UNVERIFIED)
inline constexpr u32 kFloorMaskAllEleven = 2047u;

// ---- enums ----
// W2 companion spawn mode. UNKNOWN: numeric codes; Companion.spawnMode is i32 and its mapping is not given.
enum class CompanionSpawnMode : i32 {
    Always,
    PercentageChance,
    OnlyIfSpacePermits
};








// Pure: Good monsters join only Good characters. Neutral monsters can join any character. Evil monsters never join.

// Hostility: Good monsters are hostile to Neutral/Evil characters. Neutral monsters are hostile 60% of the time.
// Evil monsters are always hostile.
// CONFLICT: overlaps hostileBase / hostileRoll in clock.h (engagement). Wiki rule is declared here for the join path only.


// R45 Charm and join roll
// No routine here: the roll is given in R43 (friendlyJoinRoll).






// R51 Refill waiting
// Covered by refillDue (strict >) and groupStale (stale after twice the respawn time).

// R52 Gold and chests
// Covered by settleFightGold (R47.2) and markCleared (chest stays).

// R53 Quest "Defeat One" monster pick
// Loop repeated until a valid pick exists. Returns table index, or -1 when there are no classes.
// Tables are the caller's view of monster classes (classFirst[cls], classLast[cls]) and monsters (levelFound, chance).
// Quirk: no cap in the original. VB And is not short-circuit, so the Rnd draw happens for every candidate.
// Rare is chance < kRareMonsterWeight. Turned is Rnd*100 > kRareRejectPercent.


// W1 Level population (generator)
// No routine declared: step order is Unknown.


// W3 to W6 Winning a fight, room occupancy, refill, respawn time
// See R47 to R51. No new declarations.


// W8 Paralysis cured and W9 freed at city gate
// See unparalyse and clearParalysisAtCityGate in the API INDEX.

// W10 Monster attack resolution: see R38.
// W11 Charm and join: see R44.
// W12 Spreadsheet note: display only, out of scope.

// W13 Library knowledge: constants kKnowledgeScale and kItemDeathsSentinel above.

// W14 Bonus HP at early guild levels: see ConHitsBonus in the API INDEX (guilds3.h).




// W18 Spawn counts and laired placement: hypothesis only, no routine declared.

// Quick Reference
// Every item refers to a rule above or to a rule in another module. No new declarations.

}  // namespace mordor
