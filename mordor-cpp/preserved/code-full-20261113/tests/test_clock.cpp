#include "test_support.h"
#include "mordor/clock.h"

using namespace mordor;

namespace {

i32 ik(CombatOrderKind k) {
    return static_cast<i32>(k);
}

}  // namespace

// ---------- Hostile base table (R19, Tables) ----------

TEST(hostile_base_good_monster_row) {
    CHECK_EQ(hostileBase(CharAlign::Good, Align::Good), 0);
    CHECK_EQ(hostileBase(CharAlign::Good, Align::Neutral), 15);
    CHECK_EQ(hostileBase(CharAlign::Good, Align::Evil), 100);
}

TEST(hostile_base_neutral_monster_row) {
    CHECK_EQ(hostileBase(CharAlign::Neutral, Align::Good), 25);
    CHECK_EQ(hostileBase(CharAlign::Neutral, Align::Neutral), 15);
    CHECK_EQ(hostileBase(CharAlign::Neutral, Align::Evil), 25);
}

TEST(hostile_base_evil_monster_row) {
    CHECK_EQ(hostileBase(CharAlign::Evil, Align::Good), 35);
    CHECK_EQ(hostileBase(CharAlign::Evil, Align::Neutral), 35);
    CHECK_EQ(hostileBase(CharAlign::Evil, Align::Evil), 25);
}

// ---------- Hostile roll (R19): T = trunc(base - 7.5*ln(max(1,CHA))) ----------
// Draws are 1..100; aggression if draw < T. Only extreme RNG values are used
// so the expected result does not depend on the exact draw mapping.

TEST(hostile_roll_low_draw_hits_full_threshold) {
    // Lowest draw: nextDouble() in [0,1) gives below(100)=0, so draw = 1.
    // The previous setup fed Rnd=1, which is outside VB Rnd's range and produced draw 101.
    tsup::Env env({0});
    // base 100, CHA 1 gives T = 100; draw 1 < 100
    CHECK(hostileRoll(env.ctx, 100, 1));
}

TEST(hostile_roll_top_draw_misses_threshold_100) {
    tsup::Env env({100});  // highest draw, 100
    // T = 100; draw 100 is not strictly below 100
    CHECK(!hostileRoll(env.ctx, 100, 1));
}

TEST(hostile_roll_top_draw_misses_threshold_25) {
    tsup::Env env({100});
    // base 25, CHA 1 gives T = 25; draw 100 is not below 25
    CHECK(!hostileRoll(env.ctx, 25, 1));
}

TEST(hostile_roll_low_draw_hits_threshold_25) {
    // Lowest draw: Rnd=0 gives draw = 1 (see note above).
    tsup::Env env({0});
    // base 25, CHA 1 gives T = 25; draw 1 < 25
    CHECK(hostileRoll(env.ctx, 25, 1));
}

TEST(hostile_roll_zero_base_never_aggressive) {
    tsup::Env env({1});
    // base 0, CHA 1 gives T = 0; no draw 1..100 is below 0
    CHECK(!hostileRoll(env.ctx, 0, 1));
    // base 0, CHA 5 gives T = trunc(-12.07) = -12
    CHECK(!hostileRoll(env.ctx, 0, 5));
}

TEST(hostile_roll_cha_below_one_is_clamped_to_one) {
    tsup::Env env({100});
    // CHA 0 and CHA -5 both use ln(1) = 0, so T = base = 100; draw 100 misses
    CHECK(!hostileRoll(env.ctx, 100, 0));
    CHECK(!hostileRoll(env.ctx, 100, -5));
}

TEST(hostile_roll_cha_zero_uses_base_threshold) {
    // Lowest draw: Rnd=0 gives draw = 1 (see note above).
    tsup::Env env({0});
    // CHA 0 clamps to 1, so T = 15; draw 1 is below 15
    CHECK(hostileRoll(env.ctx, 15, 0));
}

TEST(hostile_roll_very_high_cha_suppresses_aggression) {
    tsup::Env env({1});
    // base 100, CHA 1000000: T = trunc(100 - 7.5*13.8155) = trunc(-3.6) = -3
    CHECK(!hostileRoll(env.ctx, 100, 1000000));
}

// ---------- Free-attack threshold (R22/R26) ----------
// max(5, 50 + 5*(monsterDex - charDex)); cap at 98 only when cap is set.

TEST(free_attack_threshold_equal_dex_is_fifty) {
    CHECK_EQ(freeAttackThreshold(12, 12, false), 50);
    CHECK_EQ(freeAttackThreshold(12, 12, true), 50);
}

TEST(free_attack_threshold_floor_of_five) {
    // 50 + 5*(0-20) = -50, floored to 5 regardless of cap
    CHECK_EQ(freeAttackThreshold(0, 20, false), 5);
    CHECK_EQ(freeAttackThreshold(0, 20, true), 5);
}

TEST(free_attack_threshold_floor_boundary) {
    // 50 + 5*(0-9) = 5 exactly
    CHECK_EQ(freeAttackThreshold(0, 9, false), 5);
    // 50 + 5*(0-8) = 10
    CHECK_EQ(freeAttackThreshold(0, 8, false), 10);
    // 50 + 5*(0-10) = 0, floored to 5
    CHECK_EQ(freeAttackThreshold(0, 10, false), 5);
}

TEST(free_attack_threshold_small_gap) {
    // 50 + 5*(1-2) = 45
    CHECK_EQ(freeAttackThreshold(1, 2, false), 45);
    // 50 + 5*(9-0) = 95, under the cap
    CHECK_EQ(freeAttackThreshold(9, 0, true), 95);
}

TEST(free_attack_threshold_uncapped_can_exceed_98) {
    // 50 + 5*30 = 200 with no cap (Moved, argument 1)
    CHECK_EQ(freeAttackThreshold(30, 0, false), 200);
    // 50 + 5*10 = 100 with no cap
    CHECK_EQ(freeAttackThreshold(10, 0, false), 100);
}

TEST(free_attack_threshold_cap_at_98_on_level_change) {
    // 200 capped to 98 (ChangedLevel, argument 2)
    CHECK_EQ(freeAttackThreshold(30, 0, true), 98);
    // 100 capped to 98
    CHECK_EQ(freeAttackThreshold(10, 0, true), 98);
}

// ---------- Play clock (R7, R28-R30, P8) ----------

TEST(play_clock_adds_elapsed_seconds_and_advances_start) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> members;
    Character c;
    c.playStart = 100;
    c.playSeconds = 0;
    members.push_back(c);
    playClockTick(env.ctx, members, false, 110);
    CHECK_EQ(members[0].playSeconds, 10);
    CHECK_EQ(members[0].playStart, 110);
}

TEST(play_clock_adds_to_existing_play_time) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> members;
    Character c;
    c.playStart = 500;
    c.playSeconds = 1000;
    members.push_back(c);
    playClockTick(env.ctx, members, false, 504);
    CHECK_EQ(members[0].playSeconds, 1004);
    CHECK_EQ(members[0].playStart, 504);
}

TEST(play_clock_midnight_wrap_uses_86340) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> members;
    Character c;
    c.playStart = 86300;
    c.playSeconds = 0;
    members.push_back(c);
    // now < start: gap = (86340 - 86300) + 20 = 60
    playClockTick(env.ctx, members, false, 20);
    CHECK_EQ(members[0].playSeconds, 60);
    CHECK_EQ(members[0].playStart, 20);
}

TEST(play_clock_paused_time_not_counted_but_start_advances) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> members;
    Character c;
    c.playStart = 100;
    c.playSeconds = 0;
    members.push_back(c);
    playClockTick(env.ctx, members, true, 110);
    CHECK_EQ(members[0].playSeconds, 0);
    CHECK_EQ(members[0].playStart, 110);
}

TEST(play_clock_zero_start_sets_start_and_returns_early) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> members;
    Character first;
    first.playStart = 0;
    first.playSeconds = 0;
    Character second;
    second.playStart = 100;
    second.playSeconds = 0;
    members.push_back(first);
    members.push_back(second);
    playClockTick(env.ctx, members, false, 110);
    // first member: start set, no time added
    CHECK_EQ(members[0].playStart, 110);
    CHECK_EQ(members[0].playSeconds, 0);
    // later member skipped by the early return
    CHECK_EQ(members[1].playStart, 100);
    CHECK_EQ(members[1].playSeconds, 0);
}

TEST(play_clock_negative_play_time_sets_start_and_returns_early) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> members;
    Character c;
    c.playStart = 100;
    c.playSeconds = -1;
    members.push_back(c);
    playClockTick(env.ctx, members, false, 110);
    CHECK_EQ(members[0].playStart, 110);
    CHECK_EQ(members[0].playSeconds, -1);
}

// ---------- Clock setup (P1) ----------

TEST(clock_setup_intervals) {
    tsup::Env env({0.5, 0.9});
    GameState game;
    clockSetup(env.ctx, game);
    CHECK_EQ(game.combatIntervalMs, 1000);
    CHECK_EQ(game.ailmentIntervalMs, 4000);
    CHECK_EQ(game.playIntervalMs, 250);
}

// ---------- Standing orders ----------

TEST(order_from_setting_fight_and_defend) {
    tsup::Env env({0.5, 0.9});
    Arr<i32, kSpellBuffers> buffers{};
    OrderChoice fight = orderFromSetting(env.ctx, 1, buffers);
    CHECK_EQ(ik(fight.kind), ik(CombatOrderKind::Fight));
    CHECK(!fight.useItem);
    OrderChoice defend = orderFromSetting(env.ctx, 2, buffers);
    CHECK_EQ(ik(defend.kind), ik(CombatOrderKind::Defend));
    CHECK(!defend.useItem);
}

TEST(order_from_setting_non_positive_is_none) {
    tsup::Env env({0.5, 0.9});
    Arr<i32, kSpellBuffers> buffers{};
    OrderChoice zero = orderFromSetting(env.ctx, 0, buffers);
    CHECK_EQ(ik(zero.kind), ik(CombatOrderKind::None));
    OrderChoice neg = orderFromSetting(env.ctx, -1, buffers);
    CHECK_EQ(ik(neg.kind), ik(CombatOrderKind::None));
}

TEST(check_cast_clears_fight_and_keeps_queued_spell) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.order.kind = CombatOrderKind::Fight;
    who.order.spellId = 7;
    who.order.target = 2;
    checkCast(env.ctx, who);
    CHECK_EQ(ik(who.order.kind), ik(CombatOrderKind::None));
    CHECK_EQ(who.order.spellId, 7);
    CHECK_EQ(who.order.target, 2);
}

TEST(check_cast_clears_defend) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.order.kind = CombatOrderKind::Defend;
    checkCast(env.ctx, who);
    CHECK_EQ(ik(who.order.kind), ik(CombatOrderKind::None));
}

TEST(check_cast_leaves_other_orders_alone) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.order.kind = CombatOrderKind::Buffer;
    who.order.spellId = 9;
    checkCast(env.ctx, who);
    CHECK_EQ(ik(who.order.kind), ik(CombatOrderKind::Buffer));
    CHECK_EQ(who.order.spellId, 9);
}

TEST(open_spell_list_clears_fight_and_defend_keeps_spell) {
    tsup::Env env({0.5, 0.9});
    Character fighter;
    fighter.order.kind = CombatOrderKind::Fight;
    fighter.order.spellId = 4;
    fighter.order.target = 1;
    openSpellList(env.ctx, fighter);
    CHECK_EQ(ik(fighter.order.kind), ik(CombatOrderKind::None));
    CHECK_EQ(fighter.order.spellId, 4);
    CHECK_EQ(fighter.order.target, 1);

    Character defender;
    defender.order.kind = CombatOrderKind::Defend;
    openSpellList(env.ctx, defender);
    CHECK_EQ(ik(defender.order.kind), ik(CombatOrderKind::None));
}

TEST(att_reorder_none_and_fight_become_defend) {
    tsup::Env env({0.5, 0.9});
    Character none;
    none.order.kind = CombatOrderKind::None;
    none.order.spellId = 5;
    attReorder(env.ctx, none, -1);
    CHECK_EQ(ik(none.order.kind), ik(CombatOrderKind::Defend));
    CHECK_EQ(none.order.spellId, 0);  // queued spell dropped

    Character fight;
    fight.order.kind = CombatOrderKind::Fight;
    fight.order.spellId = 5;
    attReorder(env.ctx, fight, -1);
    CHECK_EQ(ik(fight.order.kind), ik(CombatOrderKind::Defend));
    CHECK_EQ(fight.order.spellId, 0);
}

TEST(att_reorder_defend_and_other_become_fight) {
    tsup::Env env({0.5, 0.9});
    Character defend;
    defend.order.kind = CombatOrderKind::Defend;
    attReorder(env.ctx, defend, -1);
    CHECK_EQ(ik(defend.order.kind), ik(CombatOrderKind::Fight));

    Character buffer;
    buffer.order.kind = CombatOrderKind::Buffer;
    buffer.order.spellId = 3;
    attReorder(env.ctx, buffer, -1);
    CHECK_EQ(ik(buffer.order.kind), ik(CombatOrderKind::Fight));
    CHECK_EQ(buffer.order.spellId, 0);
}

TEST(end_fight_clears_standing_orders) {
    tsup::Env env({0.5, 0.9});
    GameState game;
    game.party.size = 2;
    game.party.members[0].order.kind = CombatOrderKind::Fight;
    game.party.members[1].order.kind = CombatOrderKind::Defend;
    endFight(env.ctx, game);
    CHECK_EQ(ik(game.party.members[0].order.kind), ik(CombatOrderKind::None));
    CHECK_EQ(ik(game.party.members[1].order.kind), ik(CombatOrderKind::None));
}
