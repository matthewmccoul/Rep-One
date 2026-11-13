#include "test_support.h"
#include "mordor/loot1a.h"

using namespace mordor;

// B_0057 chest find report (chestFindMonsterId).

TEST(engine_B_0057_30_find_reports_lead_when_taken_for_self) {
    // Lead recognised then killed; chest taken for the lead's KnownAs index.
    // Taker is Neutral male, no See Invisible; lead is not taken for another.
    const i32 lead = 42;
    CHECK_EQ(chestFindMonsterId(lead, false, false), lead);
}

TEST(engine_B_0057_32_find_reports_minus_two_when_taken_for_other) {
    // Lead is the chest's monster; chest is taken for the other (KnownAs) monster.
    const i32 lead = 42;
    CHECK_EQ(chestFindMonsterId(lead, true, false), -2);
}

TEST(engine_B_0057_46_find_shadow_ninja_seen_by_see_invisible) {
    // Shadow Ninja (index 389) is the chest's monster, taken for itself
    // by a sage with See Invisible.
    const i32 lead = 389;
    CHECK_EQ(chestFindMonsterId(lead, false, true), 307);
}

// B_0059 gold giver tags (GiveCommand.GoldAnswer / GoldFromTag).
// The headers have no API that takes a typed string or returns a tag string;
// typedGoldWhole takes a double and giveGold takes Character/Purse arguments.

// SKIP B_0059#4: GiveCommand.GoldAnswer takes a typed string; no equivalent in headers
// SKIP B_0059#5: GiveCommand.GoldAnswer takes a typed string; no equivalent in headers
// SKIP B_0059#6: GiveCommand.GoldAnswer takes a typed string; no equivalent in headers
// SKIP B_0059#7: GiveCommand.GoldAnswer refusal text; no equivalent in headers
// SKIP B_0059#8: GiveCommand.GoldAnswer returns a tag string; no equivalent in headers
// SKIP B_0059#9: GiveCommand.GoldFromTag parses a tag string; no equivalent in headers
// SKIP B_0059#10: GiveCommand.GoldFromTag parses a tag string; no equivalent in headers
