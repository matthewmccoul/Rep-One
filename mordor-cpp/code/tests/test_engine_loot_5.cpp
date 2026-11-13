#include "test_support.h"
#include "mordor/loot1b.h"

using mordor::Currency;

// Currency = gold * 10000. Whole-gold expectations are converted to Currency.
// Case ids contain '#', which is not valid in a C++ identifier, so '#' is written as '_'.

// SKIP B_0059#21: typed text "1,000" needs locale-aware parsing; typedGoldWhole takes a double, and giveGold path depends on the current culture.

// SKIP B_0063#74: announcement is written to env.log; the EventLog read API is not in the headers, so "nothing printed" cannot be checked.

// SKIP B_0063#75: announcement text "Your share is 35 Gold!" is written to env.log; the EventLog read API is not in the headers, so the text cannot be checked.

// SKIP B_0065#13: needs a seeded simulation of 400 boxer fights (monster data, spoils, chest creation); no equivalent single function.

// SKIP B_0065#14: needs a seeded simulation of 400 boxer fights; no equivalent single function.

// SKIP B_0065#15: needs AwardSpoils, which has no equivalent in the headers.

// SKIP B_0065#16: needs PayGold and chest opening over a simulation; no equivalent in the headers.
