// Module companions1a: companion slots and identity, binding scale, join offers,
// Confinement pen, sale, and combat mishap / breath / spit rules (SRD c12p1).
//
// ASSUMPTIONS (game_data.h and the GameState layer were not supplied to this ant;
// verify against the generated headers):
//   ctx.data.monsters        : Vec<MonsterDef>
//   MonsterDef.id            : i32   (creature identifier, not unique)
//   MonsterDef.alignment     : i32   (0 Good, 1 Neutral, 2 Evil)
//   MonsterDef.levelFound    : i32
//   MonsterDef.guildLevel    : i32
//   MonsterDef.hits          : i32   (rolled-maximum source, "M4EAC" in docs)
//   ctx.rng.below(n) returns an integer; ctx.rng.percent(p) returns bool.
//   ctx.say(code, text, nums, strs) with empty braces for nums/strs.
//   GameState is opaque here; no field is read through it.
#include <cmath>

#include "mordor/companions1a.h"
#include "mordor/events.h"
#include "mordor/rng.h"
#include "mordor/items2b.h"

namespace mordor {

namespace {

constexpr i32 kPenShelfCap = 9;  // R19: mc0298 (pen shelf cap constant)
constexpr i32 kBreathChanceBase = 50;  // R23.2 base fire roll (c28)

}  // namespace

}  // namespace mordor
