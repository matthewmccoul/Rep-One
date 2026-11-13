#include "test_support.h"
#include "mordor/loot1a.h"

// SKIP B_0055#131: CharStatFile.FormatNumber has no equivalent in the headers
// SKIP B_0055#141: CombatEngine.PayGold has no equivalent in the headers
// SKIP B_0056#24: MDATA3 table iteration and ItemDef-to-Item mapping are not available (ItemDef fields unknown)
// SKIP B_0056#26: CombatEngine.GoldLine has no equivalent in the headers
// SKIP B_0056#27: CombatEngine.PayGold has no equivalent in the headers
// SKIP B_0056#28: CombatEngine.PayGold has no equivalent in the headers

// Lead monster Id is passed directly as the `lead` argument; 42 stands in for the lead's Id.
TEST(engine_B_0057_1_chest_find_lead_own_id) {
    CHECK_EQ(mordor::chestFindMonsterId(42, false, false), 42);
}

TEST(engine_B_0057_2_chest_find_other_index_gives_minus_two) {
    CHECK_EQ(mordor::chestFindMonsterId(42, true, false), -2);
}

// SKIP B_0057#3: chestFindMonsterId has no input for the lead's invisibility
// SKIP B_0057#4: chestFindMonsterId has no input for the lead's invisibility
