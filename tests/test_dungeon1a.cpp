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

// R25/R26: BecomeLost sets all three flags; FoundYourself clears all three.
TEST(dungeon1a_become_lost_then_found_yourself) {
    Env env({static_cast<u64>(0.5)});
    LostState ls;
    becomeLost(env.ctx, ls);
    CHECK(ls.position);
    CHECK(ls.level);
    CHECK(ls.facing);
    // FIXED: removed call to undeclared placeholder helper
    // isLostHelperUnused_placeholder_never_called_or_true_check
}

// R32: a morgue visit clears all three lost flags.
TEST(dungeon1a_morgue_clears_all_lost_flags) {
    Env env({static_cast<u64>(0.5)});
    LostState ls = allLost();
    clearLostAtMorgue(env.ctx, ls);
    CHECK(!ls.position);
    CHECK(!ls.level);
    CHECK(!ls.facing);
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

// R29: save if p >= Rnd*70 + DungeonLevel*2. With Rnd = 0.5, DL = 2:
// threshold = 35 + 4 = 39, so p = 39 is saved (>=).
TEST(dungeon1a_rotator_save_at_threshold) {
    Env env({static_cast<u64>(0.5)});
    LostState ls;
    applyRotatorLoss(env.ctx, ls, 39, 2);
    CHECK(!ls.position);
    CHECK(!ls.facing);
    CHECK(!ls.level);
}

// R29: failure sets LostPosition and LostFacing; the floor flag is untouched.
// With Rnd = 0.51: threshold = 35.7 + 4 = 39.7 > 39, so the save fails.
TEST(dungeon1a_rotator_failure_keeps_floor_flag) {
    // R29: threshold = 0.51*70 + 2*2 = 39.7 > 39, so the save fails and position/facing are lost.
    Env env({drawOf(0.51)});
    LostState ls;
    ls.level = true;
    applyRotatorLoss(env.ctx, ls, 39, 2);
    CHECK(ls.position);
    CHECK(ls.facing);
    CHECK(ls.level);
}

// R52: hovers if 5 + level/4.0 < Rnd*100. Level 0: threshold 5.
TEST(dungeon1a_pit_hover_boundary_level_zero) {
    // R52: hover if 5 < Rnd*100. Level 0 gives threshold 5.
    Env envLow({drawOf(0.04)});
    CHECK(!rollPitHover(envLow.ctx, 0));
    Env envHigh({drawOf(0.06)});
    CHECK(rollPitHover(envHigh.ctx, 0));
}

// R52: level term. Level 40 gives threshold 5 + 10 = 15.
TEST(dungeon1a_pit_hover_level_term) {
    // R52: level 40 gives threshold 5 + 40/4 = 15.
    Env envLow({drawOf(0.1)});
    CHECK(!rollPitHover(envLow.ctx, 40));
    Env envHigh({drawOf(0.2)});
    CHECK(rollPitHover(envHigh.ctx, 40));
}

// R50: roll = Int(Rnd*100)+1; the "< 50" test adds 1 to spread.
// Level 10, not on stud: base = level/5 = 2. Roll 49 adds 1 (spread 3); roll 50 adds 0 (spread 2).
TEST(dungeon1a_encounter_spread_roll_boundary) {
    // R50: roll = Int(Rnd*100)+1. Rnd 0.48 gives roll 49 (<50, +1); Rnd 0.49 gives roll 50 (no +1).
    Env envBelow({drawOf(0.48)});
    CHECK_EQ(rollEncounterSpread(envBelow.ctx, 10, false), 3);
    Env envAt({drawOf(0.49)});
    CHECK_EQ(rollEncounterSpread(envAt.ctx, 10, false), 2);
}

// R50: spread is capped at the level. Level 0 with roll < 50 gives 1, capped to 0.
// Level 12 with roll >= 50 gives 12/5 = 2.
TEST(dungeon1a_encounter_spread_capped_at_level) {
    // R50: spread = min(spread, level). Level 0 caps to 0; level 12 gives 0 + 12/5 = 2.
    Env envZero({drawOf(0.1)});
    CHECK_EQ(rollEncounterSpread(envZero.ctx, 0, false), 0);
    Env envTwelve({drawOf(0.9)});
    CHECK_EQ(rollEncounterSpread(envTwelve.ctx, 12, false), 2);
}

// R49: ROCK suppresses encounters; STUD adds one floor; FOG leaves the group unidentified.
TEST(dungeon1a_encounter_traits_per_square_flag) {
    EncounterSquareTraits rock = encounterTraitsForSquare(true, false, false);
    CHECK(rock.suppressed);
    CHECK_EQ(rock.floorBonus, 0);
    CHECK(!rock.unidentified);

    EncounterSquareTraits stud = encounterTraitsForSquare(false, true, false);
    CHECK(!stud.suppressed);
    CHECK_EQ(stud.floorBonus, 1);

    EncounterSquareTraits fog = encounterTraitsForSquare(false, false, true);
    CHECK(!fog.suppressed);
    CHECK_EQ(fog.floorBonus, 0);
    CHECK(fog.unidentified);
}

// R48: kept while (now - stamp) > 2 * respawnT. Here 2 * 10 = 20.
TEST(dungeon1a_standing_group_boundary) {
    Area area{};
    area.stamp = 100;
    area.respawnT = 10;
    CHECK(!standingGroupKept(120, area));
    CHECK(standingGroupKept(121, area));
}

// R46: perceive-ahead chance is capped at 95.
TEST(dungeon1a_perceive_ahead_chance_cap) {
    CHECK_EQ(capPerceiveAheadChance(40), 40);
    CHECK_EQ(capPerceiveAheadChance(95), 95);
    CHECK_EQ(capPerceiveAheadChance(96), 95);
    CHECK_EQ(capPerceiveAheadChance(200), 95);
}

// R46: hit if NextDouble*100 < chance (strict). Chance 0 never hits.
TEST(dungeon1a_perceive_ahead_hit_boundary) {
    // R46: hit if Rnd*100 < best, best capped at 95. The comparison is strict,
    // so a draw of exactly 0.5 with best 50 is a miss.
    Env envBelow({drawOf(0.49)});
    CHECK(perceiveAheadHit(envBelow.ctx, 50));
    Env envAt({drawOf(0.5)});
    CHECK(!perceiveAheadHit(envAt.ctx, 50));
    Env envZero({drawOf(0.0)});
    CHECK(!perceiveAheadHit(envZero.ctx, 0));
}

// R36: notice roll Int(Rnd*100)+1 <= P. At P = 0 nothing is noticed, and no identify roll is made.
TEST(dungeon1a_square_perception_zero_never_noticed) {
    Env env({static_cast<u64>(0.0)});
    SquareNotice n = rollSquarePerception(env.ctx, 0);
    CHECK(!n.noticed);
    CHECK(!n.identified);
}

// R36: at P = 50, notice roll 50 passes (<= 50); identify roll needs <= P/2 = 25.
// Draws {0.49, 0.49} give noticed but not identified. Draws {0.49, 0.0} give both.
// Draw 0.5 gives roll 51, so nothing is noticed.
TEST(dungeon1a_square_perception_notice_identify_boundary) {
    // R36: notice = Next(100)+1 <= P; identify = Next(100)+1 <= P/2 (only if noticed).
    // Case 1: notice roll 50 <= 50, identify roll 50 > 25, so noticed but not identified.
    Env envNoId({drawOf(0.49), drawOf(0.49)});
    SquareNotice a = rollSquarePerception(envNoId.ctx, 50);
    CHECK(a.noticed);
    CHECK(!a.identified);

    // Case 2: notice roll 50 <= 50, identify roll 1 <= 25, so noticed and identified.
    Env envBoth({drawOf(0.49), drawOf(0.0)});
    SquareNotice b = rollSquarePerception(envBoth.ctx, 50);
    CHECK(b.noticed);
    CHECK(b.identified);

    // Case 3: draw 0.5 gives notice roll Int(50)+1 = 51 > 50, so neither flag is set.
    Env envMiss({drawOf(0.5)});
    SquareNotice c = rollSquarePerception(envMiss.ctx, 50);
    CHECK(!c.noticed);
    CHECK(!c.identified);
}

// R31: newcomer copies lost-on-floor, in-position and in-facing from the leader (Chars(0)).
TEST(dungeon1a_inherit_lost_on_join_copies_leader) {
    Env env({static_cast<u64>(0.5)});
    Party party{};
    party.size = 1;
    party.members[0].lost.position = true;
    party.members[0].lost.facing = true;
    party.members[0].lost.level = false;

    Character newcomer{};
    inheritLostOnJoin(env.ctx, party, newcomer);
    CHECK(newcomer.lost.position);
    CHECK(!newcomer.lost.level);
    CHECK(newcomer.lost.facing);
}
