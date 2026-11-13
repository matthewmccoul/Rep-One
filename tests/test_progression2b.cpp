#include "test_support.h"
#include "mordor/progression2b.h"

// Expectations are derived from the SRD tables and rules for progression2b.
// Currency is fixed point at 10000 units per gold piece.
// Not covered, because the SRD leaves the behaviour UNKNOWN or the API is not indexed:
//   announceLevelThresholdCrossed (EventLog query API not in the index),
//   hitsGainForLevel (guild hit values UNKNOWN),
//   naturalDeathChancePercent / rollNaturalDeath (comparison direction UNKNOWN),
//   applySanctuary (target and amount UNKNOWN for the port),
//   guildJoinCost (ability and spell inputs UNKNOWN; table is archive-sourced).

using namespace mordor;

namespace {

void setGuildLevel(Character& ch, Guild g, i32 lvl) {
    ch.guilds[static_cast<u32>(g)].level = lvl;
}

} // namespace

// ---- Hit cap (SRD T10: MaxHits = min(MaxHits, 999)) ----

// ---- Rest age cost (SRD T9, rule B((100 - 100*cur/max)/3.25)) ----

// ---- Poison and disease clear thresholds (SRD T9) ----

// ---- Old-age onset at 98% of racial lifespan (SRD T9) ----

// ---- Highest guild (strict comparison against every other guild) ----

// ---- Level-up gold: 30.1 * L^2 * (1 + ggf/10), cumulative at current level (SRD T7, T10) ----

// ---- Experience cap: rises with level and with the factors (SRD T7, T10, conflicts 13-14) ----

// ---- Rest aging applied to the character (SRD T9) ----

// ---- Poison cure: success clears and costs 2 HP; failure leaves poison (SRD T9) ----

// ---- Disease cure: success clears and costs 1 HP; failure leaves disease (SRD T9) ----

