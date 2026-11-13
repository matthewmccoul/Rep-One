#include "test_support.h"
#include "mordor/loot1a.h"

// SKIP B_0055#73: CharStatFile.FormatNumber has no equivalent; formatWholeGold does not take the same rounding/format inputs
// SKIP B_0055#74: CharStatFile.FormatNumber culture/pattern comparison; no equivalent in headers
// SKIP B_0055#75: CharStatFile.FormatNumber bracket formatting; no equivalent in headers
// SKIP B_0055#76: CharStatFile.FormatNumber Currency vs Long text comparison; no equivalent in headers

TEST(engine_B_0055_96_gold_share_paid) {
    // 5 gold shared between 2 members: each gets 2 gold (Currency = gold * 10000)
    CHECK_EQ(mordor::goldSharePaid(50000, 2), 20000);
}

// SKIP B_0055#97: GoldShare.Line text is an event message; event text accessor on EventLog is not in the headers shown

TEST(engine_B_0055_98_gold_share_paid) {
    // 7 gold shared between 2 members: paid share is 4 gold
    CHECK_EQ(mordor::goldSharePaid(70000, 2), 40000);
}
// SKIP B_0055#98 (line part): GoldShare.Line text is an event message; event text accessor on EventLog is not in the headers shown

TEST(engine_B_0055_99_gold_share_paid_and_shown) {
    // 10 gold shared between 3 members: paid share is 3 gold, shown figure is 3.3333 gold (Currency units)
    CHECK_EQ(mordor::goldSharePaid(100000, 3), 30000);
    CHECK_EQ(mordor::goldShareDisplayed(100000, 3), 33333);
}
// SKIP B_0055#99 (line part): GoldShare.Line text is an event message; event text accessor on EventLog is not in the headers shown

// SKIP B_0055#100: GoldAfterClass whole-gold rounding of class-scaled chest gold is not specified by scaleGoldForClass (Currency result only); units ambiguous

// SKIP B_0055#130: Purse of MaxValue on hand plus MaxValue banked overflows Currency (i64) in Character::purse(); no saturating equivalent in headers
