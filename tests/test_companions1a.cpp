#include <cmath>
#include "mordor/spells1a.h"

#include "test_support.h"
#include "mordor/companions1a.h"

namespace mr = mordor;

namespace {

bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

mr::Companion filledSlot(mr::i32 hits) {
    mr::Companion c;
    c.ref = 5;
    c.occupied = true;
    c.hits = hits;
    c.maxHits = 12;
    return c;
}

mr::Companion deadSlot() {
    mr::Companion c = filledSlot(0);
    c.isDead = true;
    c.deathState = mr::CompanionDeathState::Dead;
    return c;
}

}  // namespace

TEST(slot_first_free_hole) {
    mr::CompanionSlots slots{};
    slots[0] = filledSlot(4);
    CHECK_EQ(mr::firstFreeCompanionSlot(slots), 1);
    slots[1] = filledSlot(4);
    CHECK_EQ(mr::firstFreeCompanionSlot(slots), 2);
    slots[2] = filledSlot(4);
    slots[3] = filledSlot(4);
    CHECK_EQ(mr::firstFreeCompanionSlot(slots), -1);
}

TEST(living_count_skips_holes_and_dead) {
    mr::CompanionSlots slots{};
    slots[0] = filledSlot(3);
    slots[2] = deadSlot();
    slots[3] = filledSlot(9);
    CHECK_EQ(mr::livingCompanionCount(slots), 2);
}

TEST(alignment_from_stored_and_definition) {
    CHECK(mr::companionAlignmentFromStored(0, 1) == mr::CompanionAlignment::Good);
    CHECK(mr::companionAlignmentFromStored(1, 1) == mr::CompanionAlignment::Neutral);
    CHECK(mr::companionAlignmentFromStored(2, 1) == mr::CompanionAlignment::Evil);
    // Out of the 0..2 range: fall back to the definition's alignment.
    CHECK(mr::companionAlignmentFromStored(7, 2) == mr::CompanionAlignment::Evil);
    CHECK(mr::companionAlignmentFromStored(7, 5) == mr::CompanionAlignment::Neutral);
    CHECK(mr::companionAlignmentFromStored(-1, 0) == mr::CompanionAlignment::Good);

    CHECK(mr::alignmentOfDefinition(0) == mr::CompanionAlignment::Good);
    CHECK(mr::alignmentOfDefinition(1) == mr::CompanionAlignment::Neutral);
    CHECK(mr::alignmentOfDefinition(2) == mr::CompanionAlignment::Evil);
    CHECK(mr::alignmentOfDefinition(3) == mr::CompanionAlignment::Neutral);
    CHECK(mr::alignmentOfDefinition(-1) == mr::CompanionAlignment::Neutral);
}

TEST(id307_and_ref_resolution) {
    tsup::Env env({0.5, 0.9});
    const auto& ctx = env.ctx;

    CHECK_EQ(mr::id307Ref(ctx, mr::CompanionAlignment::Neutral), mr::kId307NeutralRef);
    CHECK_EQ(mr::id307Ref(ctx, mr::CompanionAlignment::Evil), mr::kId307EvilRef);
    // Good or unaligned: Morgul (first index).
    CHECK_EQ(mr::id307Ref(ctx, mr::CompanionAlignment::Good), mr::kId307NeutralRef);

    // A valid Ref whose Id matches is kept.
    CHECK_EQ(mr::resolveMonsterRef(ctx, 389, mr::kCompanionId307), 389);
    // No Ref: first index with Id 307 is Morgul.
    CHECK_EQ(mr::resolveMonsterRef(ctx, -1, mr::kCompanionId307), 278);

    CHECK_EQ(mr::monsterRefFromId(ctx, mr::kCompanionId307, true, mr::CompanionAlignment::Evil), 389);
    CHECK_EQ(mr::monsterRefFromId(ctx, mr::kCompanionId307, true, mr::CompanionAlignment::Neutral), 278);
    // Alignment unknown: first match.
    CHECK_EQ(mr::monsterRefFromId(ctx, mr::kCompanionId307, false, mr::CompanionAlignment::Evil), 278);

    mr::Companion c;
    c.monsterId = mr::kCompanionId307;
    c.ref = 389;
    mr::resyncCompanionRef(ctx, c);
    CHECK_EQ(c.ref, 389);
}

TEST(binding_predicates) {
    mr::Companion c;
    c.bindingLevel = 0;
    CHECK(!mr::companionBound(c));
    c.bindingLevel = mr::kFreeWillBinding;
    CHECK(!mr::companionBound(c));
    c.bindingLevel = 1;
    CHECK(mr::companionBound(c));
    c.bindingLevel = 99;
    CHECK(!mr::companionForLifeBinding(c));
    c.bindingLevel = 100;
    CHECK(mr::companionForLifeBinding(c));
    CHECK(mr::companionBound(c));
}

TEST(starting_and_bought_binding) {
    CHECK_EQ(mr::startingBinding(0), 5);
    CHECK_EQ(mr::startingBinding(4), 9);
    CHECK_EQ(mr::startingBinding(5), 10);
    CHECK_EQ(mr::startingBinding(9), 10);  // capped at 10

    CHECK_EQ(mr::boughtBinding(0), 5);
    CHECK_EQ(mr::boughtBinding(3), 8);
    CHECK_EQ(mr::boughtBinding(5), 10);
    CHECK_EQ(mr::boughtBinding(9), 10);  // capped at 10
}

TEST(next_binding_level_and_cost_factor) {
    CHECK_EQ(mr::nextBindingLevel(0), 4);
    CHECK_EQ(mr::nextBindingLevel(4), 8);
    CHECK_EQ(mr::nextBindingLevel(8), 12);
    CHECK_EQ(mr::nextBindingLevel(9), 12);
    CHECK_EQ(mr::nextBindingLevel(12), 12);
    CHECK_EQ(mr::nextBindingLevel(-4), 8);  // uses |current|

    // Factor f by a = |current| (editor-derived table).
    CHECK_EQ(mr::bindingCostFactor(0), 4);
    CHECK_EQ(mr::bindingCostFactor(1), 5);
    CHECK_EQ(mr::bindingCostFactor(8), 12);
    CHECK_EQ(mr::bindingCostFactor(9), 3);
    CHECK_EQ(mr::bindingCostFactor(10), 2);
    CHECK_EQ(mr::bindingCostFactor(11), 1);
    CHECK_EQ(mr::bindingCostFactor(12), 0);
    CHECK_EQ(mr::bindingCostFactor(-9), 3);
}

TEST(binding_raise_charge_signs_and_rounding) {
    // Binding 0 is free.
    CHECK_EQ(mr::bindingRaiseCharge(1, 0), 0);
    // GL 2, current 4, f = 8: 2 * 1237 * 8 * 4 / 4 = 19792.
    CHECK_EQ(mr::bindingRaiseCharge(2, 4), 19792);
    // Negative content binding gives a negative cost: 1 * 1237 * 8 * -4 / 4.
    CHECK_EQ(mr::bindingRaiseCharge(1, -4), -9896);
    // At max, f = 0 so the cost is 0 (port quirk).
    CHECK_EQ(mr::bindingRaiseCharge(3, 12), 0);
    // GL 1, current 9, f = 3: 33399 / 4 = 8349.75, rounds to 8350.
    CHECK_EQ(mr::bindingRaiseCharge(1, 9), 8350);
    // GL 1, current 11, f = 1: 13607 / 4 = 3401.75, rounds to 3402.
    CHECK_EQ(mr::bindingRaiseCharge(1, 11), 3402);
}

TEST(binding_purchase_gates) {
    tsup::Env env({0.5, 0.9});

    mr::Character poor;
    poor.wallet.goldOnHand = 50;
    CHECK(!mr::payCharacterGold(env.ctx, poor, 100));
    CHECK_EQ(poor.purse(), 50);
    CHECK(mr::payCharacterGold(env.ctx, poor, 30));
    CHECK_EQ(poor.purse(), 20);

    mr::Character owner;
    owner.wallet.goldOnHand = 20000;
    mr::Companion forLife;
    forLife.bindingLevel = 100;
    CHECK(!mr::raiseBinding(env.ctx, owner, forLife));
    CHECK_EQ(forLife.bindingLevel, 100);
    CHECK_EQ(owner.purse(), 20000);

    // Paid raise: binding 4 -> 8, cost 9896 (GL 1).
    mr::Companion c;
    c.bindingLevel = 4;
    c.guildLevel = 1;
    CHECK(mr::raiseBinding(env.ctx, owner, c));
    CHECK_EQ(c.bindingLevel, 8);
    CHECK_EQ(owner.purse(), 10104);

    // Not enough gold: refused, binding unchanged.
    mr::Character broke;
    broke.wallet.goldOnHand = 100;
    mr::Companion d;
    d.bindingLevel = 4;
    d.guildLevel = 1;
    CHECK(!mr::raiseBinding(env.ctx, broke, d));
    CHECK_EQ(d.bindingLevel, 4);
    CHECK_EQ(broke.purse(), 100);
}

TEST(binding_constants_match_srd) {
    CHECK_EQ(mr::kBindingBase, 5);
    CHECK_EQ(mr::kStartBindingCap, 10);
    CHECK_EQ(mr::kBoughtBindingCap, 10);
    CHECK_EQ(mr::kBindingStep, 4);
    CHECK_EQ(mr::kBindingPurchaseMax, 12);
    CHECK_EQ(mr::kBindingCostConst, 1237);
    CHECK_EQ(mr::kBindingCostDivisor, 4);
    CHECK_EQ(mr::kFreeWillBinding, -10);
    CHECK_EQ(mr::kBindingMax, 100);
    CHECK_EQ(mr::kJoinRollMax, 3000);
    CHECK_EQ(mr::kJoinBarChaScale, 10);
    CHECK_EQ(mr::kSubtypeResetTries, 150);
    CHECK_EQ(mr::kSubtypeFailTries, 300);
    CHECK_EQ(mr::kBreathChanceStrong, 80);
    CHECK_EQ(mr::kBreathChanceWeak, 20);
    CHECK_EQ(mr::kSpitRetryCap, 1000);
    CHECK(near(mr::kMishapRows[0].factor, 2.0));
    CHECK(near(mr::kMishapRows[2].high, 20.0));
}

TEST(join_bar_formula_and_floor) {
    CHECK_EQ(mr::joinBar(10, 1), 100);
    CHECK_EQ(mr::joinBar(3, 1), 30);
    CHECK_EQ(mr::joinBar(20, 3), 180);
    CHECK_EQ(mr::joinBar(3, 5), 0);  // 30 - 40 floors at 0
}

TEST(group_may_offer_join_rules) {
    mr::MonsterGroup g;
    g.front.align = mr::CharAlign::Neutral;
    g.front.guildLevel = 0;
    // Neutral (1) may offer to anyone.
    CHECK(mr::groupMayOfferJoin(g, mr::Align::Evil, 0));

    g.front.align = mr::CharAlign::Evil;
    CHECK(!mr::groupMayOfferJoin(g, mr::Align::Good, 5));
    g.front.guildLevel = 2;
    // Guild level 2 exceeds viewer's top guild level 1.
    CHECK(!mr::groupMayOfferJoin(g, mr::Align::Evil, 1));
    g.front.guildLevel = 1;
    CHECK(mr::groupMayOfferJoin(g, mr::Align::Evil, 1));

    g.front.align = mr::CharAlign::Good;
    g.front.guildLevel = 1;
    CHECK(mr::groupMayOfferJoin(g, mr::Align::Good, 1));

    // Engaged groups never offer.
    g.state = mr::GroupState::Engaged;
    CHECK(!mr::groupMayOfferJoin(g, mr::Align::Good, 1));
}

TEST(roll_join_offer_timing_and_cap) {
    // t = Int(0.5*3000)+1 = 1501 > bar 100: no offer.
    {
        tsup::Env env({0.5, 0.5});
        CHECK_EQ(mr::rollJoinOffer(env.ctx, 10, 1, 3), 0);
    }
    // t = Int(0.0334*3000)+1 = 101 > 100: no offer.
    {
        tsup::Env env({0.0334, 0.9});
        CHECK_EQ(mr::rollJoinOffer(env.ctx, 10, 1, 3), 0);
    }
    // t = Int(0.0333*3000)+1 = 100 <= 100: offer. Count = Int(0.99*min(10,4))+1 = 4.
    {
        tsup::Env env({0.0333, 0.99});
        CHECK_EQ(mr::rollJoinOffer(env.ctx, 10, 1, 10), 4);
    }
    // t = 1 <= 100: offer. Count = Int(0*1)+1 = 1.
    {
        tsup::Env env({0.0, 0.0});
        CHECK_EQ(mr::rollJoinOffer(env.ctx, 10, 1, 1), 1);
    }
}

TEST(monster_join_roll_boundary) {
    // rand(1,3000) = 100 <= 100: join.
    {
        tsup::Env env({0.0333, 0.9});
        CHECK(mr::monsterJoinRoll(env.ctx, 10, 1));
    }
    // rand(1,3000) = 101 > 100: no join.
    {
        tsup::Env env({0.0334, 0.9});
        CHECK(!mr::monsterJoinRoll(env.ctx, 10, 1));
    }
    // Bar is negative (3 - 5 + 1 scaled): never joins.
    {
        tsup::Env env({0.0, 0.9});
        CHECK(!mr::monsterJoinRoll(env.ctx, 3, 5));
    }
}

TEST(joiner_copies_front_and_binds_free_will) {
    mr::Creature front;
    front.hits = 7;
    front.maxHits = 12;
    mr::Companion j = mr::makeJoinerFromFront(front, 2, 1);
    CHECK_EQ(j.hits, 7);
    CHECK_EQ(j.maxHits, 12);
    CHECK_EQ(j.bindingLevel, mr::kFreeWillBinding);
    CHECK_EQ(j.owner, 2);
    CHECK_EQ(j.slot, 1);
    CHECK(!mr::companionBound(j));
}

TEST(accept_join_offer_fills_up_to_offer_count) {
    tsup::Env env({0.5, 0.9});
    mr::Character owner;
    mr::CompanionSlots slots{};
    mr::MonsterGroup group;
    // R2/R3: a live monster has a real creature table Ref. Ref -1 marks an empty slot,
    // so the front monster must carry a valid Ref for its joiners to occupy slots.
    group.front.ref = 5;
    group.front.hits = 7;
    group.front.maxHits = 12;
    group.count = 3;
    group.offerCount = 2;
    mr::acceptJoinOffer(env.ctx, owner, 0, slots, group);
    CHECK_EQ(slots[0].hits, 7);
    CHECK_EQ(slots[0].bindingLevel, mr::kFreeWillBinding);
    CHECK_EQ(slots[1].hits, 7);
    CHECK_EQ(slots[1].bindingLevel, mr::kFreeWillBinding);
    CHECK_EQ(slots[2].hits, 0);  // only two offered

    // No free slot: existing companions are untouched.
    mr::CompanionSlots full{};
    for (int i = 0; i < 4; ++i) full[i] = filledSlot(99);
    mr::MonsterGroup g2;
    g2.front.ref = 5;
    g2.front.hits = 7;
    g2.count = 3;
    g2.offerCount = 2;
    mr::acceptJoinOffer(env.ctx, owner, 0, full, g2);
    CHECK_EQ(full[0].hits, 99);
    CHECK_EQ(full[3].hits, 99);
}

TEST(join_refused_dead_only_with_monsters) {
    mr::Creature dead;
    dead.alive = false;
    dead.hits = 0;
    dead.deathState = 1;
    CHECK(mr::joinRefusedDead(dead, true));
    CHECK(!mr::joinRefusedDead(dead, false));

    mr::Creature living;
    living.alive = true;
    living.hits = 5;
    living.deathState = 0;
    CHECK(!mr::joinRefusedDead(living, true));
}

TEST(charm_force_join_and_voluntary_probability) {
    // "Exceeds": strictly greater than remaining HP.
    CHECK(mr::charmForcesJoin(10, 5));
    CHECK(!mr::charmForcesJoin(5, 5));
    CHECK(!mr::charmForcesJoin(4, 5));

    // Proposed formula: max(0, CHA - M + 1) / 300.
    CHECK(near(mr::voluntaryJoinProbability(30, 1), 0.1));
    CHECK(near(mr::voluntaryJoinProbability(3, 10), 0.0));
}

TEST(subtype_reject_counter_limits) {
    mr::SubtypePick p;
    p.subtype = 5;
    p.tries = 149;
    p = mr::rejectSubtypeDraw(p);
    CHECK_EQ(p.tries, 150);
    CHECK_EQ(p.subtype, 5);  // 150 is not > 150
    CHECK(!p.failed);

    mr::SubtypePick q;
    q.subtype = 5;
    q.tries = 150;
    q = mr::rejectSubtypeDraw(q);
    CHECK_EQ(q.tries, 151);
    CHECK_EQ(q.subtype, 0);  // reset after 150 rejections
    CHECK(!q.failed);

    mr::SubtypePick r;
    r.tries = 299;
    r = mr::rejectSubtypeDraw(r);
    CHECK_EQ(r.tries, 300);
    CHECK(!r.failed);

    mr::SubtypePick s;
    s.tries = 300;
    s = mr::rejectSubtypeDraw(s);
    CHECK_EQ(s.tries, 301);
    CHECK(s.failed);
}

TEST(spawn_pick_modes) {
    mr::SpawnWindow prev;
    prev.below = 4;
    prev.above = 6;

    mr::SpawnPick p0 = mr::spawnPickForMode(0, prev);
    CHECK_EQ(p0.window.below, 3);
    CHECK_EQ(p0.window.above, -1);
    CHECK_EQ(p0.tableEntry, -1);

    mr::SpawnPick p1 = mr::spawnPickForMode(1, prev);
    CHECK_EQ(p1.window.below, 3);
    CHECK_EQ(p1.window.above, 0);

    mr::SpawnPick p2 = mr::spawnPickForMode(2, prev);
    CHECK_EQ(p2.window.below, 0);
    CHECK_EQ(p2.window.above, 0);

    CHECK(mr::spawnPickForMode(3, prev).leadItself);

    CHECK_EQ(mr::spawnPickForMode(4, prev).tableEntry, 4);
    CHECK_EQ(mr::spawnPickForMode(5, prev).tableEntry, 5);
    CHECK_EQ(mr::spawnPickForMode(6, prev).tableEntry, 4);
    CHECK_EQ(mr::spawnPickForMode(10, prev).tableEntry, 3);  // mode - 7

    // Mode 7 and negative modes reuse the previous window.
    mr::SpawnPick p7 = mr::spawnPickForMode(7, prev);
    CHECK_EQ(p7.window.below, 4);
    CHECK_EQ(p7.window.above, 6);
    mr::SpawnPick pn = mr::spawnPickForMode(-2, prev);
    CHECK_EQ(pn.window.below, 4);
    CHECK_EQ(pn.window.above, 6);
}

TEST(breath_guard_fire_ignores_earlier_cold) {
    CHECK(mr::companionBreathGuard(true, false, false));
    // Fire is never stopped by an earlier breath.
    CHECK(mr::companionBreathGuard(true, false, true));
    CHECK(mr::companionBreathGuard(false, true, false));
    // Cold is stopped once a breath has happened this round.
    CHECK(!mr::companionBreathGuard(false, true, true));
    CHECK(!mr::companionBreathGuard(false, false, false));
}

TEST(breath_percent_and_range) {
    // Same level: base roll 50.
    CHECK_EQ(mr::companionBreathPercent(10, 10), 50);
    // Lead foe far stronger than companion: 80.
    CHECK_EQ(mr::companionBreathPercent(4, 12), 80);
    // Lead foe far weaker than companion: 20.
    CHECK_EQ(mr::companionBreathPercent(12, 4), 20);

    // a = atk+1, tgt = 0: Int((ln a + a/15)/ln 1.08 + 1).
    CHECK_EQ(mr::specialHitRange(0, 0), 1);   // a=1: 1.87 -> 1
    CHECK_EQ(mr::specialHitRange(9, 0), 39);  // a=10: 39.58 -> 39
}

TEST(mishap_chance_formula_and_clamps) {
    // Worked check: 999/100 * 5/20 * factor.
    CHECK(near(mr::mishapChance(mr::MishapKind::Breath, 100, 5), 4.995));
    CHECK(near(mr::mishapChance(mr::MishapKind::Spell, 100, 5), 3.74625));
    CHECK(near(mr::mishapChance(mr::MishapKind::Spit, 100, 5), 2.4975));

    // Level-1 companion is pinned at the clamp ceiling: 999*0.05*2 = 99.9 -> 40.
    CHECK(near(mr::mishapChance(mr::MishapKind::Breath, 1, 1), 40.0));
    CHECK(near(mr::mishapChance(mr::MishapKind::Spit, 1, 5), 20.0));
    // Low clamps: spit 0.05 -> 0.5; spell 0.075 -> 1.
    CHECK(near(mr::mishapChance(mr::MishapKind::Spit, 999, 1), 0.5));
    CHECK(near(mr::mishapChance(mr::MishapKind::Spell, 999, 1), 1.0));
}

TEST(mishap_roll_boundary_and_bodies) {
    CHECK_EQ(mr::countBodies(3, 2), 5);
    CHECK_EQ(mr::countBodies(4, 0), 4);

    // rng*100 = 4.99 < 4.995: mishap.
    {
        tsup::Env env({0.0499, 0.9});
        CHECK(mr::rollMishap(env.ctx, 4.995));
    }
    // rng*100 = 5.0 is not < 4.995: no mishap.
    {
        tsup::Env env({0.05, 0.9});
        CHECK(!mr::rollMishap(env.ctx, 4.995));
    }
}

TEST(buy_companion_refused_when_no_free_slot) {
    tsup::Env env({0.5, 0.9});
    mr::Character buyer;
    buyer.wallet.goldOnHand = 1000;
    mr::CompanionSlots slots{};
    for (int i = 0; i < 4; ++i) slots[i] = filledSlot(5);
    mr::Pen pen{};
    CHECK(!mr::buyCompanion(env.ctx, buyer, slots, pen, mr::CompanionAlignment::Neutral,
                            mr::kId307NeutralRef, 100));
    CHECK_EQ(buyer.purse(), 1000);
    CHECK_EQ(slots[0].hits, 5);
}

