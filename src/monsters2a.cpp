#include "mordor/monsters2a.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

// [c9][c24][c66] Respawn terms: base 1200 s, 150 s per dungeon level (test set: per floor).
constexpr i32 kRespawnBaseSeconds = 1200;
constexpr i32 kRespawnPerLevelSeconds = 150;
// [c9][c66] Minimum respawn delay, seconds.
constexpr i32 kRespawnFloorSeconds = 120;
// [c19] Quest: class head start, 40%.
constexpr i32 kQuestHeadStartPercent = 40;
constexpr i64 kMsPerSecond = 1000;

}  // namespace

}  // namespace mordor
