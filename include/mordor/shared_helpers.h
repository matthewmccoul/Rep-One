#pragma once
// One shared copy of helpers that several modules had each written for themselves.
#include "mordor/types.h"
#include "mordor/game_data.h"
#include "mordor/store_prices.h"
#include "mordor/progression2b.h"
#include "mordor/progression2.h"
#include "mordor/partyb.h"
#include "mordor/items1a.h"
#include "mordor/guilds1.h"
#include "mordor/city1.h"
#include "mordor/items1b.h"
#include "mordor/state_common.h"
#include "mordor/game.h"
#include "mordor/casting1.h"
#include "mordor/access.h"
#include "mordor/features.h"
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
Str seerJoinWords(const Vec<Str>& w, size_t from);
Str bufferInt(i64 v);
const Character& memberAt(const Party& p, i32 i);
}  // namespace mordor
