#include "test_support.h"
#include "mordor/death1b.h"
#include "mordor/game_data.h"

using namespace mordor;

// FIXED: The namespace `teleporting` and its function `forSpell` are not declared in any
// header in the API index, and TeleportRockOutcome has no member `IntoRock`. Those names
// cannot be used without inventing declarations, so the test body is disabled. Restore it
// once the teleporting API and the IntoRock enumerator exist.
#if 0
// B_0063#110: a teleport decided to one level past the deepest level is into rock.
TEST(engine_rtt105_teleport_one_past_deepest_level_is_rock) {
  const auto& floors = tsup::data().floors;
  const i32 deepest = static_cast<i32>(floors.size());
  auto out = teleporting::forSpell(floors, deepest + 1, 0, 0);
  CHECK(out == TeleportRockOutcome::IntoRock);
}
#endif

// SKIP B_0063#104: needs the coordinates of a rock square on shipped floor 1 and a
// Floor rock query (Floor.Has(x, y, ROCK)); neither is in the declarations provided.
// The TeleportTo offset (right=0, forwards=+1, facing North, level 1) is otherwise expressible.
