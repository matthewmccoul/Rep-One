#include "test_support.h"
#include "mordor/death1b.h"
#include "mordor/death2b.h"

#include <cmath>

// Lifespan in years used throughout. 1 year = 365 days (kAgeStartDays = 16 * 365).
// 100 years = 36500 days, so 36500 days is 100% of max age.

TEST(old_age_constants_match_srd) {
    CHECK(mordor::kOldAgeStartPct == 98.0);
    CHECK(mordor::kOldAgeBaseChance == 5.0);
}

