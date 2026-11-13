#include <cmath>

#include "test_support.h"
#include "mordor/death3.h"

using namespace mordor;

namespace {

// Builds a body whose guild level and character level agree, so the
// morgue level check gives the same answer whichever field it reads.
Character makeBody(DeathState state, i32 level, i32 maxHits) {
    Character ch;
    ch.deathState = state;
    ch.level = level;
    ch.guilds[static_cast<u32>(Guild::Nomad)].level = level;
    ch.vitals.maxHits = maxHits;
    return ch;
}

}  // namespace

TEST(death3_constants_match_srd) {
    CHECK_EQ(kMorgueOwnBodyBonus, 40);
    CHECK_EQ(kMorgueCounterBonus, 10);
    CHECK_EQ(kRockStoredRaiseChance, 10);
    CHECK_EQ(kAgeQuietFailure, 54);
    CHECK_EQ(kAgeStoned, 84);
    CHECK_EQ(kAgeRock, 124);
    CHECK_EQ(kMorgueMinLevel, 1);
    CHECK_EQ(kMorgueMaxLevel, 15);
    CHECK_EQ(kGuildPaysMaxLevel, 5);
    CHECK_EQ(kOldAgePct, 98);
}

TEST(death3_old_age_predicate_away_from_boundary) {
    // The SRD is inconsistent about pct == 98 itself, so only test clear cases.
    CHECK(!isOldAgePct(97));
    CHECK(!isOldAgePct(50));
    CHECK(isOldAgePct(99));
    CHECK(isOldAgePct(120));
}

TEST(death3_old_age_term_at_99_is_exactly_five) {
    // 5 + Ln(99 - 98) = 5 + Ln(1) = 5
    CHECK(std::fabs(oldAgeTerm(99) - 5.0) < 1e-12);
}

TEST(death3_old_age_term_at_101) {
    // 5 + Ln(101 - 98) = 5 + Ln(3)
    const double expected = 5.0 + std::log(3.0);
    CHECK(std::fabs(oldAgeTerm(101) - expected) < 1e-9);
}

TEST(death3_clamp_raise_chance_bounds) {
    CHECK_EQ(clampRaiseChance(-5), 10);
    CHECK_EQ(clampRaiseChance(5), 10);
    CHECK_EQ(clampRaiseChance(10), 10);
    CHECK_EQ(clampRaiseChance(55), 55);
    CHECK_EQ(clampRaiseChance(98), 98);
    CHECK_EQ(clampRaiseChance(99), 98);
    CHECK_EQ(clampRaiseChance(150), 98);
}

TEST(death3_morgue_refuses_level_zero_as_too_low) {
    Character ch = makeBody(DeathState::Dead, 0, 15);
    CHECK(morgueRefusal(ch) == MorgueRefusal::LevelTooLow);
}

TEST(death3_morgue_accepts_level_bounds_one_and_fifteen) {
    Character low = makeBody(DeathState::Dead, 1, 15);
    CHECK(morgueRefusal(low) == MorgueRefusal::None);

    Character high = makeBody(DeathState::Dead, 15, 15);
    CHECK(morgueRefusal(high) == MorgueRefusal::None);
}

TEST(death3_morgue_refuses_level_sixteen_as_too_high) {
    Character ch = makeBody(DeathState::Dead, 16, 15);
    CHECK(morgueRefusal(ch) == MorgueRefusal::LevelTooHigh);
}

TEST(death3_morgue_refuses_rock_body) {
    Character ch = makeBody(DeathState::Rock, 5, 15);
    CHECK(morgueRefusal(ch) == MorgueRefusal::RockCannotHelp);
}

TEST(death3_revival_fee_formula_max_hits_15) {
    // Int(2912.32 * (15 / 1.5)) = Int(29123.2) = 29123 gold.
    // Currency is i64 in 1/10000 gold, so the expected value is currencyFromWhole(29123).
    Character ch = makeBody(DeathState::Dead, 5, 15);
    CHECK_EQ(revivalFee(ch), currencyFromWhole(29123));
}

TEST(death3_revival_fee_formula_max_hits_30) {
    // Int(2912.32 * (30 / 1.5)) = Int(58246.4) = 58246 gold.
    // Currency is i64 in 1/10000 gold, so the expected value is currencyFromWhole(58246).
    Character ch = makeBody(DeathState::Dead, 5, 30);
    CHECK_EQ(revivalFee(ch), currencyFromWhole(58246));
}

TEST(death3_guild_pays_level_five_when_short) {
    Character ch = makeBody(DeathState::Dead, 5, 15);
    ch.topGuildLevel = 5;
    ch.wallet.goldOnHand = 0;
    ch.wallet.goldInBank = 0;
    const Currency fee = revivalFee(ch);
    CHECK(guildPaysRevival(ch, fee));
}

TEST(death3_guild_does_not_pay_level_six_when_short) {
    Character ch = makeBody(DeathState::Dead, 6, 15);
    ch.topGuildLevel = 6;
    ch.wallet.goldOnHand = 0;
    ch.wallet.goldInBank = 0;
    const Currency fee = revivalFee(ch);
    CHECK(!guildPaysRevival(ch, fee));
}

TEST(death3_guild_does_not_pay_when_affordable) {
    Character ch = makeBody(DeathState::Dead, 3, 15);
    ch.topGuildLevel = 3;
    // 1,000,000 gold in Currency units (1/10000 gold). The fee is about 29,123 gold, so this is affordable.
    ch.wallet.goldOnHand = currencyFromWhole(1000000);
    const Currency fee = revivalFee(ch);
    CHECK(!guildPaysRevival(ch, fee));
}

TEST(death3_quote_revival_level_six_asks_payer) {
    Character ch = makeBody(DeathState::Dead, 6, 15);
    ch.topGuildLevel = 6;
    ch.wallet.goldOnHand = 0;
    RevivalPayment p = quoteRevival(ch);
    // Fee is 29123 gold, expressed in Currency units (1/10000 gold).
    CHECK_EQ(p.fee, currencyFromWhole(29123));
    CHECK(!p.guildPays);
    CHECK(p.asksPayer);
}

TEST(death3_quote_revival_level_five_short_asks_nothing) {
    Character ch = makeBody(DeathState::Dead, 5, 15);
    ch.topGuildLevel = 5;
    ch.wallet.goldOnHand = 0;
    RevivalPayment p = quoteRevival(ch);
    // Fee is 29123 gold, expressed in Currency units (1/10000 gold).
    CHECK_EQ(p.fee, currencyFromWhole(29123));
    CHECK(p.guildPays);
    CHECK(!p.asksPayer);
}

TEST(death3_raise_age_cost_by_cause) {
    CHECK_EQ(raiseAgeCost(DeathState::Dead), 54);
    CHECK_EQ(raiseAgeCost(DeathState::Stoned), 84);
    CHECK_EQ(raiseAgeCost(DeathState::Rock), 124);
}

TEST(death3_complication_guaranteed_at_half_max_age) {
    // RaceMaxAge 200: threshold is age >= 100
    CHECK(!complicationGuaranteed(99, 200));
    CHECK(complicationGuaranteed(100, 200));
    CHECK(complicationGuaranteed(150, 200));
}

TEST(death3_complicated_max_hits_is_divided_by_1_2) {
    // Int(MaxHits / 1.2)
    CHECK_EQ(complicatedMaxHits(60), 50);
    CHECK_EQ(complicatedMaxHits(24), 20);
    CHECK_EQ(complicatedMaxHits(100), 83);
}

TEST(death3_rock_spell_raise_stored_chance_is_ten) {
    Character ch = makeBody(DeathState::Rock, 5, 15);
    CHECK_EQ(spellRaiseChance(ch, RaiseKind::Resurrect), kRockStoredRaiseChance);
    CHECK_EQ(spellRaiseChance(ch, RaiseKind::Resurrect), 10);
}

TEST(death3_dead_body_gets_no_city_rest) {
    Character ch = makeBody(DeathState::Dead, 5, 15);
    CHECK(!cityRestApplies(ch));
}

TEST(death3_hall_date_field_applies_day_offset) {
    CHECK_EQ(hallDateField(100), 100 + kHallDateDayOffset);
}

TEST(death3_hall_stat_field_applies_scale) {
    CHECK_EQ(hallStatField(7), 7 * kHallStatScale);
}

TEST(death3_raise_rock_at_morgue_is_refused) {
    tsup::Env env({0.5, 0.9});
    Character ch = makeBody(DeathState::Rock, 5, 15);
    PlayerRaiseResult out = raiseCharacter(env.ctx, ch, RaiseKind::Morgue, 50);
    CHECK(out.outcome == RaiseAttemptOutcome::Refused);
}
