#include "test_support.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"

// SKIP B_0061#126: no SwingsAt function in the headers; extraSwingsFromMultiple takes a multiplier and does not return the per-level pairs the reference asserts
// SKIP B_0061#165: no spell damage function (Rules.SpellDamage equivalent) in the headers
// SKIP B_0061#166: no spell damage function (Rules.SpellDamage equivalent) in the headers
// SKIP B_0061#167: no spell damage function (Rules.SpellDamage equivalent) in the headers
// SKIP B_0062#1: needs MdrLoader to load MDATA5.MDR; no loader in the headers
// SKIP B_0062#2: needs MdrLoader to load MDATA5.MDR; no loader in the headers
// SKIP B_0062#3: needs MdrLoader to load MDATA5.MDR; no loader in the headers
// SKIP B_0062#4: needs MdrLoader to load MDATA5.MDR; no loader in the headers

TEST(engine_B_0064_11_resist_kinds_count) {
    CHECK_EQ(mordor::kResistKinds, 11);
}

TEST(engine_B_0064_12_chest_trap_list_count) {
    // Trap ids run 0 (NotTrapped) through 10 (Trap10), so the count including NotTrapped is max id + 1.
    CHECK_EQ(static_cast<mordor::i32>(mordor::ChestTrap::Trap10) + 1, 11);
}
