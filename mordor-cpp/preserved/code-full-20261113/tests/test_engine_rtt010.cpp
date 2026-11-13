#include "test_support.h"
#include "mordor/combat1b.h"
#include "mordor/companions1b.h"
#include "mordor/state_common.h"

using namespace mordor;

// SKIP B_0065#20: needs CombatEngine::RoundPaced with a party (m0 with a companion, m1 with a Fight order) and a monster list; no round-level combat engine or Party/Monster state types are declared in our headers, so "does m1 act after m0's companion kills the last monster" cannot be set up or observed.

// SKIP B_0065#19: needs CombatEngine::RoundPaced run over 200 seeded rounds with companion id 30100 (900 hits, 900 attack) and a no-weapon Fight order against a one-hit monster; the seeded round driver and companion-turn resolution are not in our declarations. Once available, the check is CHECK(killedRounds >= 150) with the expected value "at least 150" kept verbatim.

