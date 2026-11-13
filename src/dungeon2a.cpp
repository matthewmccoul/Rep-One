// dungeon2a.cpp: Chapter 15 dungeon exploration, part c15p2
// (exit, close, entry hooks, limited build, Seer).
// Pure helpers live in an anonymous namespace; they hold no state.

#include "mordor/dungeon2a.h"
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/events.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {
namespace {

// Number of Area records in FloorState::areas (Arr<Area, 201>).
// UNKNOWN: index base not stated; scanned as 0..200.
constexpr i32 kAreaSlots = 201;

// FloorFlag bit positions from the Field.Flags table (c13).
constexpr u32 kBitStairsUp = 12u;
constexpr u32 kBitStairsDown = 13u;
constexpr u32 kBitChute = 21u;

}  // namespace

// ---------------------------------------------------------------------------
// Action hooks (R123)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Stairs up and exit (R124, R125, R126, R127)
// ---------------------------------------------------------------------------

void climbToTownAlive(Ctx& ctx, Party& party) {
    // R124: the leader's position is placed at the party's square and the
    // party exits to town alive.
    // RESOLVED-BY-REFERENCE: town is dungeon level 0. Rescue.cs CarryHome: "carried to town -
    // DungeonLevel = 0". The town level is therefore 0, not the party's level.
    Location& at = party.members[0].location;
    at.x = party.x;
    at.y = party.y;
    at.dungeonLevel = 0;
    at.facing = static_cast<i32>(party.facing);
    party.inDungeon = false;
    party.onStairs = false;
    ctx.say("dungeon.climb.town", "You climb the stairs to town.", {}, {});
}

// ---------------------------------------------------------------------------
// Limited (shareware) build (R128 to R131)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Seer (R132 to R139)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Preserved depth formulas
// ---------------------------------------------------------------------------

}  // namespace mordor
