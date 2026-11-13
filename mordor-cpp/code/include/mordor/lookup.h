#pragma once
// Table lookups by id. Items, monsters and spells are identified by id; their ids are not their table rows.
#include "mordor/game_data.h"

namespace mordor {
const ItemDef* itemDefById(const GameData& data, i32 id);
const MonsterDef* monsterDefById(const GameData& data, i32 id);
}  // namespace mordor
