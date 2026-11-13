#include "test_support.h"
#include "mordor/state_common.h"

using namespace mordor;

// t121: none of the eight reference cases can be expressed with the declarations
// currently available. Each needs a round or attack resolver (MonsterAttacks,
// RoundPaced, ChestRounds) that is not declared in our headers. The enum AttackKind
// here (Moved, ChangedLevel) does not match the reference AttackKind (Round, ...),
// so the reference's Round kind has no direct counterpart yet.

// SKIP B_0063#14: needs a monster-attack round resolver with stat-drain effect; not declared (see NEED)
// SKIP B_0063#15: needs a monster-attack round resolver with stat-drain effect; not declared (see NEED)
// SKIP B_0063#16: needs a monster-attack round resolver with max-hits drain; not declared (see NEED)
// SKIP B_0063#17: depends on the drain counts of B_0063#14-16; not declared (see NEED)
// SKIP B_0056#6: needs a RoundPaced-style round with a Defend order and companion breath; not declared (see NEED)
// SKIP B_0056#7: needs group-record reload behaviour (MaxHits per front) from a round resolver; not declared (see NEED)
// SKIP B_0065#34: needs a hitter attack with chest-count damage modifier input; not declared (see NEED)
// SKIP B_0065#35: needs a hitter attack with chest-count damage modifier input; not declared (see NEED)

