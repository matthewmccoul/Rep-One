#include "test_support.h"
#include "mordor/combat1a.h"

using namespace mordor;

// SKIP B_0056#13: needs CombatEngine.PlayerSwings simulation loop (no equivalent in headers)
// SKIP B_0056#14: needs CombatEngine.PlayerSwings simulation loop (no equivalent in headers)
// SKIP B_0058#12: asks about port UI code (DungeonWindow.axaml.cs ChestTeleport); needs UI, not testable from C++ headers
// SKIP B_0061#122: SwingsAt has no direct equivalent; extraSwingsFromMultiple takes a multiplier m, and the header does not document how guild milestone levels map to m
// SKIP B_0061#123: SwingsAt has no direct equivalent; extraSwingsFromMultiple takes a multiplier m, and the header does not document how guild milestone levels map to m
// SKIP B_0061#124: SwingsAt has no direct equivalent; extraSwingsFromMultiple takes a multiplier m, and the header does not document how guild milestone levels map to m
// SKIP B_0061#125: SwingsAt has no direct equivalent; extraSwingsFromMultiple takes a multiplier m, and the header does not document how guild milestone levels map to m

TEST(engine_B_0061_112_hit_threshold_atk10_def0) {
    CHECK_EQ(hitThresholdPercent(10, 0), 98);
}

TEST(engine_B_0061_113_hit_threshold_atk0_def0) {
    CHECK_EQ(hitThresholdPercent(0, 0), 50);
}

TEST(engine_B_0061_114_hit_threshold_atk1_def1000) {
    CHECK_EQ(hitThresholdPercent(1, 1000), 2);
}
