#include "test_support.h"
#include "mordor/ref_respawn.h"

using namespace mordor;

// B_0066#90: floor 1, headerAreas 50, tally 50 -> no rooms emptied.
TEST(engine_ref_respawn_floor1_full_no_deficit) {
    CHECK_EQ(respawnDelay(1, 50, 50), 1350);
}

// B_0066#91: floor 1, headerAreas 50, tally 40 -> 10 rooms emptied, 5 s each subtracted.
TEST(engine_ref_respawn_floor1_ten_emptied) {
    CHECK_EQ(respawnDelay(1, 50, 40), 1300);
}

// B_0066#92: floor 15, headerAreas 50, tally 50 -> no rooms emptied.
TEST(engine_ref_respawn_floor15_full_no_deficit) {
    CHECK_EQ(respawnDelay(15, 50, 50), 3450);
}

// B_0066#93: floor 1, headerAreas 400, tally 0 -> deficit drives time below the 120 s floor.
TEST(engine_ref_respawn_floor1_clamped_to_minimum) {
    CHECK_EQ(respawnDelay(1, 400, 0), 120);
}
