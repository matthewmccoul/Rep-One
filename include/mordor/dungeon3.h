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













// FIXED: removed declaration `i32 rollRotatorFacing(Ctx& ctx);` (R42, 0-based port rotator draw).
// It conflicted with an existing declaration of rollRotatorFacing(Ctx&) in the same namespace that
// has a different return type ("ambiguating new declaration"). The existing declaration in the
// other header is the one to use; its return type must be checked against the R42 meaning.




// R47 Lost-state recovery step: found yourself (order within a step UNKNOWN)
void foundYourself(Ctx& ctx, Character& who);

// R47 Lost-state recovery attempt; returns true when the character works it out (order UNKNOWN)
bool tryToWorkItOut(Ctx& ctx, Character& who);

// R47 Square effects in order (CONFLICT, c44 preferred): water/quicksand, anti-magic and stud perception, extinguisher, facing, rotator, pit, stairs caption, teleporter, chute
void applySquareEffects(Ctx& ctx, Arr<Character, kPartyMaxSize>& members, i32 partySize, FloorState& floor, Location& loc);

// R48 Automap update for one square: fog edges not mapped, wall bump recorded in fog (engine preferred over wiki)
void mapSquareOnAutomap(Ctx& ctx, SquareState& square, bool fogEdge, bool wallBump);





// R44 No routine: wall-bit numbering 19–22 is governed by the FloorFlag table in game_data.h (original UNKNOWN)
// R46 No routine: lost-flag readout is a text screen, out of scope (CONFLICT: "[pos]" vs "(Lost)")
// R54 No routine: stud spawning engine detail not in findings (UNKNOWN)
// R55 No routine: progression beyond 15 levels has no formula (UNKNOWN)
// R56 No routine: Feather Essence level unresolved (CONFLICT: L3 vs L5)
// R57 No routine: room state persistence unresolved (CONFLICT: port write-back vs seed regeneration)
// R58 No routine: naming only; see R22 and R31 in the other part
// R60 No routine: math vocabulary (Rnd range UNKNOWN; Log assumed natural; B(...) undefined)

}  // namespace mordor
