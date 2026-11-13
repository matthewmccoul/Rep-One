// Batch t090: translated from reference assertions in CombatEngine / CastGateProbe / Party orders.
// None of these can be expressed against our current declarations: they need the CombatEngine
// round loop, the CombatOrder/Party order store, the monster state, and the help-label probe.
// Each one is recorded as SKIP below with its reason; no expected value was altered.

#include "test_support.h"
#include "mordor/casting1.h"

using namespace mordor;

// SKIP B_0061#30: needs CastGateProbe.Compare (help COMBAT/NON-COMBAT labels vs fn149E over MDATA2); no help-label table or fn149E-from-MDATA2 declaration in our API.
// SKIP B_0061#31: needs CastGateProbe.Compare agreement count (91 or more); same missing help-label probe.
// SKIP B_0061#98: needs CombatEngine.CastSpell for a Healer at guild level 40 casting Heal in a fight, and the returned message text; CombatEngine is not declared (CastRefusal members unknown).
// SKIP B_0056#5: needs 300-run Monte Carlo over CombatEngine.CastSpell with item-level charm, CombatMonster Alive/AtNought/Hits/Unrolled; none declared.
// SKIP B_0065#55: needs CombatEngine.CastSpell (Firebolt vs magic-immune monster) and Party.OrderFor(caster).kind after the failed cast; Party order store and CombatOrder not declared.
// SKIP B_0061#93: needs CombatEngine.RoundPaced with a Healer ordered to Heal and the round's text containing the refusal line; RoundPaced and round text events not declared.
// SKIP B_0061#94: needs CombatEngine.RoundPaced and CombatOrder kind after the round for a Healer ordered to Heal; CombatOrder not declared.
// SKIP B_0061#95: needs CombatEngine.RoundPaced and CombatOrder target (group 0) after the round for a Healer ordered to Heal; CombatOrder not declared.
