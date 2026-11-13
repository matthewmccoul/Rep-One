#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"

namespace mordor {

// Fn13ED experience cap = xpThreshold(level+1) - 1; ±1 vs Fn12A1 at about 0.2% of cases [c3][c34]
i64 experienceCap(i32 level, double guildExpFactor, double raceExpFactor);
// T13 join cost for a guild, as Currency (1/10000 gold) [c104]
Currency guildJoinCost(Guild guild);



} // namespace mordor
