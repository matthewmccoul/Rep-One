#include <cmath>

#include "test_support.h"
#include "mordor/status1a.h"

using namespace mordor;

namespace {

// Character with base CON set (stats index 3 = Con) and no gear CON.
Character makeVictim(i32 con) {
    Character c;
    c.abilities.stats[3] = con;
    c.abilities.gearCon = 0;
    return c;
}

bool closeTo(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

// roll offset = (effCON - 10) * 2 - level * 2
TEST(ailment_roll_offset_follows_con_and_level) {
    CHECK_EQ(ailmentRollOffset(10, 5), -10);
    CHECK_EQ(ailmentRollOffset(6, 5), -18);
    CHECK_EQ(ailmentRollOffset(14, 0), 8);
}

// R2/R16: a paralysis level above 0 means held; held characters do not act on their turn.
TEST(held_flag_follows_paralysis_level) {
    Character c;
    CHECK(!isHeld(c));
    c.ailments.paralysisLevel = 3;
    CHECK(isHeld(c));
    CHECK(!canActOnOwnTurn(c));
}

// R17/R18: the listed actions are refused while held; stairs down and chute are not.
TEST(held_refuses_listed_actions_only) {
    Character c;
    c.ailments.paralysisLevel = 2;
    CHECK(heldActionRefused(c, HeldAction::Move));
    CHECK(heldActionRefused(c, HeldAction::Turn));
    CHECK(heldActionRefused(c, HeldAction::StairsUp));
    CHECK(heldActionRefused(c, HeldAction::Exit));
    CHECK(heldActionRefused(c, HeldAction::PickUpBody));
    CHECK(heldActionRefused(c, HeldAction::DropBody));
    CHECK(heldActionRefused(c, HeldAction::GiveBody));
    CHECK(heldActionRefused(c, HeldAction::Fight));
    CHECK(heldActionRefused(c, HeldAction::OpenChest));
    CHECK(!heldActionRefused(c, HeldAction::StairsDown));
    CHECK(!heldActionRefused(c, HeldAction::Chute));

    Character free;
    CHECK(!heldActionRefused(free, HeldAction::Move));
}

// R27 / ratio bands: High above 1.25M, Low below 0.75M, Even between.
// With M = 8: 1.25M = 10, 0.75M = 6. Equality at 10 and 6 is Even.
TEST(ratio_band_edges_at_monster_level_8) {
    CHECK(ratioBand(11, 8) == RatioBand::High);
    CHECK(ratioBand(10, 8) == RatioBand::Even);
    CHECK(ratioBand(6, 8) == RatioBand::Even);
    CHECK(ratioBand(5, 8) == RatioBand::Low);
}

// R27 breath (audit): >1.25M -> 80, 0.75M..1.25M -> 50, <0.75M -> 20. M = 8.
TEST(breath_threshold_bands) {
    CHECK_EQ(breathThresholdPercent(11, 8), 80);
    CHECK_EQ(breathThresholdPercent(8, 8), 50);
    CHECK_EQ(breathThresholdPercent(5, 8), 20);
}

// R30 / companion test value: CON*(ln CON / ln 2.5) - floor, floor 0.
// CON 10 gives 25.1294. Engine stoning: minus LevelFound, capped at 95.
TEST(stone_value_and_chance_at_con_10) {
    CHECK(closeTo(companionStoneValue(10, 0), 25.1294, 0.001));

    Character t = makeVictim(10);
    CHECK(closeTo(stoneChance(t, 5), 20.1294, 0.001));

    Character tough = makeVictim(100);
    CHECK(closeTo(stoneChance(tough, 0), 95.0, 1e-9));
}

// R22 item 5: stone if Rnd*100 > stoneValue (25.13 at CON 10).
// A stoned target loses its hits.
TEST(companion_blow_stone_threshold) {
    Creature target;
    target.hits = 12;

    tsup::Env low({0.2, 0.2});   // 20 > 25.13 is false
    CHECK(!companionBlowStones(low.ctx, target, 10, 0));
    CHECK_EQ(target.hits, 12);

    tsup::Env high({0.3, 0.3});  // 30 > 25.13 is true
    CHECK(companionBlowStones(high.ctx, target, 10, 0));
    CHECK_EQ(target.hits, 0);
}

// R23: dmg - dmg * ((res / 2) * 0.01); no change when res <= 0.
TEST(special_resistance_reduction) {
    CHECK_EQ(applySpecialResistance(100, 20), 90);
    CHECK_EQ(applySpecialResistance(100, 0), 100);
    CHECK_EQ(applySpecialResistance(100, -20), 100);
    CHECK_EQ(applySpecialResistance(100, 100), 50);
}

// R19 wear-off chance = clamp(hold - fight, 5, 90).
// Hold = ln(0.6P)/ln(1.02). Fight (L = 1, CON 10, resist 0) = 0.2049919 + 15 = 15.2050.
TEST(paralysis_wear_off_chance_clamps_and_formula) {
    Character c = makeVictim(10);

    c.ailments.paralysisLevel = 1;      // hold = -25.8, clamps low
    CHECK(closeTo(paralysisWearOffChance(c, 1), 5.0, 1e-9));

    c.ailments.paralysisLevel = 100;    // hold = 206.7, clamps high
    CHECK(closeTo(paralysisWearOffChance(c, 1), 90.0, 1e-9));

    c.ailments.paralysisLevel = 6;      // hold 64.685 - fight 15.205 = 49.48
    CHECK(closeTo(paralysisWearOffChance(c, 1), 49.48, 0.1));
}

// Wear-off with chance clamped to 5: ends when Int(Rnd*100)+1 > 5.
TEST(paralysis_wear_off_tick_uses_chance_threshold) {
    Character c = makeVictim(10);
    c.ailments.paralysisLevel = 1;

    tsup::Env stay({0.03, 0.03});   // Int(3)+1 = 4, not > 5
    CHECK(!paralysisWearOffTick(stay.ctx, c, 1));

    tsup::Env ends({0.06, 0.06});   // Int(6)+1 = 7, > 5
    CHECK(paralysisWearOffTick(ends.ctx, c, 1));
}

// R20: paralysis always ends when no monster is on the square.
TEST(paralysis_clears_without_monster_on_square) {
    Character c = makeVictim(10);
    c.ailments.paralysisLevel = 3;
    tsup::Env env({0.5, 0.5});
    paralysisNoMonsterClear(env.ctx, c, false);
    CHECK(!isHeld(c));
}

// R12: chance = min(Res(c,8) + CON, 95). Paralyse(monLvl) when Rnd(100)+1 > chance.
TEST(melee_paralysis_chance_and_check) {
    Character c = makeVictim(10);
    c.resistances.resist[8] = 20;            // 20 + 10 = 30
    CHECK_EQ(meleeParalysisChance(c), 30);

    tsup::Env hit({0.5, 0.5});               // 51 > 30 -> paralysed at level 7
    meleeParalysisCheck(hit.ctx, c, 7);
    CHECK_EQ(c.ailments.paralysisLevel, 7);

    Character d = makeVictim(10);
    d.resistances.resist[8] = 20;
    tsup::Env miss({0.1, 0.1});              // 11 > 30 is false
    meleeParalysisCheck(miss.ctx, d, 7);
    CHECK_EQ(d.ailments.paralysisLevel, 0);

    Character e = makeVictim(20);
    e.resistances.resist[8] = 90;            // 110 capped at 95
    CHECK_EQ(meleeParalysisChance(e), 95);
}

// R13.1: Paralyse returns early if already held, so the first level is kept.
TEST(paralysis_does_not_overwrite_existing_level) {
    Character c = makeVictim(10);
    c.ailments.paralysisLevel = 4;
    tsup::Env hit({0.5, 0.5});               // would paralyse again
    meleeParalysisCheck(hit.ctx, c, 7);
    CHECK_EQ(c.ailments.paralysisLevel, 4);
}

// R9: poison and disease are inflicted with level = attacker level,
// each skipped on an existing instance or a successful resistance.
TEST(inflict_poison_and_disease_rules) {
    Character a;                              // resistances all 0
    tsup::Env env1({0.5, 0.5});               // 51 > 0 for both
    inflictPoisonAndDisease(env1.ctx, a, 7);
    CHECK_EQ(a.ailments.poisonLevel, 7);
    CHECK_EQ(a.ailments.diseaseLevel, 7);

    Character b;
    b.resistances.resist[5] = 100;            // poison resistance blocks 51
    tsup::Env env2({0.5, 0.5});
    inflictPoisonAndDisease(env2.ctx, b, 7);
    CHECK_EQ(b.ailments.poisonLevel, 0);
    CHECK_EQ(b.ailments.diseaseLevel, 7);

    Character p;
    p.ailments.poisonLevel = 3;               // existing poison is kept
    tsup::Env env3({0.5, 0.5});
    inflictPoisonAndDisease(env3.ctx, p, 7);
    CHECK_EQ(p.ailments.poisonLevel, 3);
    CHECK_EQ(p.ailments.diseaseLevel, 7);
}

// R7/R8: a failed poison roll (X=0, roll -10) costs 2 hits.
TEST(poison_tick_failure_costs_two_hits) {
    Character c = makeVictim(10);
    c.ailments.poisonLevel = 5;
    c.vitals.curHits = 20;
    tsup::Env env({0.0, 0.0});
    AilmentTickResult r = poisonDiseaseTick(env.ctx, c);
    CHECK(!r.poisonCured);
    CHECK_EQ(r.hitsLost, 2);
    CHECK_EQ(c.vitals.curHits, 18);
}

// R8 poison cure needs roll > 185. CON 10, level 5: roll = X - 10.
// X = 195 gives 185 (not cured). X = 196 gives 186 (cured).
TEST(poison_cure_threshold_boundary) {
    Character low = makeVictim(10);
    low.ailments.poisonLevel = 5;
    low.vitals.curHits = 20;
    tsup::Env envLow({0.9775, 0.0});         // Int(195.5) = 195
    AilmentTickResult rl = poisonDiseaseTick(envLow.ctx, low);
    CHECK(!rl.poisonCured);
    CHECK_EQ(low.vitals.curHits, 18);

    Character high = makeVictim(10);
    high.ailments.poisonLevel = 5;
    high.vitals.curHits = 20;
    tsup::Env envHigh({0.9825, 0.0});        // Int(196.5) = 196
    AilmentTickResult rh = poisonDiseaseTick(envHigh.ctx, high);
    CHECK(rh.poisonCured);
    CHECK_EQ(rh.hitsLost, 0);
    CHECK_EQ(high.vitals.curHits, 20);
}

// R7: with poison not cured, disease roll subtracts poisonLevel*2.
// Poison X=0 fails (-2). Disease X=196: roll 196 - 10 = 186, not > 195, fails (-1).
TEST(disease_roll_subtracts_poison_term) {
    Character c = makeVictim(10);
    c.ailments.poisonLevel = 5;
    c.ailments.diseaseLevel = 3;
    c.vitals.curHits = 20;
    tsup::Env env({0.0, 0.9825});
    AilmentTickResult r = poisonDiseaseTick(env.ctx, c);
    CHECK(!r.poisonCured);
    CHECK(!r.diseaseCured);
    CHECK_EQ(r.hitsLost, 3);
    CHECK_EQ(c.vitals.curHits, 17);
}

// R8 disease cure needs roll > 195. Without poison, X = 196 gives 196 > 195.
TEST(disease_cured_when_not_poisoned) {
    Character c = makeVictim(10);
    c.ailments.diseaseLevel = 3;
    c.vitals.curHits = 20;
    tsup::Env env({0.9825, 0.0});
    AilmentTickResult r = poisonDiseaseTick(env.ctx, c);
    CHECK(r.diseaseCured);
    CHECK_EQ(r.hitsLost, 0);
    CHECK_EQ(c.vitals.curHits, 20);
}

// Documented quirk: at CON 6, level 5 poison, X = 199 gives roll 181, never cured.
TEST(con_6_poison_cannot_be_cured) {
    Character c = makeVictim(6);
    c.ailments.poisonLevel = 5;
    c.vitals.curHits = 20;
    tsup::Env env({0.9999, 0.0});
    AilmentTickResult r = poisonDiseaseTick(env.ctx, c);
    CHECK(!r.poisonCured);
    CHECK_EQ(r.hitsLost, 2);
    CHECK_EQ(c.vitals.curHits, 18);
}

// R11: disease halves healing while diseased.
TEST(healing_halved_while_diseased) {
    Character c;
    c.ailments.diseaseLevel = 3;
    CHECK_EQ(healingWhileDiseased(c, 10), 5);

    Character healthy;
    CHECK_EQ(healingWhileDiseased(healthy, 10), 10);
}

// Drain: trials = Int(Rnd*VbInt(LevelFound/2+0.5))+1; with level 5 the range is 3.
// Save skipped when Rnd(100)+1 <= CON*2.5 - level*2 = 25 - 10 = 15.
TEST(drain_save_skip_and_failed_pick) {
    Character c = makeVictim(10);

    tsup::Env skipped({0.0, 0.1});           // Int(10)+1 = 11 <= 15, skipped
    DrainOutcome a = drainCharacter(skipped.ctx, c, 5);
    CHECK(a.picks.size() == 0);

    tsup::Env failed({0.0, 0.5, 0.5});       // save 51 > 15 fails; pick Int(3)+1 = 4
    DrainOutcome b = drainCharacter(failed.ctx, c, 5);
    CHECK(b.picks.size() == 1);
    CHECK_EQ(b.picks[0], 4);
}

// Table E: pick 1..5 drains stat index = pick (index 0 unreachable); pick 6 drains MaxHits 2.
TEST(apply_drain_pick_targets) {
    Character c = makeVictim(10);
    c.abilities.stats[0] = 12;
    c.abilities.stats[1] = 12;
    c.vitals.maxHits = 15;

    tsup::Env env({0.5, 0.5});
    applyDrainPick(env.ctx, c, 1);
    CHECK_EQ(c.abilities.stats[1], 11);
    CHECK_EQ(c.abilities.stats[0], 12);

    applyDrainPick(env.ctx, c, 6);
    CHECK_EQ(c.vitals.maxHits, 13);
}

// Aging save = 2.5*CON - 2F = 25 - 10 = 15 at CON 10, floor 5.
// Destruction save = clamp(45 - F + 4*DEX - 2*monDEX, 25, 90).
TEST(aging_save_and_destruction_clamp) {
    Character c = makeVictim(10);
    CHECK(closeTo(agingSaveChance(c, 5), 15.0, 1e-9));

    Character mid;
    mid.abilities.stats[5] = 10;             // DEX
    CHECK_EQ(destructionSaveChance(mid, 10, 5), 65);   // 45 - 10 + 40 - 10

    Character hi;
    hi.abilities.stats[5] = 20;
    CHECK_EQ(destructionSaveChance(hi, 0, 0), 90);     // 125 clamps to 90

    Character lo;
    lo.abilities.stats[5] = 3;
    CHECK_EQ(destructionSaveChance(lo, 30, 20), 25);   // -13 clamps to 25
}

// R36: gold attempted if score - 2M < 50. With M = 10: 69 gives 49 (yes), 70 gives 50 (no).
TEST(theft_gold_threshold_boundary) {
    CHECK(theftGoldAttempted(69, 10));
    CHECK(!theftGoldAttempted(70, 10));
}

// R37 archive fallbacks: stone min 5; paralysis = 100 - (res + CON), min 5; age = 100 + 2S - 2.5CON.
TEST(archive_fallback_formulas_and_floors) {
    CHECK_EQ(archiveStoneChance(0, 100), 5);
    CHECK_EQ(archiveParalysisChance(0, 10), 90);
    CHECK_EQ(archiveParalysisChance(50, 60), 5);
    CHECK_EQ(archiveAgeChance(10, 10), 95);   // (100 + 20) - 25
}
