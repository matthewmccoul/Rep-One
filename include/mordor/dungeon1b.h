#pragma once
// dungeon1b.h: SRD part c15p1 (R56-R122).
// Chutes, teleporter squares, teleport/displacement/portal/locate spells,
// area change and forced settle, rotator facing, lead group, look/roster,
// bodies and carrying, rescue, chests and traps, and joining.
// API INDEX items are used here, never redeclared.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"
#include "mordor/spells1a.h"   // ArrivalOutcome
#include "mordor/spells1b.h"   // PortalOutcome, LocateLine
#include "mordor/spells2.h"    // DisplaceOutcome
#include "mordor/status1b.h"   // sub0930, pruneTheDead, setDead
#include "mordor/status2.h"    // paralyse
#include "mordor/loot1a.h"     // ChestOpenStart
#include "mordor/loot2.h"      // DisarmResult
#include "mordor/death1a.h"    // AskToJoinOutcome // FIXED: was undefined, include its declaring header

namespace mordor {

// ---- Constants ----
inline constexpr i32 kNoRecord = -1;                  // R56 no chute or teleporter record
inline constexpr i32 kLimitedLevelCap = 3;            // R86 limited build clamps level to 3
inline constexpr i32 kTeleportWrapSize = 30;          // R83 coordinates wrap at 30x30
inline constexpr i32 kDisplaceLevelLimit = 3;         // R83 level offset clamped to -3..3

// ---- Chute structures (R56-R66) ----
struct ChuteFloatRolls {                   // R57 one roll per member, leader first
    Arr<bool, kPartyMaxSize> floats{};
};

struct ChuteFallPlan {                     // R57, R58 pure plan from the rolls
    Arr<bool, kPartyMaxSize> falls{};          // member goes down
    Arr<bool, kPartyMaxSize> goesNowhere{};    // floated but did not follow
    bool leaderFloatHint = false;              // R57 hint when the leader floats
    bool askFollow = false;                    // R58 Follow Leader box shown
};

struct ChuteWalkResult {
    bool recordFound = false;              // R56
    bool fell = false;                     // R57/R59 someone changed level
    i32 destLevel = 0;                     // R59
    i32 damage = 0;                        // R60
    bool leaderChangedLevel = false;       // R64
    bool rerun = false;                    // R65
};

enum class ChuteTakeOutcome : i32 {
    Dropped = 0,                           // R67 everyone sent down
    RefusedLimitedDeadEnd = 1              // R67 limited build dead-end level
};

// ---- Teleporter structures (R75-R82) ----
struct TeleporterDest {
    i32 x = 0;                             // 0-based destination square
    i32 y = 0;
    i32 level = 0;
    bool unmapped = false;                 // R77 destination is unmapped
};

// ---- Stairs, pickup, chest and join outcomes ----
enum class StairsOutcome : i32 {
    Taken = 0,                             // R69
    RefusedParalysed = 1,                  // R69 up-climb while paralysed
    RefusedDead = 2                        // R69 up-climb in death state
};

enum class PickUpOutcome : i32 {
    PickedUp = 0,                          // R104
    PickedUpDroppedFirst = 1,              // R104 previous carry dropped and said so
    NoCandidate = 2,                       // R104
    Refused = 3                            // R104 picker dead or paralysed
};

enum class ChestTeleportOutcome : i32 {
    Unmapped = 0,                          // R118 all three lost flags set
    OpenerRemoved = 1,                     // R118 party size > 1
    Teleported = 2                         // R118 party size = 1
};


// ---- Look / roster (R99-R100) ----
struct RosterCounts {
    i32 living = 0;
    i32 dead = 0;
};

// ---- Lead group picture (R98) ----
struct PictureLayout {
    i32 x = 0;
    i32 y = 0;
    i32 w = 0;
    i32 h = 0;
};

i32 mapHeight(const GameData& data, i32 level);                          // map height for a level
i32 squareIndexOf(const GameData& data, i32 level, i32 x, i32 y);       // index into FloorState::squares, -1 if off map

void changeLevel(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                 i32& flags, i32 newLevel);                                                // FIXED: AreaFlags undefined, now i32& flags. R64 ChangeLevel






void leaveArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data);     // R94 LeaveArea; hostile groups not fighting get leave round





bool deadScreenOwed(const Party& party);                                                   // R112 dead and alone (UBound(Chars) = 0)
// R115 ordinary rescue is covered by applyRescueArrival and waitForRescueDays.



}  // namespace mordor
