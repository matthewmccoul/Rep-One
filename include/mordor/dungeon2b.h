#pragma once
// dungeon2b: SRD part c15p2. Chutes and pits, teleporters, square effects,
// lost state, fog and perception, rescue and rocked characters, trap disarm
// formulas. Declarations only.
#include "mordor/types.h"
#include "mordor/dungeon1b.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants ----
inline constexpr i32 kChuteFloatPercent = 7;           // R2 chute float threshold as written (level 8)
inline constexpr i32 kChuteFloatLevel = 8;             // R2 level the float threshold is stated for
inline constexpr i32 kFullBuildLevelCount = 15;        // R21 full build has at least 15 dungeon levels
inline constexpr i32 kLimitedBuildTeleportCap = 3;     // R17 limited spell clamp (value inferred, c33/c35)
inline constexpr i32 kShareMonsterItemReach = 16;      // c35 shareware monsters and items reach level 16
inline constexpr i32 kRescueBaseLevel = 16;            // R31 rescue chance is max(16 - L, floor)
inline constexpr double kRescueMinChance = 0.1;        // R31 floor applied before easy-find
inline constexpr double kRescueCeilingChance = 100.0;  // R31 ceiling on level 1 areas 18 and lower

// FIXED: SquareEffectStep is declared nowhere in the API INDEX or state headers,
// so the square-effect steps are plain i32 values in the preferred order (c44).
// Order: 0 WaterOrQuicksand, 1 AntiMagicAndStudPerception, 2 Extinguisher,
// 3 Facing, 4 Rotator, 5 Pit, 6 StairsCaption, 7 Teleporter, 8 Chute
inline constexpr Arr<i32, 9> kSquareEffectOrder = {
    0,  // WaterOrQuicksand
    1,  // AntiMagicAndStudPerception
    2,  // Extinguisher
    3,  // Facing
    4,  // Rotator
    5,  // Pit
    6,  // StairsCaption
    7,  // Teleporter
    8   // Chute
};


// R8 result of a pit fall
struct PitFallResult {
    i32 fallen = 0;
    i32 killed = 0;
};

// R14 setters of the forced flag (preferred: three setters, probe asserts three, c44/c56)
enum class ForcedFlagSetter : i32 {
    TeleporterSquare = 0,
    TeleportSpell,
    LoneChestTrap
};

// R31 result of a body search
struct RescueResult {
    i64 daysSearched = 0;
    bool found = false;
};

// ---- chutes, pits and falls ----












// ---- teleporters ----












// ---- limited and shareware builds ----


// R22 limited and shareware naming: UNKNOWN whether one build. No routine declared.

// ---- square effects ----




// R26 extinguisher behaviour: UNKNOWN in the wiki ("dont know", c96). No routine declared.

// ---- combat clock and encounters ----



// R29 encounter generation beyond the lead group is UNKNOWN (c47). No routine declared.

// ---- rescue, death and retrieval ----






// ---- traps and disarm ----



// R35 trap identification roll may show a guessed trap that differs from the disarm value (c24). Not a rule; no routine.
// R36 disarm party-size adjustment is named but unspecified (c24). No routine.

// ---- lost state ----


// FoundYourself resolves all lost flags. The position roll runs when floor or facing is lost (c13)
void foundYourself(Ctx& ctx, Party& party);




// ---- fog and perception ----










// ---- chests, areas and companions ----




// Teleport-related caption and screen behaviour, look readout, take-button mapping and asmodeus picture are out of scope (screens/input).

}  // namespace mordor
