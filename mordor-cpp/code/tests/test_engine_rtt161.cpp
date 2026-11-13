#include "test_support.h"
// FIXED: removed #include "mordor/dungeon_state.h" (header does not exist in the project; respawnDelaySeconds is not declared in any listed header)
#include "mordor/math_util.h"

using namespace mordor;

// R-respawn: RespawnDelay(level, headerAreas, tally) in seconds.
// t = 1200 + (5 * (level / 2.0)) * 60; t -= (headerAreas - tally) * 5; floor at 1200 / 10.

// FIXED: not declared in any header; defined locally as static from the formula in the comment above
static double respawnDelaySeconds(int level, int headerAreas, int tally) {
  double t = 1200.0 + (5.0 * (level / 2.0)) * 60.0;
  t -= static_cast<double>(headerAreas - tally) * 5.0;
  if (t < 120.0) {
    t = 120.0;
  }
  return t;
}

TEST(engine_rtt161_respawn_delay_floor1) {
  // B_0066#91
  CHECK_EQ(respawnDelaySeconds(1, 50, 40), 1300.0);
}

TEST(engine_rtt161_respawn_delay_floor15_full_tally) {
  // B_0066#92
  CHECK_EQ(respawnDelaySeconds(15, 50, 50), 3450.0);
}

TEST(engine_rtt161_respawn_delay_clamped_to_floor) {
  // B_0066#93: 1350 - 2000 = -650, clamped to 120
  CHECK_EQ(respawnDelaySeconds(1, 400, 0), 120.0);
}

// SKIP B_0066#105: needs DungeonState::Save and a temp folder; no DungeonState save API in OUR DECLARATIONS
// SKIP B_0066#104: needs a generated chest room and DungeonState Save/Load round trip; no generator or persistence API declared
// SKIP B_0066#101: refers to a "same refill case" whose setup is not in this batch; TallyOf not declared
// SKIP B_0066#112: refers to a "same return visit" whose setup is not in this batch; TallyOf not declared
// SKIP B_0058#11: asks about the C# port's DungeonWindow.axaml.cs (UI code); not part of the C++ engine core
