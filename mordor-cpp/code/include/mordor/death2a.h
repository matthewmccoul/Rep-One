#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"

namespace mordor {


// R25 complication roll on a failed raise. Guaranteed case returns true without a draw;
// otherwise complicates if Int(Rnd*100) < chance%.
// UNKNOWN: whether the "rng.Next(100) < chance%" comparison is kept as a real comparison (assumed).
bool rollComplication(Ctx& ctx, i32 ageDays, i32 raceLifespanYears);

// ---- R26 constitution-loss roll
// Natural-CON loss chance, percent, floored at 0 (stone path, every raise).
i32 conLossChancePercent(i32 naturalCon);

// R26 morgue-path threshold, unfloored: 50 - Con * 2 (kind 23 only).
// Outcome is identical to the floored value for every roll, because Int(Rnd*100) >= 0.
i32 morgueConLossThreshold(i32 con);

// R26 loss test: loses 1 natural CON if Int(Rnd*100) < threshold. Returns true on loss.
// Mutates ch.abilities.stats[Con] on loss.
// UNKNOWN: whether natural CON may fall below 1 or another floor applies (not stated).
bool rollConLoss(Ctx& ctx, Character& ch, i32 threshold);

// R26 stone-path CON-loss roll, run on every raise. Threshold = conLossChancePercent(natural CON).
// CONFLICT 27: engine says stone costs 1 CON on any raise at raise time; archive says at death.
// Engine preferred.
bool stoneConLossRoll(Ctx& ctx, Character& ch);

// R26 morgue-path CON-loss roll. Only when ch.raiseKind == RaiseKind::Morgue (kind 23).
// Otherwise no draw is consumed and the result is false.
bool morgueConLossRoll(Ctx& ctx, Character& ch);

// ---- Table 4 age added by cause on a raise (days)
// Stoned -> 84, Rock -> 124, any other cause -> 54.
// CONFLICT 5: engine gives quiet failure 54 whatever the cause; FIDELITY applies cause to all raises.
// Engine preferred: callers pass a non-stone, non-rock cause for quiet failures.
// UNKNOWN: whether the complicated-failure age (kComplicationAgeDays) replaces or adds to this value.
i32 raiseAgeDays(DeathState cause);

// NOT DECLARED HERE:
// R27 (on-spot resurrection), R28 (retreat), R31-R33 (player-view summaries): no new routines;
//   engine equivalents are R12 and R16 (raise flow, declared elsewhere).
// R29 (body recovery): engine R10, declared elsewhere.
// R30 (morgue revival fee, guild payment): CONFLICT 20, CONFLICT 21; engine R15/R16 paths, declared elsewhere.
// R34 (Exchange Items with a dead character): already declared (items2b.h ExchangeOutcome / SwapRequest).
// R35 (save-file handling): design note, unverified; not declared.

}  // namespace mordor
