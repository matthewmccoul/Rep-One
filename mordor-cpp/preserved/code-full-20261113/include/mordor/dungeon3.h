#pragma once
// dungeon3.h: SRD part c15p3 (Dungeon Exploration: perception, identification,
// facing, coordinates, automap, dungeon build, square-effect helpers).
//
// UNKNOWN: state_character.h, state_common.h, state_item.h and state_dungeon.h
// have no include guards, and state_party.h includes state_character.h again.
// This header therefore includes state_dungeon.h only, and takes the party as
// Arr<Character, kPartyMaxSize> + partySize instead of Party.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_dungeon.h"

namespace mordor {


// R12 True when a teleporter draw value is in the excluded MAZEMOD range 1798–1808 (inclusive, assumed)
bool mazeModExcluded(i32 mazeMod);

// R21 Limited build test by dungeon level count only (CONFLICT: engine uses three tests; level count preferred)
bool isLimitedBuild(i32 dungeonLevelCount);

// R31 Rescue chance in percent: max(16 - L, 0.1), plus 16 - L when easyFind, capped at 100 on level 1 areas 18 and lower (order UNKNOWN)
double rescueChance(i32 dungeonLevel, bool easyFind, i32 area);

// R31 Rescue roll against a percent chance
bool rollRescue(Ctx& ctx, double chance);

// R34 Trap disarm log term ln(max(2, M)) (CONFLICT: c97 uses ln(mLvl) with no max; c24 preferred)
double trapDisarmLogTerm(double m);

// R34 Disarm stat WIS/4 + DEX/2 (real division assumed; UNKNOWN: rounding and gear rule)
double disarmStat(i32 wis, i32 dex);

// R37 Notice roll: passes when rng.Next(100) + 1 <= perception (CONFLICT: P = 100 and 200; rotator roll UNKNOWN)
bool noticePasses(Ctx& ctx, i32 perception);

// R38 Meeting in fog clears identification (M4EEE meaning UNKNOWN; fog flag from code [c5])
void clearIdentificationOnFogMeeting(Ctx& ctx, Creature& monster, bool fogPresent);

// R39 Wrong-guess bit set (UNKNOWN: which bits; "eight bits always pass" conflicts with exclusions)
u32 setWrongGuessBits(Ctx& ctx, u32 bits);

// R40 Seer fee charge through charPay mechanics (UNKNOWN: fee amount and deduction order)
bool seerCharge(Ctx& ctx, Character& payer, Currency fee);

// R41 Blur spread in squares: 4 for monsters; other kinds UNKNOWN (returns 4 as simplest choice)
i32 blurSpreadSquares(bool isMonster);

// FIXED: removed declaration `i32 rollRotatorFacing(Ctx& ctx);` (R42, 0-based port rotator draw).
// It conflicted with an existing declaration of rollRotatorFacing(Ctx&) in the same namespace that
// has a different return type ("ambiguating new declaration"). The existing declaration in the
// other header is the one to use; its return type must be checked against the R42 meaning.

// R42 Dead-body facing from lead facing: (leadFacing + 3) & 3 (base UNKNOWN: 0- or 1-based)
i32 deadBodyFacing(i32 leadFacing);

// R43 Facing arrow cell: facing + 22 (CONFLICT: cell 23 also the RESCUE floor bit)
i32 facingArrowCell(i32 facing);

// R45 Chute lookup square: (x + 1, y + 1) (UNKNOWN: whether +1 applies at the API boundary)
// FIXED: SquareCoord does not exist anywhere in the API index; replaced by Location, the closest
// existing position type (used by stepFacing and detectRockAhead in casting1.h).
Location chuteLookupSquare(i32 x, i32 y);

// R47 Lost-state recovery step: found yourself (order within a step UNKNOWN)
void foundYourself(Ctx& ctx, Character& who);

// R47 Lost-state recovery attempt; returns true when the character works it out (order UNKNOWN)
bool tryToWorkItOut(Ctx& ctx, Character& who);

// R47 Square effects in order (CONFLICT, c44 preferred): water/quicksand, anti-magic and stud perception, extinguisher, facing, rotator, pit, stairs caption, teleporter, chute
void applySquareEffects(Ctx& ctx, Arr<Character, kPartyMaxSize>& members, i32 partySize, FloorState& floor, Location& loc);

// R48 Automap update for one square: fog edges not mapped, wall bump recorded in fog (engine preferred over wiki)
void mapSquareOnAutomap(Ctx& ctx, SquareState& square, bool fogEdge, bool wallBump);

// R49 Rotator loss: position and facing lost, floor kept (which lost flags are set UNKNOWN)
void applyRotatorLoss(Ctx& ctx, Character& who);

// R52 Stairs-up destination level (0 is town; engine preferred over wiki "down only")
i32 stairsUpDestination(i32 dungeonLevel);

// R53 Stud spread: 1 when Rnd*75 < 50, otherwise 0 (port followed; original UNKNOWN)
i32 rollStudSpread(Ctx& ctx);

// R59 Ethereal Portal: step one square forward, then test the square stepped onto (rock = death, engine preferred over wiki)
void applyEtherealPortal(Ctx& ctx, Arr<Character, kPartyMaxSize>& members, i32 partySize, const FloorState& floor, Location& loc);

// R44 No routine: wall-bit numbering 19–22 is governed by the FloorFlag table in game_data.h (original UNKNOWN)
// R46 No routine: lost-flag readout is a text screen, out of scope (CONFLICT: "[pos]" vs "(Lost)")
// R54 No routine: stud spawning engine detail not in findings (UNKNOWN)
// R55 No routine: progression beyond 15 levels has no formula (UNKNOWN)
// R56 No routine: Feather Essence level unresolved (CONFLICT: L3 vs L5)
// R57 No routine: room state persistence unresolved (CONFLICT: port write-back vs seed regeneration)
// R58 No routine: naming only; see R22 and R31 in the other part
// R60 No routine: math vocabulary (Rnd range UNKNOWN; Log assumed natural; B(...) undefined)

}  // namespace mordor
