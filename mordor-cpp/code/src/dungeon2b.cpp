// Module dungeon2b: chutes, pits, teleporters, lost state, fog, perception,
// rescue and trap-disarm helpers. Rule numbers (Rxx) refer to the c15p2 notes.
//
// UNKNOWN (module-wide): the SRD excerpt gives quirks and conflicts but not the
// full pseudo-code. Floor-data field names (FloorChute::depth) and the automap,
// square-flag and fog bit numbers are not in the excerpt. They are assumed here
// and marked UNKNOWN where used. Verify against game_data.h and the decompile
// before relying on them.
//
// CONFLICT (module-wide): isLimitedBuild, rescueChance(i32,bool),
// deadBodyFacing and trapDisarmLogTerm are also declared in other headers
// (dungeon3.h, dungeon1b.h) with the same signatures. They are defined only here.

#include <cmath>

#include "mordor/dungeon1b.h"  // ChuteTakeOutcome must be visible before dungeon2b.h
#include "mordor/combat2a.h"   // resolveChestTrap
#include "mordor/dungeon2b.h"

namespace mordor {

namespace {

// UNKNOWN: SquareState::trapFlags bit for anti-magic (R24).
constexpr i32 kAntiMagicFlag = 0x1;
// UNKNOWN: SquareState::trapFlags bit for quicksand (R23/c17).
constexpr i32 kQuicksandFlag = 0x2;
// UNKNOWN: SquareState::bits mask for walls and doors removed by fog (c27).
constexpr u32 kWallDoorMask = 0x0Fu;
// UNKNOWN: SquareState::bits mask for secret doors (c13).
constexpr u32 kSecretDoorMask = 0x100u;
// UNKNOWN: automap bit numbers (c16 notes the numbering was lost).
constexpr i32 kAutomapTeleporterBit = 3;  // MAZEMOD 631
constexpr i32 kAutomapBumpBit = 5;        // c42/c44
constexpr i32 kAutomapLostBit = 6;        // c9
// UNKNOWN: safety cap on teleport redraws (the original has no stated cap).
constexpr i32 kTeleportDrawTries = 100000;
// R31 (c17): search ends at this many days (2,000,000-day cap).
constexpr i64 kRescueDayCap = 2000000;
// Take outcome codes (numeric; ChuteTakeOutcome names are not in the excerpt).
constexpr i32 kTakeOk = 0;
constexpr i32 kTakeRefused = 1;

}  // namespace

// ---------------------------------------------------------------------------
// Chutes
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Teleporters and automap
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Lost state
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Fog and mapping
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Perception
// ---------------------------------------------------------------------------

}  // namespace mordor
