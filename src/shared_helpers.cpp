#include "mordor/shared_helpers.h"
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
#include "mordor/types.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"
#include "mordor/rng.h"
#include "mordor/monsters1a.h"
#include "mordor/math_util.h"
#include "mordor/game_data.h"
#include "mordor/ctx.h"

namespace mordor {
u32 uidx(i32 i) {
    return static_cast<u32>(i);
}

// Words after the subcommand, joined with single spaces (names may be typed in several words).
Str seerJoinWords(const Vec<Str>& w, size_t from) {
  Str out;
  for (size_t i = from; i < w.size(); ++i) {
    if (i > from) out += ' ';
    out += w[i];
  }
  return out;
}

Str bufferInt(i64 v) {
  if (v == 0) return "0";
  bool neg = v < 0;
  if (neg) v = -v;
  Str out;
  while (v > 0) {
    out.insert(out.begin(), static_cast<char>('0' + (v % 10)));
    v /= 10;
  }
  if (neg) out.insert(out.begin(), '-');
  return out;
}

const Character& memberAt(const Party& p, i32 i) {
    return p.members[static_cast<u32>(i)];
}

}  // namespace mordor
