#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"
#include "mordor/city2a.h"  // FIXED: SeerCharacterOutcome is declared in city2a.h; include it so the type name resolves

namespace mordor {

// ---- constants (R123-R139) ----
inline constexpr i32 kFloorWidth = 30;             // R137 blur clamp 1..30; Automap Square = X + 30*(Y-1)
inline constexpr i32 kFloorHeight = 30;            // R137 blur clamp 1..height. UNKNOWN: height not stated; MDATA11 implies 30x30
inline constexpr i32 kFullBuildLevelLimit = 15;    // R128 mc015E (full build)
inline constexpr i32 kLimitedBuildLevelLimit = 3;  // R128 limited (shareware) build; inferred from MORDOR.WRI and MDATA11
inline constexpr i32 kReleaseLevelThreshold = 15;  // R129 levels >= 15 and some LevelFound/Floor <= 15
inline constexpr i32 kFullMdata11Bytes = 339606;   // R129 release size, full build MDATA11.MDR
inline constexpr i32 kLimitedMdata11Bytes = 68166; // R129 release size, shareware MDATA11.MDR
inline constexpr i32 kSeerDeepestReach = 12;       // R132 DeepestHeCanReach
inline constexpr i32 kSeerEarlyFactor = 5;         // R133 fail if Int(Rnd*100)+1 < level*5
inline constexpr i32 kSeerNeedFactor = 3;          // R136 need = level*3
inline constexpr i32 kSeerLevelOnlySlack = 10;     // R136 roll+10 >= need gives "level only"
inline constexpr i32 kSeerLairPercent = 75;        // R135 lair returned 75% of the time when reached
inline constexpr i32 kSeerMonsterBlurSpread = 4;   // R137 monsters blurred by up to four squares
inline constexpr i32 kSeerPluralNameForm = 2;      // R134 monsterNameFor(Me, 2, record); IdLevel passed as 0
inline constexpr i32 kSeerMonsterFirstRecord = 1;  // R134 monsters read from record 1 on
inline constexpr i32 kSeerItemFirstRecord = 0;     // R134 items read from record 0

// ---- enums and small result structs ----
enum class ReleaseKind : i32 { Full = 0, Limited = 1 };
enum class LimitedAction : i32 { TakeChute = 0, WalkOnChute = 1, TeleporterDestination = 2 };
enum class LimitedMessageKind : i32 { None = 0, ChuteTakeDenied, ChuteWalkOnNoEffect, TeleporterDenied };
enum class ActionTrigger : i32 { EnteredDungeonFromTown = 0, LeavingByStairs = 1 };
enum class ActionRunStart : i32 { Refused = 0, NoHooks, RunHooks };
enum class ExitMenuOutcome : i32 { BlockedByMonsters = 0, ConfirmPrompt };
enum class SessionCloseOutcome : i32 { RefusedMonsters = 0, ConfirmExit };
enum class TakeTarget : i32 { None = 0, Stairs, Chute };
enum class SeerTargetKind : i32 { Monster = 0, Item, Character };
enum class SeerCommonality : i32 { VeryCommon = 0, FairlyCommon, Uncommon, Rare, AlmostUnheard };
enum class SeerFrequency : i32 { AlmostNever = 0, VeryRarely, Rarely, Uncommonly, Commonly, VeryCommonly };

// R123 run-state for the single active action run
struct ActionRunState {
    bool active = false;
};

// R123 per-mod hook record; hooks run in load order (loadOrder ascending)
struct ModHookSlot {
    i32 loadOrder = 0;
    bool live = false;
    bool hasHook = false;
    bool disabled = false;
};

// R133 early failure and level-naming roll
struct SeerEarlyResult {
    bool failed = false;
    bool namesLevel = false;
};

// R135 monster locate result; areaIndex -1 when no candidate
struct SeerMonsterPick {
    bool lairResult = false;
    i32 areaIndex = -1;
};

// R134 name match result; count 0 none, 1 one, >=2 several
struct SeerNameMatch {
    i32 count = 0;
    i32 index = -1;
    bool exact = false;
};

// R136 character locate result
struct SeerCharacterReport {
    SeerCharacterOutcome outcome = SeerCharacterOutcome::CityRefused;  // FIXED: CannotLocate not in SeerCharacterOutcome; closest existing is CityRefused
    i32 x = 0;
    i32 y = 0;
    i32 level = 0;
    bool amongDead = false;
};

// R137 blurred coordinates
struct BlurredPoint {
    i32 x = 0;
    i32 y = 0;
};

// ---- accessors ----
// R19 automap addressing: Square = X + 30 * (Y - 1)
inline constexpr i32 squareIndexOf(i32 x, i32 y) {
    return x + kFloorWidth * (y - 1);
}

// R128 level limit for a release (pure)
inline constexpr i32 buildLevelLimit(ReleaseKind release) {
    return release == ReleaseKind::Full ? kFullBuildLevelLimit : kLimitedBuildLevelLimit;
}


// ---- R124 stairs up from level 1 ----
// R124 leader placed at the party's square; exit to town alive
void climbToTownAlive(Ctx& ctx, Party& party);

















}  // namespace mordor
