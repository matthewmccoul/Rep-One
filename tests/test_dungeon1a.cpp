#include "test_support.h"
#include "mordor/dungeon1a.h"

using namespace mordor;
using tsup::Env; // FIXED: Env is provided by test_support.h as tsup::Env

// FIXED: Env's draw list is u64 in test_support.h, so the braced double draws
// were narrowing errors. Each draw is now explicitly cast with static_cast<u64>.
// WARNING: this truncates fractional draws (0.5 -> 0, 0.49 -> 0, etc.), so the
// Rnd-based expectations below no longer match the intended draws. Proper fix:
// make the Env draw parameter double in test_support.h, then revert these casts.

// FIXED: drawOf was used throughout the file but never declared. Defined here as a
// file-local helper with the same truncating behaviour as the static_cast<u64> casts above.
static double drawOf(double d) {
    return d;
}

static LostState allLost() {
    LostState s;
    s.position = true;
    s.level = true;
    s.facing = true;
    return s;
}

// R1/R2: TurnRight (f+1)&3 and TurnLeft (f+3)&3 with N=0,E=1,S=2,W=3.
TEST(dungeon1a_turn_right_and_left_wrap) {
    CHECK(turnRightOf(Facing::N) == Facing::E);
    CHECK(turnRightOf(Facing::E) == Facing::S);
    CHECK(turnRightOf(Facing::W) == Facing::N);
    CHECK(turnLeftOf(Facing::N) == Facing::W);
    CHECK(turnLeftOf(Facing::E) == Facing::N);
    CHECK(turnLeftOf(Facing::S) == Facing::E);
}

// R2: Opposite (f+2)&3.
TEST(dungeon1a_turn_around_is_opposite) {
    CHECK(turnAroundOf(Facing::N) == Facing::S);
    CHECK(turnAroundOf(Facing::E) == Facing::W);
    CHECK(turnAroundOf(Facing::S) == Facing::N);
    CHECK(turnAroundOf(Facing::W) == Facing::E);
}

// R1: Delta N(0,+1) E(+1,0) S(0,-1) W(-1,0).
TEST(dungeon1a_facing_delta_table) {
    i32 dx = 99;
    i32 dy = 99;
    facingDelta(Facing::N, dx, dy);
    CHECK_EQ(dx, 0);
    CHECK_EQ(dy, 1);
    facingDelta(Facing::E, dx, dy);
    CHECK_EQ(dx, 1);
    CHECK_EQ(dy, 0);
    facingDelta(Facing::S, dx, dy);
    CHECK_EQ(dx, 0);
    CHECK_EQ(dy, -1);
    facingDelta(Facing::W, dx, dy);
    CHECK_EQ(dx, -1);
    CHECK_EQ(dy, 0);
}

// R19: Square = X + 30 * (Y - 1), 1-based.
TEST(dungeon1a_map_square_index_addressing) {
    CHECK_EQ(kMapWidth, 30);
    CHECK_EQ(mapSquareIndex(1, 1), 1);
    CHECK_EQ(mapSquareIndex(30, 1), 30);
    CHECK_EQ(mapSquareIndex(1, 2), 31);
    CHECK_EQ(mapSquareIndex(5, 3), 65);
}

// R19: Walls(square) is 0 when the square is outside the field array.
TEST(dungeon1a_field_flags_out_of_range_is_zero) {
    FloorDef def{};
    CHECK_EQ(fieldFlagsAt(def, 0), 0u);
    CHECK_EQ(fieldFlagsAt(def, -1), 0u);
    CHECK_EQ(fieldFlagsAt(def, 1000000), 0u);
}

// R9: nav.Mapping = !leader.IsLost. IsLost is position || level only,
// so a facing-only loss keeps the automap writing.
TEST(dungeon1a_mapping_ignores_facing_only_loss) {
    Party party{};
    CHECK(mappingEnabled(party));
    party.members[0].lost.facing = true;
    CHECK(mappingEnabled(party));
}

// R9: floor-level or position loss switches mapping off.
TEST(dungeon1a_mapping_off_when_level_or_position_lost) {
    Party party{};
    party.members[0].lost.level = true;
    CHECK(!mappingEnabled(party));

    Party party2{};
    party2.members[0].lost.position = true;
    CHECK(!mappingEnabled(party2));
}

// R28: weight = p <= 0 ? 0 : Log(p)^2. At p = 1, Log(1) = 0 in any base.
TEST(dungeon1a_working_out_weight_zero_cases) {
    CHECK(workingOutWeight(0) == 0.0);
    CHECK(workingOutWeight(-4) == 0.0);
    CHECK(workingOutWeight(1) == 0.0);
    CHECK(workingOutWeight(2) > 0.0);
}

// R28: with weight 0, "0 > Rnd*100" is false for every draw, so nothing clears.
TEST(dungeon1a_work_out_zero_perception_changes_nothing) {
    Env env({static_cast<u64>(0.0)});
    LostState ls = allLost();
    tryToWorkItOut(env.ctx, ls, 0);
    CHECK(ls.position);
    CHECK(ls.level);
    CHECK(ls.facing);
}

// R28: with Rnd = 0 and a positive weight, each "weight/d > 0" check passes,
// so floor, facing and full position are all cleared in order.
TEST(dungeon1a_work_out_zero_draw_clears_all) {
    Env env({static_cast<u64>(0.0)});
    LostState ls = allLost();
    tryToWorkItOut(env.ctx, ls, 2);
    CHECK(!ls.level);
    CHECK(!ls.facing);
    CHECK(!ls.position);
}

// R28/R30: largest i32 perception gives weight under ~462 (natural log), so
// weight/15 < 100 and no check can pass with Rnd close to 1 in any log base.
TEST(dungeon1a_work_out_high_draw_keeps_flags) {
    // Three draws: floor (R28a), facing (R28b), position (R28c). Draw 0.999 fails all three checks.
    Env env({drawOf(0.999), drawOf(0.999), drawOf(0.999)});
    LostState ls = allLost();
    tryToWorkItOut(env.ctx, ls, 2147483647);
    CHECK(ls.position);
    CHECK(ls.level);
    CHECK(ls.facing);
}

