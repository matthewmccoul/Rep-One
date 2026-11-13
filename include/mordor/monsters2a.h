#pragma once

// Module monsters2a: SRD c11p2 (monster engagement, thresholds, companion spawn modes, respawn).
// This part gives no R-numbers; rules are cited by their SRD source tag, e.g. [c7].
// Engagement base chance table [c18][c25] is already declared as hostileBase() in clock.h; not repeated here.
// Free-attack threshold [c25] is already declared as freeAttackThreshold() in clock.h; not repeated here.
// Breath/acid constants 0.7, 1.08, 15, 20, 10 [c99] and join constants 3000, 300, 10, 3 [c97] have no stated use
// in this part (UNKNOWN); they are deliberately not declared.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"

namespace mordor {

// ---- Constants ----

// [c13] Class count shipped; MonsterKind values 0..17 map to class indices by numeric match.
inline constexpr i32 kMonsterClassCount = 18;

// UNKNOWN: the 5 s per empty-area term is inferred from the test set [c9][c66].
inline constexpr i32 kRespawnPerEmptiedRoomSeconds = 5;

// [c7][c24] Lair, any-class and Neutral-lead rolls.
inline constexpr i32 kLairNamedPercent = 75;
inline constexpr i32 kAnyClassRange = 100;      // Int(Rnd*100)+1
inline constexpr i32 kAnyClassThreshold = 2;    // ... < 2, i.e. 1%
inline constexpr i32 kNeutralLeadPeacedPercent = 40;


// [c19] Quest: rare monster = Chance < 10.
inline constexpr i32 kRareChanceBelow = 10;

// [c97][c25] At most 4 offers in a group (min(4, ...)).
inline constexpr i32 kMaxJoinOffersPerGroup = 4;

// [c18] Level-gap gate percentages [c19] (band edges are UNKNOWN; see levelGapGatePercent).
inline constexpr i32 kLevelGapHighPercent = 90;
inline constexpr i32 kLevelGapMidPercent = 50;
inline constexpr i32 kLevelGapLowPercent = 10;
// UNKNOWN: gap band edges are not stated in the SRD part; these are placeholders.
inline constexpr i32 kLevelGapNearMax = 2;
inline constexpr i32 kLevelGapMidMax = 5;

// [c6] Companion breath chance.
inline constexpr i32 kCompanionBreathBasePercent = 50;
inline constexpr i32 kCompanionBreathHighPercent = 80;
inline constexpr i32 kCompanionBreathLowPercent = 20;
inline constexpr f64 kBreathQuarterFraction = 0.25;

// [c23] Crit and backstab thresholds: k, lo, hi.
// CONFLICT: c23 says flag 11 enables backstab; c5 names flag 11 StealGold. Flag 10 is Critical in both.
// UNKNOWN: how k is used; this part gives only the clamp range reading.
inline constexpr i32 kCritK = 650;
inline constexpr i32 kCritLo = 2;
inline constexpr i32 kCritHi = 50;
inline constexpr i32 kBackstabK = 500;
inline constexpr i32 kBackstabLo = 4;
inline constexpr i32 kBackstabHi = 75;

// [c5] Special attack flag bit positions used by the crit/backstab rules.
// CONFLICT: see kCritK note above; bit 11 is the backstab bit per [c23] and StealGold per [c5].
inline constexpr i32 kSpecialBitCritical = 10;
inline constexpr i32 kSpecialBitBackstab = 11;

// [c23] Melee attempts per group: min(living count, 5).
inline constexpr i32 kAttemptsPerGroupCap = 5;

inline constexpr i32 kStatPartAtEighteen = 32;

// [c66][c97] Bonus HP: Con must be at least 17 (Con - 16 counts). Consistent with kConBonusThreshold = 16.
inline constexpr i32 kConBonusMinCon = 17;

inline constexpr i32 kSeenFoundScale = 10000;

// [c100] Trap-table floor value with all eleven flag bits set.
inline constexpr i32 kAllElevenFlagsMask = 2047;

inline constexpr i32 kMorgulMonsterId = 307;
// CONFLICT: ID 307 is used by both Morgul and Shadow Ninja [c100]; the id alone does not identify a monster.
inline constexpr i32 kShadowNinjaMonsterId = 307;
// [c100] ID 275 is missing from the trap table (rows 274-276).
inline constexpr i32 kMissingMonsterId = 275;

// [c13] MonsterKind; SpawnMask bit n = kind n.
// CONFLICT: value 10 is "Aquatic" in the SRD table [c13] and "Water-Dweller" in the trap table [c100][c91].
// UNKNOWN: whether the numeric class mapping is confirmed [c6][c13].
enum class MonsterKind : i32 {
    Humanoid = 0,
    Cleanup = 1,
    Demon = 2,
    Devil = 3,
    Elemental = 4,
    Reptile = 5,
    Dragon = 6,
    Animal = 7,
    Insect = 8,
    Undead = 9,
    Aquatic = 10,
    Giant = 11,
    Mythical = 12,
    Lycanthrope = 13,
    Thief = 14,
    Mage = 15,
    Warrior = 16,
    Indigini = 17
};

// [c13] Kind names in value order (Aquatic per the SRD table; see CONFLICT above).
inline constexpr const char* kMonsterKindNames[kMonsterClassCount] = {
    "Humanoid", "Cleanup", "Demon", "Devil", "Elemental", "Reptile",
    "Dragon", "Animal", "Insect", "Undead", "Aquatic", "Giant",
    "Mythical", "Lycanthrope", "Thief", "Mage", "Warrior", "Indigini"
};

// [c5] Companion spawn mode selection, as read from the mode table [c24][c31].
// CONFLICT: [c31] says "Modes 0-2 are floor windows. Mode 3 repeats... Modes 4 and above name a table index";
// the table itself also gives mode 4 and mode 6 both as table index 4.
struct CompanionSpawnSelection {
    bool repeatPrevious = false;   // mode 3: repeat previous monster
    bool keepPrevious = false;     // mode 7: keeps previous window
    i32 floorOffsetMin = 0;        // native floor offset window, inclusive (modes 0-2)
    i32 floorOffsetMax = 0;
    i32 tableIndex = -1;           // table index for modes 4-6 and >7; -1 = none
};

// ---- Monster kinds and spawn masks [c13] ----

// [c13] Class index of a kind; numeric match with ClassCount 18. UNKNOWN: mapping confirmation.
i32 classIndexOfKind(MonsterKind kind);

// [c13] True when SpawnMask bit n (n = kind value) is set for this kind.
bool spawnMaskAllowsKind(i32 spawnMask, MonsterKind kind);

// ---- Engagement rolls [c7][c18][c19][c24] ----

// [c7][c24] 75% lair named monster.
bool rollLairNamed(Ctx& ctx);

// [c7] 1% any-class roll: Int(Rnd*100)+1 < 2.
bool rollAnyClass(Ctx& ctx);

// [c7][c24] 40% Neutral lead peaced. (Neutral monster hostility is the complement, 60% [c7][c97].)
bool rollNeutralLeadPeaced(Ctx& ctx);

// [c18][c19] Level-gap gate percent for a level gap.
// UNKNOWN: the SRD gives only the values 90 / 50 / 10 and not their band edges.
// Preferred: gap <= kLevelGapNearMax -> 90; gap <= kLevelGapMidMax -> 50; otherwise 10.
i32 levelGapGatePercent(i32 gap);

// [c19] 40% quest class head start.
bool rollQuestClassHeadStart(Ctx& ctx);

// [c19] Rare monster: Chance < 10.
bool isRareMonsterChance(i32 chance);

// [c6] Companion breath chance (percent). Base 50; 80 when the lead is more than a quarter stronger;
// 20 when more than a quarter weaker.
// UNKNOWN: the stat compared ("strength") is not named in this part; passed in as a power value.
i32 companionBreathChance(i32 leadPower, i32 companionPower);

// [c23] Melee attempts for a group: min(living count, 5).
i32 attemptsForGroup(i32 livingCount);

// [c4] Fear release: hits > 0 and not ForLife.
bool fearReleases(const Character& c);

// ---- Crit and backstab clamps [c23] ----

// [c23] Clamp a crit chance into [kCritLo, kCritHi].
// UNKNOWN: this part gives lo/hi with k but not the formula that k enters.
i32 clampCritChance(i32 chance);

// [c23] Clamp a backstab chance into [kBackstabLo, kBackstabHi].
// UNKNOWN: as for clampCritChance.
i32 clampBackstabChance(i32 chance);

// ---- Companion spawn modes [c24][c31] ----

// [c24][c31] Resolve a companion spawn mode to its search window or table selection.
// UNKNOWN: negative modes are not in the table; implementer should treat them as mode 0.
CompanionSpawnSelection companionSpawnSelection(i32 mode);

// ---- Respawn [c9][c24][c31][c66] ----

// [c9][c66] Respawn delay in milliseconds for a floor and number of emptied rooms:
// max(kRespawnMinSeconds, kRespawnBaseSeconds + kRespawnPerLevelSeconds * dungeonLevel
//     - kRespawnPerEmptiedRoomSeconds * emptiedRooms) * 1000.
// CONFLICT: the SRD gives "per level" and "per empty area" in general terms; the test set [c66] reads them as
// per floor and per room of (total - tally). emptiedRooms is taken as (total - tally).


// [c24] Stale standing group: older than 2t.
// UNKNOWN: t is not defined in this part; tMs is taken as that t in milliseconds.
bool standingGroupStale(i64 groupStampMs, i64 nowMs, i64 tMs);

// ---- Trap-table floor mask [c100] ----

// [c100] Trap-table floor value read as a bitmask: bit (level-1) set means floor `level` is included.
// e.g. Kobold 31 = floors 1-5; Phoenix 64 = floor 7.
// UNKNOWN: flag column meanings are not stated; floor 0 rows are probably not floor-placed (mask 0 -> false).
bool floorMaskPlaces(i32 floorMask, i32 dungeonLevel);

} // namespace mordor
