#include <cmath>

#include "test_support.h"
#include "mordor/death1a.h"

using namespace mordor;

namespace {

template <typename E>
i32 ordOf(E e) {
    return static_cast<i32>(e);
}

Character pendingBody() {
    Character c;
    c.raiseChance = 40;
    c.raiseKind = RaiseKind::RaiseDead;
    return c;
}

}  // namespace

TEST(death_state_names_follow_cause_codes) {
    CHECK(deathStateName(0) == Str("Dead"));
    CHECK(deathStateName(1) == Str("Dead"));
    CHECK(deathStateName(2) == Str("Stoned"));
    // Original misspelling is kept on purpose (SRD R3 quirk).
    CHECK(deathStateName(6) == Str("Annihalated"));
    CHECK(deathStateName(8) == Str("Rock"));
}

TEST(body_area_prefers_stored_area) {
    Character who;
    who.location.area = 5;
    CHECK_EQ(bodyAreaOr(who, 9), 5);
    // 0 is a legal stored sub-location (below ground), so it counts as set.
    who.location.area = 0;
    CHECK_EQ(bodyAreaOr(who, 9), 0);
}

TEST(body_area_rock_death_uses_depth_not_square) {
    Character who;
    who.deathState = DeathState::Rock;
    who.location.area = kAreaUnset;
    who.location.dungeonLevel = 2;
    CHECK_EQ(bodyAreaOr(who, 9), kRockAreaBelowGround);
    who.location.dungeonLevel = 0;
    CHECK_EQ(bodyAreaOr(who, 9), kRockAreaAboveGround);
}

TEST(body_area_falls_back_to_square_when_unset) {
    Character who;
    who.location.area = kAreaUnset;
    CHECK_EQ(bodyAreaOr(who, 9), 9);  // alive
    who.deathState = DeathState::Dead;  // ordinary death, not rock
    CHECK_EQ(bodyAreaOr(who, 9), 9);
}

TEST(rock_area_setter_keeps_depth_and_sets_area_zero) {
    Character who;
    who.location.dungeonLevel = 3;
    who.location.area = kAreaUnset;
    setAreaForRockDeath(who);
    CHECK_EQ(who.location.dungeonLevel, 3);
    CHECK_EQ(who.location.area, 0);
}

TEST(rock_area_setter_clamps_town_and_negative_depth) {
    Character a;
    a.location.dungeonLevel = 0;
    a.location.area = 6;
    setAreaForRockDeath(a);
    CHECK_EQ(a.location.dungeonLevel, 0);
    CHECK_EQ(a.location.area, -1);

    Character b;
    b.location.dungeonLevel = -2;
    b.location.area = 6;
    setAreaForRockDeath(b);
    CHECK_EQ(b.location.dungeonLevel, 0);
    CHECK_EQ(b.location.area, -1);
}

TEST(record_death_sets_fields_and_counts_once) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.vitals.curHits = 5;
    who.location.area = 4;
    bool counted = false;
    recordDeath(env.ctx, who, DeathState::Stoned, 9, counted);
    CHECK_EQ(who.vitals.curHits, 0);
    CHECK(who.isDead());
    CHECK_EQ(ordOf(who.deathState), ordOf(DeathState::Stoned));
    CHECK_EQ(who.deaths, 1);
    CHECK(counted);
    CHECK_EQ(who.location.area, 4);  // stored area is kept
}

TEST(record_death_already_counted_is_not_recounted) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.vitals.curHits = 3;
    bool counted = true;
    recordDeath(env.ctx, who, DeathState::Dead, 2, counted);
    CHECK_EQ(who.vitals.curHits, 0);
    CHECK(who.isDead());
    CHECK_EQ(who.deaths, 0);
}

TEST(dead_screen_owed_only_when_dead_and_alone) {
    Party party;
    Character who;
    party.size = 1;
    CHECK(!deadScreenOwed(who, party));  // alive
    who.deathState = DeathState::Dead;
    CHECK(deadScreenOwed(who, party));
    party.size = 2;
    CHECK(!deadScreenOwed(who, party));
}

TEST(rescue_wait_only_for_dead_below_ground) {
    Character who;
    who.location.dungeonLevel = 3;
    CHECK(!needsRescueWait(who));  // alive
    who.deathState = DeathState::Dead;
    who.location.dungeonLevel = 0;
    CHECK(!needsRescueWait(who));  // dead in town
    who.location.dungeonLevel = 3;
    CHECK(needsRescueWait(who));   // dead below ground
}

TEST(rescue_coordinate_bounds) {
    CHECK(validRescueCoordinates(1, 1, 1, kRescueCoordParts));
    CHECK(validRescueCoordinates(5, 7, kRescueLevelMax, kRescueCoordParts));
    CHECK(!validRescueCoordinates(0, 1, 1, kRescueCoordParts));
    CHECK(!validRescueCoordinates(1, 0, 1, kRescueCoordParts));
    CHECK(!validRescueCoordinates(1, 1, 0, kRescueCoordParts));
    CHECK(!validRescueCoordinates(1, 1, kRescueLevelMax + 1, kRescueCoordParts));
    CHECK(!validRescueCoordinates(1, 1, 1, 2));
    CHECK(!validRescueCoordinates(1, 1, 1, 4));
}

TEST(close_enough_is_strict_eight_and_same_level) {
    // Target 1-based position (10,10), level 3.
    CHECK(closeEnough(10, 10, 3, 10, 10, 3));
    CHECK(closeEnough(17, 10, 3, 10, 10, 3));   // |dx| = 7
    CHECK(!closeEnough(18, 10, 3, 10, 10, 3));  // |dx| = 8 is not close
    CHECK(closeEnough(10, 3, 3, 10, 10, 3));    // |dy| = 7
    CHECK(!closeEnough(10, 2, 3, 10, 10, 3));   // |dy| = 8
    CHECK(!closeEnough(10, 10, 4, 10, 10, 3));  // level differs
}

TEST(rescuers_return_threshold_is_four_percent_per_level) {
    // Return when Rnd * 100 > level * 4.
    tsup::Env e1({0.05, 0.05});
    CHECK(rescuersReturn(e1.ctx, 1));   // 5 > 4
    tsup::Env e2({0.03, 0.03});
    CHECK(!rescuersReturn(e2.ctx, 1));  // 3 > 4 is false
    tsup::Env e3({0.99, 0.99});
    CHECK(!rescuersReturn(e3.ctx, 25)); // 100 threshold: never returns
    tsup::Env e4({0.99, 0.99});
    CHECK(rescuersReturn(e4.ctx, 24));  // 99 > 96
}

TEST(rescue_search_cost_formula) {
    // cost = trunc(372567.52 * ln(L+1) * L)
    CHECK_EQ(rescueSearchCost(1),
             static_cast<Currency>(std::trunc(372567.52 * std::log(2.0) * 1.0)));
    CHECK_EQ(rescueSearchCost(10),
             static_cast<Currency>(std::trunc(372567.52 * std::log(11.0) * 10.0)));
}

TEST(morgue_counter_chance_formula_and_clamp) {
    // chance = clamp(35 * (CON / 4.5) + 10, 10, 98)
    CHECK_EQ(morgueCounterChance(9, DeathState::Dead), 80);   // 70 + 10 exact
    CHECK_EQ(morgueCounterChance(3, DeathState::Dead), 33);   // 23.33 + 10
    CHECK_EQ(morgueCounterChance(0, DeathState::Dead), 10);   // low clamp
    CHECK_EQ(morgueCounterChance(50, DeathState::Dead), 98);  // high clamp
}

TEST(guild_pays_fee_only_low_level_and_short_purse) {
    const Currency fee = 1000000;
    CHECK(guildPaysRaiseFee(kGuildPaysMaxTopLevel, 0, fee));
    CHECK(guildPaysRaiseFee(0, fee - 1, fee));
    CHECK(!guildPaysRaiseFee(kGuildPaysMaxTopLevel + 1, 0, fee));
    CHECK(!guildPaysRaiseFee(kGuildPaysMaxTopLevel, fee, fee));  // not short
}

TEST(morgue_fee_settlement_outcomes) {
    tsup::Env env({0.5, 0.9});
    const Currency fee = 1000000;

    Character payer;
    payer.topGuildLevel = 5;
    payer.wallet.goldOnHand = 0;
    CHECK_EQ(ordOf(settleMorgueFee(env.ctx, payer, fee)), ordOf(MorgueFeeOutcome::GuildPays));
    CHECK_EQ(payer.purse(), 0);

    Character poor;
    poor.topGuildLevel = 6;
    poor.wallet.goldOnHand = fee - 1;
    CHECK_EQ(ordOf(settleMorgueFee(env.ctx, poor, fee)), ordOf(MorgueFeeOutcome::NotEnoughGold));
    CHECK_EQ(poor.purse(), fee - 1);

    Character rich;
    rich.topGuildLevel = 6;
    rich.wallet.goldOnHand = fee + 500000;
    CHECK_EQ(ordOf(settleMorgueFee(env.ctx, rich, fee)), ordOf(MorgueFeeOutcome::Charged));
    CHECK_EQ(rich.purse(), 500000);
}

TEST(counter_raise_refused_when_already_pending) {
    Character body;
    body.deathState = DeathState::Dead;
    body.raiseChance = 40;
    CHECK_EQ(ordOf(counterRaiseRefusal(body)), ordOf(CounterRaiseRefusal::AlreadyPending));
    body.raiseChance = 0;
    CHECK_EQ(ordOf(counterRaiseRefusal(body)), ordOf(CounterRaiseRefusal::None));
}

TEST(raise_accept_settlement_depends_on_success) {
    Character failed = pendingBody();
    failed.raisedBy = Str("Bob");
    settleRaiseAccept(failed, false);
    CHECK_EQ(ordOf(failed.raiseKind), ordOf(RaiseKind::None));
    CHECK_EQ(failed.raiseChance, 0);
    CHECK(failed.raisedBy == Str());

    Character ok = pendingBody();
    ok.raisedBy = Str("Bob");
    settleRaiseAccept(ok, true);
    CHECK_EQ(ordOf(ok.raiseKind), ordOf(RaiseKind::RaiseDead));
    CHECK_EQ(ok.raiseChance, 0);
    CHECK(ok.raisedBy == Str());
}

TEST(clear_pending_raise_clears_chance_and_raiser) {
    Character who = pendingBody();
    who.raisedBy = Str("Bob");
    clearPendingRaise(who);
    CHECK_EQ(who.raiseChance, 0);
    CHECK(who.raisedBy == Str());
}

TEST(initiate_pending_raise_moves_body_and_skips_zero_chance) {
    tsup::Env env({0.5, 0.9});
    Character raiser;
    raiser.name = Str("Ann");
    raiser.location.x = 3;
    raiser.location.y = 4;
    raiser.location.dungeonLevel = 6;
    raiser.location.area = -1;

    Character body;
    body.location.area = 5;
    initiatePendingRaise(env.ctx, body, raiser, 40, RaiseKind::RaiseDead);
    CHECK_EQ(body.raiseChance, 40);
    CHECK(body.raisedBy == Str("Ann"));
    CHECK_EQ(ordOf(body.raiseKind), ordOf(RaiseKind::RaiseDead));
    CHECK_EQ(body.location.x, 3);
    CHECK_EQ(body.location.y, 4);
    CHECK_EQ(body.location.dungeonLevel, 6);
    CHECK_EQ(body.location.area, -1);  // copied whatever its value

    Character idle;
    initiatePendingRaise(env.ctx, idle, raiser, 0, RaiseKind::RaiseDead);
    CHECK_EQ(idle.raiseChance, 0);
    CHECK_EQ(ordOf(idle.raiseKind), ordOf(RaiseKind::None));
    // SRD-AMBIGUOUS: R11.1 "sets the chance, RaisedBy and RaiseKind (when chance > 0)" does not
    // settle whether the parenthetical gates RaisedBy or only RaiseKind. Code gates only RaiseKind,
    // so RaisedBy is set even at chance 0.
    CHECK(idle.raisedBy == Str("Ann"));
}

TEST(ask_to_join_refusal_reasons) {
    Character none;
    CHECK_EQ(ordOf(askToJoinRefusal(pendingBody(), true, false)), ordOf(AskToJoinRefusal::InCombat));
    CHECK_EQ(ordOf(askToJoinRefusal(pendingBody(), false, true)), ordOf(AskToJoinRefusal::Carried));
    CHECK_EQ(ordOf(askToJoinRefusal(none, false, false)), ordOf(AskToJoinRefusal::NoPendingRaise));
    CHECK_EQ(ordOf(askToJoinRefusal(pendingBody(), false, false)), ordOf(AskToJoinRefusal::None));
}

TEST(ask_to_join_refused_in_combat_or_without_raise_and_declined) {
    tsup::Env env({0.5, 0.9});

    Character inFight = pendingBody();
    AskToJoinResult r1 = askToJoin(env.ctx, inFight, true, false, false, true);
    CHECK_EQ(ordOf(r1.outcome), ordOf(AskToJoinOutcome::Refused));
    CHECK_EQ(ordOf(r1.refusal), ordOf(AskToJoinRefusal::InCombat));

    Character noRaise;
    AskToJoinResult r2 = askToJoin(env.ctx, noRaise, false, false, false, true);
    CHECK_EQ(ordOf(r2.outcome), ordOf(AskToJoinOutcome::Refused));
    CHECK_EQ(ordOf(r2.refusal), ordOf(AskToJoinRefusal::NoPendingRaise));

    Character offered = pendingBody();
    AskToJoinResult r3 = askToJoin(env.ctx, offered, false, false, false, false);
    CHECK_EQ(ordOf(r3.outcome), ordOf(AskToJoinOutcome::Declined));
    CHECK_EQ(ordOf(r3.refusal), ordOf(AskToJoinRefusal::None));
}

TEST(carry_to_town_and_rescue_found_update_location_and_age) {
    tsup::Env env({0.5, 0.9});

    Character carried;
    carried.deathState = DeathState::Dead;
    carried.location.dungeonLevel = 4;
    carried.location.area = 7;
    carryBodyToTown(carried);
    CHECK_EQ(carried.location.dungeonLevel, 0);
    CHECK_EQ(carried.location.area, -1);
    CHECK(carried.isDead());

    Character found;
    found.deathState = DeathState::Dead;
    found.location.dungeonLevel = 4;
    found.location.x = 2;
    found.location.y = 3;
    found.ageDays = 1000;
    applyRescueFound(env.ctx, found, 7);
    CHECK_EQ(found.location.dungeonLevel, 0);
    CHECK_EQ(found.ageDays, 1007);
    CHECK(found.isDead());
    CHECK_EQ(found.location.x, 2);  // X and Y stay
    CHECK_EQ(found.location.y, 3);
}
