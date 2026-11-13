#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0065#26: openingRounds (the chest-opening count, Combat.cs:703) is not in the declared API; needs NEED: openingRounds
// SKIP B_0055#141: CombatEngine / PayGold / VbCurrency (MaxValue, TryAdd) are not in the declared API; needs NEED: CombatEngine, VbCurrency
// SKIP B_0065#16: property over all chest fights needs a CombatEngine fight simulation; needs NEED: CombatEngine
// SKIP B_0056#27: GoldLine caption of CombatEngine::payGold is not in the declared API; needs NEED: CombatEngine
// SKIP B_0056#28: second payGold call and blank GoldLine caption need CombatEngine; needs NEED: CombatEngine
// SKIP B_0065#17: property over chest fights after a second PayGold call needs CombatEngine; needs NEED: CombatEngine
// SKIP B_0065#29: chest-open line count from roundPaced needs CombatEngine and a chest fixture; needs NEED: CombatEngine, roundPaced
// SKIP B_0065#31: which characters open the chest in round two needs CombatEngine and a chest fixture; needs NEED: CombatEngine, roundPaced
