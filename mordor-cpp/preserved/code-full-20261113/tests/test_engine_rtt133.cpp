#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// No TEST cases in this batch: none of the reference assertions can be expressed with the
// declarations provided. CombatEngine, CombatLine, CombatMonster and CombatSounds have no
// declarations in our code yet (see NEED below).

// SKIP B_0064#115: CombatEngine.RoundPaced and its CombatLine channel/text output are not declared in our code.
// SKIP B_0064#116: CombatEngine.RoundPaced and its CombatLine channel/text output are not declared in our code.
// SKIP B_0055#20: CombatMonster (with Def and Alignment) and the Morloch monster definition are not declared in our code.
// SKIP B_0055#21: CombatMonster (with Def and Alignment) and the Morloch monster definition are not declared in our code.
// SKIP B_0065#4: CombatSounds.OfLines and CombatEngine.CombatLine are not declared in our code.
// SKIP B_0065#6: CombatSounds.OfLines and CombatEngine.CombatLine are not declared in our code.
// SKIP B_0065#10: CombatSounds.OfLines and CombatEngine.CombatLine are not declared in our code.
// SKIP B_0065#69: CombatSounds.OfLines and CombatEngine.CombatLine are not declared in our code.
