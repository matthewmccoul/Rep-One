#include "test_support.h"

#include "mordor/status1a.h"
#include "mordor/status2.h"
#include "mordor/partyb.h"

using namespace mordor;

// B_0063: help cry threshold (HelpCry.Needed) via helpCryNeeded(party, c).

// B_0064: stoning companion blows. Each companionBlowStones call is treated as
// one landed blow; the percentage of those that stone the foe is checked.

// SKIP B_0064#117: needs a full combat simulation (group, rounds, last foe of group); companionBlowStones is a single blow check only.
// SKIP B_0064#118: needs combat log ordering (SHOCK sound after 'Stoned one!'); no equivalent in the headers.
// SKIP B_0064#119: needs a multi-round fight to verify the foe's hits stay at 0 after each stoning; no equivalent function.

// SKIP B_0065#74: castCureParalysis(ctx, caster, inFight) takes only the caster and has no target parameter, so "cast by Ann on Bob" cannot be expressed.

