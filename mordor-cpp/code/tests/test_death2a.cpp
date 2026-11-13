#include "test_support.h"
#include "mordor/death2a.h"

using namespace mordor;

// Table 14 / R26: chance = max(0, 50 - CON*2) on the stone path.
TEST(conLossChance_table_values) {
    CHECK_EQ(conLossChancePercent(24), 2);
    CHECK_EQ(conLossChancePercent(21), 8);
    CHECK_EQ(conLossChancePercent(27), 0);
    CHECK_EQ(conLossChancePercent(1), 48);
    CHECK_EQ(conLossChancePercent(0), 50);
    CHECK_EQ(conLossChancePercent(10), 30);
}

// Quirk: the floor at 0 applies to the stone-path chance; CON 25 and up give 0.
TEST(conLossChance_floors_at_zero) {
    CHECK_EQ(conLossChancePercent(25), 0);
    CHECK_EQ(conLossChancePercent(26), 0);
    CHECK_EQ(conLossChancePercent(30), 0);
}

// R26 morgue roll: roll = 50 - Con*2, documented as unfloored.
TEST(morgueThreshold_is_unfloored) {
    CHECK_EQ(morgueConLossThreshold(10), 30);
    CHECK_EQ(morgueConLossThreshold(25), 0);
    CHECK_EQ(morgueConLossThreshold(27), -4);
}

// Loss if Int(Rnd*100) < threshold. Draw 0.295 gives 29.
TEST(rollConLoss_draw_below_threshold_loses) {
    tsup::Env env({0.295, 0.295});
    Character ch;
    CHECK(rollConLoss(env.ctx, ch, 30));
}

// Draw 0.305 gives 30, which is not strictly below 30: no loss.
TEST(rollConLoss_draw_at_threshold_no_loss) {
    tsup::Env env({0.305, 0.305});
    Character ch;
    CHECK(!rollConLoss(env.ctx, ch, 30));
}

// Threshold 0: draw 0 is not < 0, so no loss.
TEST(rollConLoss_zero_threshold_never_loses) {
    tsup::Env env({0.0, 0.0});
    Character ch;
    CHECK(!rollConLoss(env.ctx, ch, 0));
}

// Threshold 100: any draw 0..99 is < 100, so loss occurs.
TEST(rollConLoss_threshold_100_always_loses) {
    tsup::Env env({0.995, 0.995});
    Character ch;
    CHECK(rollConLoss(env.ctx, ch, 100));
}

// Stone path uses natural CON 12 -> chance 26. Draw 25 loses.
TEST(stoneConLoss_loses_below_chance) {
    tsup::Env env({0.255, 0.255});
    Character ch;
    ch.abilities.stats[static_cast<u32>(StatIndex::Con)] = 12;
    CHECK(stoneConLossRoll(env.ctx, ch));
}

// Stone path, CON 12 -> chance 26. Draw 26 is not < 26: no loss.
TEST(stoneConLoss_no_loss_at_chance) {
    tsup::Env env({0.265, 0.265});
    Character ch;
    ch.abilities.stats[static_cast<u32>(StatIndex::Con)] = 12;
    CHECK(!stoneConLossRoll(env.ctx, ch));
}

// Stone path floors at 0 for CON 30: draw 0 is not < 0.
TEST(stoneConLoss_floor_zero_at_high_con) {
    tsup::Env env({0.0, 0.0});
    Character ch;
    ch.abilities.stats[static_cast<u32>(StatIndex::Con)] = 30;
    CHECK(!stoneConLossRoll(env.ctx, ch));
}

// Morgue path (kind 23), CON 10 -> threshold 30. Draw 29 loses.
TEST(morgueConLoss_loses_below_threshold) {
    tsup::Env env({0.295, 0.295});
    Character ch;
    ch.raiseKind = RaiseKind::Morgue;
    ch.abilities.stats[static_cast<u32>(StatIndex::Con)] = 10;
    CHECK(morgueConLossRoll(env.ctx, ch));
}

// Morgue path, CON 10 -> threshold 30. Draw 30 does not lose.
TEST(morgueConLoss_no_loss_at_threshold) {
    tsup::Env env({0.305, 0.305});
    Character ch;
    ch.raiseKind = RaiseKind::Morgue;
    ch.abilities.stats[static_cast<u32>(StatIndex::Con)] = 10;
    CHECK(!morgueConLossRoll(env.ctx, ch));
}

// Table 4: age added on failure by cause.
TEST(raiseAgeDays_by_cause) {
    CHECK_EQ(raiseAgeDays(DeathState::Dead), 54);
    CHECK_EQ(raiseAgeDays(DeathState::Stoned), 84);
    CHECK_EQ(raiseAgeDays(DeathState::Rock), 124);
}

// R25 Table 9: Human, lifespan 100, age 16 years (5840 days) -> chance 32.
// Draw 31 complicates; draw 32 does not (strict <).
TEST(complication_human_16_years_below) {
    tsup::Env env({0.315, 0.315});
    CHECK(rollComplication(env.ctx, 5840, 100));
}

TEST(complication_human_16_years_at_chance) {
    tsup::Env env({0.325, 0.325});
    CHECK(!rollComplication(env.ctx, 5840, 100));
}

// Elf, lifespan 400, age 16 -> chance 8. Draw 7 complicates; draw 8 does not.
TEST(complication_elf_16_years_below) {
    tsup::Env env({0.075, 0.075});
    CHECK(rollComplication(env.ctx, 5840, 400));
}

TEST(complication_elf_16_years_at_chance) {
    tsup::Env env({0.085, 0.085});
    CHECK(!rollComplication(env.ctx, 5840, 400));
}

// Dwarf, lifespan 275, age 16 -> chance 11.636...
// Draw 11 (< 11.63) complicates; draw 12 does not.
TEST(complication_dwarf_16_years_fractional_chance) {
    tsup::Env envLow({0.115, 0.115});
    CHECK(rollComplication(envLow.ctx, 5840, 275));
    tsup::Env envHigh({0.125, 0.125});
    CHECK(!rollComplication(envHigh.ctx, 5840, 275));
}

// Age 0 -> chance 0. Draw 0 is not < 0.
TEST(complication_zero_age_never_complicates) {
    tsup::Env env({0.0, 0.0});
    CHECK(!rollComplication(env.ctx, 0, 100));
}

// Guaranteed at Age >= RaceMaxAge/2: human 100 at 50 years (18250 days).
TEST(complication_guaranteed_at_half_lifespan) {
    tsup::Env env({0.999, 0.999});
    CHECK(rollComplication(env.ctx, 18250, 100));
}

// Age 49 years (17885 days), lifespan 100 -> chance 98.
// Draw 97 complicates; draw 98 does not.
TEST(complication_49_years_below_and_at_chance) {
    tsup::Env envLow({0.975, 0.975});
    CHECK(rollComplication(envLow.ctx, 17885, 100));
    tsup::Env envHigh({0.985, 0.985});
    CHECK(!rollComplication(envHigh.ctx, 17885, 100));
}
