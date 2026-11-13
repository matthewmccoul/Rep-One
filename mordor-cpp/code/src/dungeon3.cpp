#include <cmath>

#include "mordor/dungeon3.h"
#include "mordor/ctx.h"
#include "mordor/events.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/types.h"

namespace mordor {

namespace {

// UNKNOWN: map side length. Engine addressing and MDATA11 give 30 (R51).
constexpr i32 kMapSide = 30;

// R37: notice roll is rng.Next(100) + 1 <= P. Next(100) assumed 0-99.
constexpr i32 kNoticeRange = 100;

// R53 / c18: stud spread draw is Rnd*75 < 50.
constexpr double kStudDrawRange = 75.0;
constexpr double kStudDrawLimit = 50.0;
constexpr i32 kStudSpreadOn = 1;

// Shareware / limited build threshold (R33 via c33, c35).
constexpr i32 kFullDungeonLevelCount = 15;

// R42: facings are 0-based in this port (&3). CONFLICT, see R42.
constexpr i32 kFacingCount = 4;

// Rescue chance constants (c17).
constexpr double kRescueBaseLevel = 16.0;
constexpr double kRescueMinChance = 0.1;
constexpr double kRescueCeiling = 100.0;
constexpr i32 kCeilingLevel = 1;
constexpr i32 kCeilingMaxArea = 18;

// Teleporter draw exclusion range (c22, MAZEMOD 1798-1808).
constexpr i32 kMazeModExcludeLow = 1798;
constexpr i32 kMazeModExcludeHigh = 1808;

// Wrong-guess bits (R39): literal reading "eight bits always pass".
constexpr u32 kWrongGuessBits = 0xFFu;

// Facing arrow base offset (R43 / c63).
constexpr i32 kFacingArrowOffset = 22;

// Body facing offset (R42 / c48).
constexpr i32 kBodyFacingOffset = 3;

// Forward step per facing for the ethereal portal (R42 mapping UNKNOWN).
// 0 = north (y-1), 1 = east (x+1), 2 = south (y+1), 3 = west (x-1).
constexpr i32 kStepDx[4] = {0, 1, 0, -1};
constexpr i32 kStepDy[4] = {-1, 0, 1, 0};

i32 squareIndexOf(i32 x, i32 y) {
    return y * kMapSide + x;
}

}  // namespace

// Square effects on arrival (R47, c44 order preferred).
void applySquareEffects(Ctx& ctx, Arr<Character, kPartyMaxSize>& members, i32 partySize, FloorState& floor, Location& loc) {
    (void)members;
    (void)partySize;
    const i32 index = squareIndexOf(loc.x, loc.y);
    if (index < 0 || index >= static_cast<i32>(floor.squares.size())) {
        return;
    }
    SquareState& square = floor.squares[static_cast<u32>(index)];

    // UNKNOWN: order of automap recording versus square effects is not stated (R47).
    // Automap recording is placed first here.
    mapSquareOnAutomap(ctx, square, false, false);

    // c44 order (preferred), steps not specified in this SRD part:
    //   1. water / quicksand (WADE)            UNKNOWN: handled by the water module
    //   2. anti-magic and stud perception       UNKNOWN: square fields not given
    //   3. extinguisher, facing, rotator        UNKNOWN: square fields not given
    //   4. pit                                  handled by dungeon2b
    //   5. stairs caption, teleporter, chute    handled by dungeon2b / dungeon1b
    // Only automap recording is implemented in this part.
}

// Automap mapping (R48, engine preferred).
void mapSquareOnAutomap(Ctx& ctx, SquareState& square, bool fogEdge, bool wallBump) {
    (void)ctx;
    // R48 / c42 / c44: fog edges are not mapped in the grey view. A wall bump is recorded in fog.
    if (fogEdge && !wallBump) {
        return;
    }
    square.explored = true;
    // UNKNOWN: encoding of the wall-bump record in square fields is not given. Not stored here.
    (void)wallBump;
}

}  // namespace mordor
