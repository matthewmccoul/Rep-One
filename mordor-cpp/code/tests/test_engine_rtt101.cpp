// Batch t101: translations of B_0065 CombatEngine.CastSpell assertions.
// None of the cases can be expressed with the declarations available yet
// (no CombatEngine, Spell, or monster-group API, and no charm/CastGate API),
// so each is skipped with its reason. No expected value was altered.

#include "test_support.h"
#include "mordor/state_common.h"
#include "mordor/casting1.h"

using namespace mordor;

// SKIP B_0065#53: needs CombatEngine::CastSpell, monster groups and LastCaptions, none declared in our API
// SKIP B_0065#54: needs CombatEngine::CastSpell and the monster property bit 2 (magic immunity) check, not declared
// SKIP B_0065#45: needs CombatEngine::CastSpell and Monsters list (alive count), not declared
// SKIP B_0065#51: needs CombatEngine::CastSpell with a null target and LastCaptions, not declared
// SKIP B_0065#44: needs CombatEngine::CastSpell for Flamesheet, the group hits API and emptied slots, not declared
// SKIP B_0065#82: needs a charm path (CastSpell with Companions slot check) and Monsters[0].Gone, not declared
// SKIP B_0065#84: needs a failed-charm path and Monsters[0].Hits, not declared
// SKIP B_0065#88: needs the refused-charm path (CastGate refusal before spending) and Monsters[0].Hits, not declared

