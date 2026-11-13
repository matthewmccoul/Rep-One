#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"

namespace mordor {









// NOT DECLARED HERE:
// R27 (on-spot resurrection), R28 (retreat), R31-R33 (player-view summaries): no new routines;
//   engine equivalents are R12 and R16 (raise flow, declared elsewhere).
// R29 (body recovery): engine R10, declared elsewhere.
// R30 (morgue revival fee, guild payment): CONFLICT 20, CONFLICT 21; engine R15/R16 paths, declared elsewhere.
// R34 (Exchange Items with a dead character): already declared (items2b.h ExchangeOutcome / SwapRequest).
// R35 (save-file handling): design note, unverified; not declared.

}  // namespace mordor
