#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

inline constexpr i32 kRespawnSecondsPerLevel = 150;      // R3 150 s per level
inline constexpr i32 kRespawnEmptyAreaPenaltySec = 5;    // R4 5 s per empty area below header count
inline constexpr i64 kRoomUnsetStamp = 0;                // R8 stamp 0 reads as never set
inline constexpr i32 kEmptiedRoomRef = -1;               // R8 Foes(0).Ref == -1 marks an emptied room
inline constexpr i32 kNewbieCharacterLevel = 1;          // R12 newbie guard: character level 1
inline constexpr i32 kNewbieDepthLimit = 18;             // R12 newbie guard applies in areas below this floor
inline constexpr i32 kNewbieGuildLevelMax = 2;           // R12 newbie guard rerolls leads above this guild level
inline constexpr i32 kNewbieExemptClassIndex = 4;        // R13 table index 4 (not monster Id 4) is exempt
inline constexpr i32 kBypassRollPercent = 1;             // R10 1% bypass roll
inline constexpr i32 kNeutralFriendlyPercent = 40;       // R20 neutral friendliness gate
inline constexpr i32 kJoinRollSides = 3000;              // R24 join chance draw range 1..3000
inline constexpr i32 kJoinChaMultiplier = 10;            // R24 CHA * 10
inline constexpr i32 kJoinLevelMultiplier = 10;          // R24 (level - 1) * 10
inline constexpr i32 kCompanionFloorWindowMax = 2;       // R16 modes 0..2 are floor windows
inline constexpr i32 kCompanionKeepWindowMode = 7;       // R16 mode 7 keeps previous window
inline constexpr i32 kCompanionModeOffset = 7;           // R16 modes above 7 map to mode - 7
inline constexpr i32 kCompanionSubtypeRetryLimit = 150;  // R17 UNKNOWN: 150 (audit) vs 300 abandon (fidelity)
inline constexpr i32 kPenCapacity = 9;                   // pen overflow (store restock) cap on pen 0
inline constexpr i32 kRestockEmptyDrawLimit = 100;       // store restock buy-out gives up after 100 empty draws
inline constexpr f64 kChestCountDamageBonus = 0.75;      // damage modifier added while counting a chest

// R16 companion spawn mode kinds
enum class CompanionModeKind : i32 {
    FloorWindow = 0,   // modes 0..2: floor windows
    TableIndex = 1,    // modes 4..6 and above 7: table index
    KeepWindow = 2     // mode 7: keeps the previous window
};

// R16 result of resolving a companion spawn mode
struct CompanionModeResult {
    CompanionModeKind kind = CompanionModeKind::FloorWindow;
    i32 tableIndex = -1;
};

// ---- rule declarations ----



















// R29 level-gap specials: UNKNOWN which specials use LevelGapChance. Not declared.















}  // namespace mordor
