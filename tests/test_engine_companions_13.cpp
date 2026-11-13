#include "test_support.h"

#include "mordor/companions1a.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"

using namespace mordor;

// SKIP B_0065#68: needs a seeded 1500-round simulation of Blue Flame mishaps; no single-call equivalent.
// SKIP B_0065#69: death cry playback is a sound/log effect over a simulation; no single-call equivalent.
// SKIP B_0065#70: log ordering of death cry vs. 'accidently cast' line needs a full round simulation.
// SKIP B_0065#71: party sound audibility (No Party Sound test) is not exposed by a function in the headers.
// SKIP B_0065#72: party sound audibility (No Party Sound test) is not exposed by a function in the headers.
// SKIP B_0065#81: the expected binding level depends on the Charm Humanoid spell level and monster floor data, which are not specified in the case.
// SKIP B_0065#85: needs a 30000-hit monster id from game data and a specified failing RNG outcome; neither is given.
// SKIP B_0065#87: needs a one-hit matching monster id from game data; the id is not given in the case.
// SKIP B_0065#89: the join message text is not returned by any function in the headers; only void log output exists.

