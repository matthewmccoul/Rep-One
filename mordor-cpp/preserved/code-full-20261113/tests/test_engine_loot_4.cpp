#include "test_support.h"
#include "mordor/loot1a.h"

// Case ids contain '#', which is not valid in a C++ identifier, so test names
// replace it with '_'. The B_0059 gold path (GoldAnswer, GoldFromTag, GiveCommand.Val)
// has no equivalent in the new headers. The only typed-gold entry point is
// mordor::typedGoldWhole(double), which takes the already-parsed number, so
// string-input cases are skipped.

// SKIP B_0059#11: GoldAnswer/GoldFromTag tag parsing has no equivalent function in the headers
// SKIP B_0059#12: GiveCommand.Val string parsing ("1e3", "&H10", etc.) has no equivalent in the headers; typedGoldWhole takes a double
// SKIP B_0059#13: full gold path from string input ("1e3") has no string-parsing equivalent; giveGold takes whole gold only
// SKIP B_0059#14: full gold path from string input ("1e9") has no string-parsing equivalent; giveGold takes whole gold only
// SKIP B_0059#15: full gold path from string input ("1D2") has no string-parsing equivalent in the headers
// SKIP B_0059#16: full gold path from string input ("&H10") has no string-parsing equivalent in the headers
// SKIP B_0059#17: full gold path from string input (" 12") has no string-parsing equivalent in the headers
// SKIP B_0059#18: "type mismatch" from CLng after Val has no equivalent error path in the headers

TEST(engine_B_0059_19_negative_typed_gives_zero) {
    // Post-parse value for "-5" is -5.0; the case expects 0 gold transferred.
    CHECK_EQ(mordor::typedGoldWhole(-5.0), 0);
}

TEST(engine_B_0059_20_empty_typed_gives_zero) {
    // Post-parse value for "" is 0.0; the case expects 0 gold transferred.
    CHECK_EQ(mordor::typedGoldWhole(0.0), 0);
}
