// Definition of a routine that progression2.h declares but no module implemented (found by the link check).
#include "mordor/progression2.h"
#include "mordor/state_game.h"

namespace mordor {

// ASSUMED (SRD ch.5/6: the play clock adds elapsed seconds to each member's PlaySeconds; time of day wraps at 86400 s).
// playSeconds is kept in 1/10000 s, playStart is the last tick as seconds of the day.
void playClockTick(GameState& game, i64 nowMs) {
  i64 nowSec = (nowMs / 1000) % 86400;
  for (i32 i = 0; i < game.party.size && i < kPartyMaxSize; ++i) {
    Character& c = game.party.members[static_cast<size_t>(i)];
    i64 d = nowSec - c.playStart;
    if (d < 0) d += 86400;
    c.playSeconds += d * 10000;
    c.playStart = nowSec;
  }
}

}  // namespace mordor
