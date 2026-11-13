#include "test_support.h"

#include "mordor/casting1.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

// B_0064#7: each character has 10 spell buffers.
TEST(engine_B_0064_7_spell_buffer_count) {
    CHECK_EQ(mordor::kSpellBuffers, 10);
}

// B_0065#36: no equivalent; needs a combat round with a wall monster and message capture.
// SKIP B_0065#36: needs a full combat round simulation and message text inspection.

// B_0065#37: no equivalent; needs a combat round and named message inspection.
// SKIP B_0065#37: needs a full combat round simulation and message text inspection.

// B_0065#38: standing orders revert to Fight when the caster lacks spell points.
// Assumes the standing cast order starts as CombatOrderKind::Buffer.
TEST(engine_B_0065_38_standing_orders_fight) {
    mordor::CombatOrder m0;
    m0.kind = mordor::CombatOrderKind::Buffer;
    mordor::enforceStandingOrderSp(m0, 0, 5);
    CHECK(m0.kind == mordor::CombatOrderKind::Fight);

    mordor::CombatOrder m1;
    m1.kind = mordor::CombatOrderKind::Buffer;
    mordor::enforceStandingOrderSp(m1, 0, 5);
    CHECK(m1.kind == mordor::CombatOrderKind::Fight);
}

// B_0065#39: no equivalent; the target group is set by the round, not by a header function.
// SKIP B_0065#39: target group is assigned during a combat round simulation.

// B_0065#55: no equivalent; magic-immune failure path is not covered by refusedCastOrder
// (which keys on antimagic square and hadSpell).
// SKIP B_0065#55: magic-immune cast failure needs a combat round, no header equivalent.

// B_0066#22: SpellBuffers.Describe is not in the headers.
// SKIP B_0066#22: SpellBuffers.Describe has no equivalent function in the headers.

// B_0066#23: SpellBuffers.Describe is not in the headers.
// SKIP B_0066#23: SpellBuffers.Describe has no equivalent function in the headers.

// B_0066#24: SpellBuffers.CostText is not in the headers.
// SKIP B_0066#24: SpellBuffers.CostText has no equivalent function in the headers.

// B_0066#25: SpellBuffers.Describe and CostText are not in the headers.
// SKIP B_0066#25: SpellBuffers.Describe and CostText have no equivalent functions in the headers.
