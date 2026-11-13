#pragma once
// One shared copy of helpers that several modules had each written for themselves.
#include "mordor/types.h"
#include "mordor/game_data.h"
#include "mordor/death1a.h"
#include "mordor/clock.h"
#include "mordor/loot1a.h"
#include "mordor/progression1a.h"
#include "mordor/monsters1b.h"
#include "mordor/combat3.h"
#include "mordor/combat2a.h"
#include "mordor/combat1b.h"
#include "mordor/combat1a.h"
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
const ItemDef* itemDefById(const GameData& data, i32 id);
bool isDigits(const Str& t);
bool swapSlotInRange(i32 slot);
Str joinFrom(const Vec<Str>& w, std::size_t from);
Str glGet(const Session& s, const Str& key);
bool parseInt(const Str& t, i32& out);
Str toStr(i64 v);
bool confParseIndex(const Str& w, i32& out);
Str storeGoldText(Currency units);
}  // namespace mordor
