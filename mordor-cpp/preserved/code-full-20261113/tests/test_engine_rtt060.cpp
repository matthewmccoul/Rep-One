// t060: translation of reference assertions B_0056#13, B_0056#14 (CombatMonster.Unrolled via PlayerSwings)
// and B_0061#122..#126 (SwingsAt). None of these can be checked against our API yet, because the
// declarations they need are not in the provided set.
//
// NEED: CombatEngine::playerSwings (or the equivalent swing loop), CombatMonster with
//       unrolled/hits/maxHits/group fields and a front-of-group accessor, the group-setup
//       function that builds three one-handed-blade foes, and swingsAt (guild extra-swing lookup
//       backed by SwingLevelRow in guilds2.h).

#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0056#13: needs CombatEngine::playerSwings and CombatMonster.unrolled, neither declared.
//   Expected: 0 runs where the new front is still unrolled after the front dies (200 runs).
// SKIP B_0056#14: needs CombatEngine::playerSwings and CombatMonster.unrolled, neither declared.
//   Expected: 0 foes standing behind the new front that are not unrolled (200 runs).
// SKIP B_0061#122: needs swingsAt(guild, level) to return extra-swing counts; declaration not provided.
//   Expected pairs (level/level-1): 1/0 2/1 3/2 4/3 for Ninja at levels 56, 122, 267, 585.
// SKIP B_0061#123: needs swingsAt(guild, level); declaration not provided.
//   Expected pairs (level/level-1): 1/0 2/1 for Warrior at levels 91, 324.
// SKIP B_0061#124: needs swingsAt(guild, level); declaration not provided.
//   Expected pairs (level/level-1): 1/0 2/1 for Villain at levels 105, 434.
// SKIP B_0061#125: needs swingsAt(guild, level); declaration not provided.
//   Expected pairs (level/level-1): 1/0 2/1 for Paladin at levels 111, 484.
// SKIP B_0061#126: needs swingsAt(guild, level); declaration not provided.
//   Expected pairs (level/level-1): 1/0 2/1 for Scavenger at levels 116, 531.

