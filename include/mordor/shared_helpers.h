#pragma once
// One shared copy of helpers that several modules had each written for themselves.
#include "mordor/types.h"
#include "mordor/game_data.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"
#include "mordor/rng.h"
#include "mordor/monsters1a.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"

namespace mordor {
u32 uidx(i32 i);
}  // namespace mordor
