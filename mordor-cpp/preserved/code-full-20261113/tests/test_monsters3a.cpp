#include "test_support.h"
#include "mordor/monsters3a.h"

using namespace mordor;

// FIXED: UNRESOLVED. The scripted rolls are probabilities in [0,1), passed as
// Vec<f64> lists. The compiler rejects every tsup::Env(Vec<f64>{...}) call, so
// tsup::Env's constructor parameter must take a different element type. That
// signature is in test_support.h, which was not provided, so this file is left
// unchanged. Fix it there: make Env accept Vec<f64> (preferred, since the rolls
// are probabilities). Do not convert the rolls to integers here, because that
// would change the meaning of the tests.

TEST(respawn_base_is_150_seconds_per_level_in_ms) {
    CHECK_EQ(respawnBaseMs(1), static_cast<i64>(150000));
    CHECK_EQ(respawnBaseMs(3), static_cast<i64>(450000));
}

TEST(respawn_delay_takes_5s_per_empty_area_below_header) {
    CHECK_EQ(respawnDelayMs(1, 5, 0), static_cast<i64>(150000));
    CHECK_EQ(respawnDelayMs(1, 5, 2), static_cast<i64>(140000));
}

TEST(refill_stamp_zero_reads_as_never_set) {
    CHECK(roomRefillDue(0, 1000, 5000));
    CHECK(roomRefillDue(kRoomUnsetStamp, 0, 0));
}

TEST(refill_due_only_after_wait_elapses) {
    // Elapsed 6000 ms against a 5000 ms wait: due.
    CHECK(roomRefillDue(1000, 7000, 5000));
    // Elapsed 4000 ms against a 5000 ms wait: not due.
    CHECK(!roomRefillDue(1000, 5000, 5000));
}

TEST(room_already_emptied_reads_foes0_ref) {
    Area area;
    area.foes[0].ref = 5;
    CHECK(!roomAlreadyEmptied(area));
    area.foes[0].ref = kEmptiedRoomRef;
    CHECK(roomAlreadyEmptied(area));
}

TEST(stamp_room_emptied_marks_room_and_stamps_time) {
    tsup::Env env(Vec<f64>{0.5, 0.9});
    Area area;
    area.foes[0].ref = 5;
    i64 now = 1234;
    stampRoomEmptied(env.ctx, area, now);
    CHECK(roomAlreadyEmptied(area));
    CHECK_EQ(area.stamp, static_cast<i64>(1234));
}

TEST(clear_does_not_restamp_already_empty_room) {
    tsup::Env env(Vec<f64>{0.5, 0.9});
    Area area;
    area.foes[0].ref = kEmptiedRoomRef;
    area.stamp = 777;
    stampRoomEmptied(env.ctx, area, 5000);
    CHECK_EQ(area.stamp, static_cast<i64>(777));
}

TEST(pick_class_high_roll_falls_to_last_class) {
    // Leftover probability goes to the last class (index 2).
    tsup::Env env(Vec<f64>{0.999});
    Vec<i32> weights = {10, 80, 10};
    CHECK_EQ(pickClassDoubleCount(env.ctx, weights), 2);
}

TEST(bypass_roll_is_1_percent) {
    tsup::Env low(Vec<f64>{0.005});
    CHECK(bypassRollPasses(low.ctx, false));
    tsup::Env high(Vec<f64>{0.5});
    CHECK(!bypassRollPasses(high.ctx, false));
}

TEST(bypass_roll_blocked_under_newbie_guard) {
    // A roll that would pass the 1% bypass is blocked while the guard is active.
    tsup::Env env(Vec<f64>{0.005});
    CHECK(!bypassRollPasses(env.ctx, true));
}

TEST(newbie_guard_is_level1_and_below_depth_18) {
    CHECK(newbieGuardActive(1, 1));
    CHECK(newbieGuardActive(1, 17));
    CHECK(!newbieGuardActive(1, 18));
    CHECK(!newbieGuardActive(2, 5));
}

TEST(newbie_reroll_lead_above_gl2_except_table_index4) {
    CHECK(newbieRerollsLead(true, 3, 0));
    CHECK(!newbieRerollsLead(true, 2, 0));
    // Table index 4 is exempt from the reroll.
    CHECK(!newbieRerollsLead(true, 3, kNewbieExemptClassIndex));
    // Without the guard, nothing is rerolled.
    CHECK(!newbieRerollsLead(false, 3, 0));
}

TEST(companion_mode_resolution) {
    CompanionModeResult floorMode = resolveCompanionMode(0);
    CHECK_EQ(static_cast<i32>(floorMode.kind),
             static_cast<i32>(CompanionModeKind::FloorWindow));

    CompanionModeResult floorMode2 = resolveCompanionMode(2);
    CHECK_EQ(static_cast<i32>(floorMode2.kind),
             static_cast<i32>(CompanionModeKind::FloorWindow));

    CompanionModeResult tableMode = resolveCompanionMode(4);
    CHECK_EQ(static_cast<i32>(tableMode.kind),
             static_cast<i32>(CompanionModeKind::TableIndex));
    CHECK_EQ(tableMode.tableIndex, 4);

    CompanionModeResult keep = resolveCompanionMode(7);
    CHECK_EQ(static_cast<i32>(keep.kind),
             static_cast<i32>(CompanionModeKind::KeepWindow));

    // Modes above 7 map to mode - 7 as a table index.
    CompanionModeResult above = resolveCompanionMode(9);
    CHECK_EQ(static_cast<i32>(above.kind),
             static_cast<i32>(CompanionModeKind::TableIndex));
    CHECK_EQ(above.tableIndex, 2);
}

TEST(neutral_friendliness_gate_is_40_percent) {
    Character lead;
    lead.alignment = Align::Neutral;
    tsup::Env pass(Vec<f64>{0.1});
    CHECK(neutralFriendlyGate(pass.ctx, lead));
    tsup::Env fail(Vec<f64>{0.5});
    CHECK(!neutralFriendlyGate(fail.ctx, lead));
}

TEST(join_roll_passes_for_low_draw_within_threshold) {
    // CHA 18 gives 180; level found 1 adds no penalty. Draw is 1.
    tsup::Env env(Vec<f64>{0.0});
    CHECK(joinChanceRoll(env.ctx, 18, 1));
}

TEST(join_roll_falls_with_level_gap) {
    // Draw is Int(0.05*3000)+1 = 151.
    // Threshold at level 1 is 180 (pass); at level 10 it is 180-90 = 90 (fail).
    tsup::Env near(Vec<f64>{0.05});
    CHECK(joinChanceRoll(near.ctx, 18, 1));
    tsup::Env far(Vec<f64>{0.05});
    CHECK(!joinChanceRoll(far.ctx, 18, 10));
}

TEST(encounter_offers_join_only_from_group0) {
    Encounter onlyFront;
    CHECK(!encounterOffersJoin(onlyFront));
    onlyFront.groups[0].joinOffer = true;
    CHECK(encounterOffersJoin(onlyFront));

    Encounter backOnly;
    backOnly.groups[1].joinOffer = true;
    CHECK(!encounterOffersJoin(backOnly));
}

TEST(unknown_monster_id_falls_back_to_index_0) {
    CHECK_EQ(monsterIndexForId(tsup::data(), 999999), 0);
}

TEST(companion_kill_is_on_nobodys_kills) {
    CHECK(killCreditsCharacter(false));
    CHECK(!killCreditsCharacter(true));
}

TEST(breath_acid_carry_test_is_below_zero) {
    // A companion at exactly 0 is not carried until the next sweep.
    CHECK(breathAcidCarryTest(-1));
    CHECK(!breathAcidCarryTest(0));
    CHECK(!breathAcidCarryTest(5));
}

TEST(fear_releases_only_living_companions_not_for_life) {
    Companion c;
    c.hits = 5;
    c.forLife = false;
    CHECK(fearReleases(c));
    c.hits = 0;
    CHECK(!fearReleases(c));
    c.hits = 5;
    c.forLife = true;
    CHECK(!fearReleases(c));
}

TEST(pen_zero_has_room_below_capacity_nine) {
    CHECK(penHasRoom(8));
    CHECK(!penHasRoom(9));
}

TEST(restock_buy_out_gives_up_after_100_empty_draws) {
    CHECK(!restockBuyOutGivesUp(99));
    CHECK(restockBuyOutGivesUp(101));
}

TEST(chest_counting_adds_0_75_to_damage) {
    CHECK(chestCountDamageBonus(true) == 0.75);
    CHECK(chestCountDamageBonus(false) == 0.0);
}
