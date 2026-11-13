// FIXED: tsup::Env rolls are doubles in [0,1). Braced `{0.x}` lists narrowed to
// mordor::u64, so each roll list is now an explicit std::initializer_list<double>.
// If Env's constructor takes only u64 rolls, test_support.h must accept doubles.
#include <initializer_list>
#include "test_support.h"
#include "mordor/loot1a.h"

using namespace mordor;

TEST(chest_constants_match_srd) {
    CHECK_EQ(kTreasureClassNone, 0);
    CHECK_EQ(kTreasureClassBox, 2);
    CHECK_EQ(kTreasureClassChest, 3);
    CHECK_EQ(kLairTreasureBonus, 25);
    CHECK_EQ(kTrapBitCount, 12);
    CHECK_EQ(kTrapDrawAttempts, 200);
    CHECK_EQ(kTrapChanceByClass[1], 25);
    CHECK_EQ(kTrapChanceByClass[2], 75);
    CHECK_EQ(kTrapChanceByClass[3], 90);
    CHECK_EQ(kLockSquareBitExtinguisher, 10);
    CHECK_EQ(kLockSquareBitAntimagic, 18);
    CHECK(kLockedGoldFactor == 1.5);
    CHECK(kClassGoldFactor[0] == 0.5);
    CHECK(kClassGoldFactor[3] == 1.5);
    CHECK_EQ(kLowGroupGoldLimit, 500);
    CHECK_EQ(kLowGroupGoldBase, 2);
    CHECK_EQ(kItemRollClassMod[1], 15);
    CHECK_EQ(kItemRollClassMod[3], -25);
    CHECK_EQ(kItemRollCap, 90);
    CHECK_EQ(kItemRollCapExempt, 100);
    CHECK_EQ(kItemFurtherDivisor, 10);
    CHECK_EQ(kChestDisarmMin, 5);
    CHECK_EQ(kChestDisarmMax, 98);
}

TEST(victory_gold_only_without_chest_and_chest_gold_needs_trap_resolved) {
    CHECK_EQ(victoryGoldPayout(500, true), 0);
    CHECK_EQ(victoryGoldPayout(500, false), 500);
    // chestGoldPayable(opened, openerKilledByTrap): gold is paid only when opened and the
    // opener was not killed by the trap (R17, R24, Q7).
    CHECK(!chestGoldPayable(false, true));
    CHECK(!chestGoldPayable(true, true));   // opener killed by trap: no gold
    CHECK(chestGoldPayable(true, false));   // opened, trap resolved, opener alive
}

TEST(class_gold_factor_by_treasure_class) {
    // Currency is 1/10000 gold; inputs and expectations are whole gold via currencyFromWhole.
    CHECK_EQ(scaleGoldForClass(currencyFromWhole(1000), 0), currencyFromWhole(500));
    CHECK_EQ(scaleGoldForClass(currencyFromWhole(1000), 1), currencyFromWhole(500));
    CHECK_EQ(scaleGoldForClass(currencyFromWhole(1000), 2), currencyFromWhole(1000));
    CHECK_EQ(scaleGoldForClass(currencyFromWhole(1000), 3), currencyFromWhole(1500));
}

TEST(lock_and_standing_gold_multipliers) {
    CHECK_EQ(scaleGoldForLock(1000), 1500);
    CHECK_EQ(scaleGoldByStanding(1000, 1), 500);
    CHECK_EQ(scaleGoldByStanding(1000, 3), 1500);
    CHECK_EQ(scaleGoldByStanding(1000, 4), 2000);
}

TEST(low_group_gold_is_replaced_from_2_to_501) {
    // Group gold is compared in whole gold (Q6); inputs and results are Currency.
    tsup::Env mid(std::initializer_list<double>{0.5});
    CHECK_EQ(applyLowGroupGoldFloor(mid.ctx, currencyFromWhole(100)), currencyFromWhole(252));  // Int(0.5*500)+2
    tsup::Env top(std::initializer_list<double>{0.999});
    CHECK_EQ(applyLowGroupGoldFloor(top.ctx, currencyFromWhole(499)), currencyFromWhole(501));  // Int(499.5)+2
    tsup::Env keep(std::initializer_list<double>{0.0});
    CHECK_EQ(applyLowGroupGoldFloor(keep.ctx, currencyFromWhole(500)), currencyFromWhole(500)); // 500 is not below 500
}

TEST(item_band_for_each_treasure_class_at_level_10) {
    tsup::Env env(std::initializer_list<double>{0.5});
    ChestDropBand box = chestDropBand(env.ctx, 10, 0, 2);
    CHECK_EQ(box.top, 10);
    CHECK_EQ(box.low, 5);
    ChestDropBand small = chestDropBand(env.ctx, 10, 0, 1);
    CHECK_EQ(small.top, 9);
    CHECK_EQ(small.low, 4);
    ChestDropBand chest = chestDropBand(env.ctx, 10, 0, 3);
    CHECK_EQ(chest.top, 10);
    CHECK_EQ(chest.low, 6);
}

TEST(item_band_clamps_low_levels_to_one) {
    tsup::Env env(std::initializer_list<double>{0.5});
    ChestDropBand b = chestDropBand(env.ctx, 1, 0, 2);   // round_half_even(0.5)=0, clamped to 1
    CHECK_EQ(b.top, 1);
    CHECK_EQ(b.low, 1);
    ChestDropBand c1 = chestDropBand(env.ctx, 1, 0, 1);  // top 0 and low -1 both clamp to 1
    CHECK_EQ(c1.top, 1);
    CHECK_EQ(c1.low, 1);
}

TEST(box_chance3_override_sets_low_one_below_top) {
    tsup::Env hit(std::initializer_list<double>{0.5});                // rng(100) = 50
    ChestDropBand over = chestDropBand(hit.ctx, 10, 51, 2);
    CHECK_EQ(over.low, 9);
    tsup::Env miss(std::initializer_list<double>{0.5});
    ChestDropBand plain = chestDropBand(miss.ctx, 10, 50, 2);  // 50 < 50 is false
    CHECK_EQ(plain.low, 5);
}

TEST(chest_disarm_clamped_to_5_to_98) {
    CHECK_EQ(clampChestDisarm(0), 5);
    CHECK_EQ(clampChestDisarm(5), 5);
    CHECK_EQ(clampChestDisarm(50), 50);
    CHECK_EQ(clampChestDisarm(98), 98);
    CHECK_EQ(clampChestDisarm(99), 98);
}

TEST(fire_damage_reduced_by_half_resistance_then_rounded_half_even) {
    CHECK_EQ(chestFireDamage(10, 0), 10);
    CHECK_EQ(chestFireDamage(10, 100), 5);   // 10 - 10*0.5
    CHECK_EQ(chestFireDamage(10, 50), 8);    // 10 - 2.5 = 7.5 rounds to 8
}

TEST(payout_stopped_by_party_kill_or_teleport_only) {
    ChestTrapResult none{};
    CHECK(!trapStopsPayout(none));
    ChestTrapResult poisoned{};
    poisoned.poisoned = true;
    CHECK(!trapStopsPayout(poisoned));
    ChestTrapResult killed{};
    killed.partyKilled = true;
    CHECK(trapStopsPayout(killed));
    ChestTrapResult teleported{};
    teleported.teleported = true;
    CHECK(trapStopsPayout(teleported));
}

TEST(group_gold_share_rounds_half_even) {
    // Pot is Currency; the share is rounded half-to-even in whole gold (R16, T8).
    CHECK_EQ(goldSharePaid(currencyFromWhole(5), 2), currencyFromWhole(2));    // 2.5 -> 2
    CHECK_EQ(goldSharePaid(currencyFromWhole(7), 2), currencyFromWhole(4));    // 3.5 -> 4
    CHECK_EQ(goldSharePaid(currencyFromWhole(9), 2), currencyFromWhole(4));    // 4.5 -> 4
    CHECK_EQ(goldSharePaid(currencyFromWhole(10), 4), currencyFromWhole(2));   // 2.5 -> 2
    CHECK_EQ(goldSharePaid(currencyFromWhole(12), 4), currencyFromWhole(3));   // exact
}

TEST(typed_gold_uses_clng_and_thief_take_caps_at_purse) {
    CHECK_EQ(typedGoldWhole(2.5), 2);
    CHECK_EQ(typedGoldWhole(3.5), 4);
    CHECK_EQ(typedGoldWhole(-2.5), 0);   // below 1 (negatives included) is 0: reference GiveCommand.GoldAnswer
    CHECK_EQ(typedGoldWhole(0.4), 0);
    CHECK_EQ(thiefTake(currencyFromWhole(100), currencyFromWhole(300)), currencyFromWhole(100));  // roll below purse
    // Roll above purse takes CLng(purse) in whole gold (R27, T7); purse must be a real Currency amount.
    CHECK_EQ(thiefTake(currencyFromWhole(500), currencyFromWhole(300)), currencyFromWhole(300));
}

TEST(store_buy_price_unchanged_on_floor0_and_zero_without_stock) {
    CHECK_EQ(storeBuyPrice(0, 5, 1000, 3), 1000);
    CHECK_EQ(storeBuyPrice(0, 0, 1000, 3), 1000);
    CHECK_EQ(storeBuyPrice(1, 0, 1000, 0), 0);
}

TEST(store_buy_price_shelf_term_caps_at_nine) {
    // shelf = min(count-1, 9); cost = 1.5*P / (ln(shelf+1)+1.15)
    CHECK_EQ(storeBuyPrice(1, 1, 1000, 0), 1304);   // 1500/1.15 = 1304.35
    CHECK_EQ(storeBuyPrice(1, 10, 1000, 0), 434);   // shelf 9: 1500/3.4526 = 434.46
    CHECK_EQ(storeBuyPrice(1, 11, 1000, 0), 434);   // capped at shelf 9
    CHECK(storeBuyPrice(1, 2, 1000, 0) > 434);      // shelf 1 is pricier
}

TEST(store_buy_price_charge_penalty) {
    // 1304.35 - ln(2)*(0.05*1000) = 1304.35 - 34.66 = 1269.69 -> 1270
    CHECK_EQ(storeBuyPrice(1, 1, 1000, 1), 1270);
}

TEST(buying_depletes_shelf_for_floor_or_aligned_purchase) {
    CHECK(!buyDepletesShelf(0, ItemAlign::Unaligned));
    CHECK(buyDepletesShelf(1, ItemAlign::Unaligned));
    CHECK(buyDepletesShelf(0, ItemAlign::Good));
}

TEST(selling_shelves_item_unless_cursed_or_shelf_full) {
    Item plain{};
    CHECK(!shelvesSaleItem(plain, 0, ItemAlign::Unaligned, 0));
    CHECK(shelvesSaleItem(plain, 0, ItemAlign::Good, 0));
    CHECK(shelvesSaleItem(plain, 1, ItemAlign::Unaligned, 8));
    CHECK(!shelvesSaleItem(plain, 1, ItemAlign::Unaligned, 9));
    Item cursed{};
    cursed.cursed = CurseKind1;
    CHECK(!shelvesSaleItem(cursed, 1, ItemAlign::Good, 0));
}

TEST(hall_record_needs_strictly_greater_purse_and_restock_needs_days) {
    CHECK(replacesWealthiestRecord(101, 100));
    CHECK(!replacesWealthiestRecord(100, 100));
    CHECK(!replacesWealthiestRecord(99, 100));
    Character leader{};
    CHECK(!restocksOnArrival(leader));   // daysAged 0
    leader.daysAged = 1;
    CHECK(restocksOnArrival(leader));
}

TEST(square_blocks_magic_lock_on_antimagic_or_extinguisher_bits) {
    CHECK(squareAllowsMagicLock(0u));
    CHECK(squareAllowsMagicLock(1u << 11));
    CHECK(!squareAllowsMagicLock(1u << 10));   // EXTINGUISHER
    CHECK(!squareAllowsMagicLock(1u << 18));   // ANTIMAGIC
}

TEST(magic_lock_needs_class_above_one_and_allowed_square) {
    tsup::Env e1(std::initializer_list<double>{0.0});
    CHECK(applyMagicLock(e1.ctx, 0u, 2, 100) == ChestLock::MAGIC_LOCKED);
    tsup::Env e2(std::initializer_list<double>{0.0});
    CHECK(applyMagicLock(e2.ctx, 0u, 1, 100) == ChestLock::UNLOCKED);
    tsup::Env e3(std::initializer_list<double>{0.0});
    CHECK(applyMagicLock(e3.ctx, 1u << 18, 2, 100) == ChestLock::UNLOCKED);
    tsup::Env e4(std::initializer_list<double>{0.0});
    CHECK(applyMagicLock(e4.ctx, 1u << 10, 3, 100) == ChestLock::UNLOCKED);
}

TEST(lock_roll_passes_at_or_below_locked_chance_for_class_two_up) {
    tsup::Env pass(std::initializer_list<double>{0.29});                  // Int(29)+1 = 30 <= 30
    CHECK(rollChestLock(pass.ctx, 2, 30));
    tsup::Env fail(std::initializer_list<double>{0.30});                  // Int(30)+1 = 31 > 30
    CHECK(!rollChestLock(fail.ctx, 2, 30));
    tsup::Env cls1(std::initializer_list<double>{0.0});
    CHECK(!rollChestLock(cls1.ctx, 1, 100)); // class 1 never locks
}

TEST(extra_item_roll_uses_item_chance_tenth) {
    tsup::Env yes(std::initializer_list<double>{0.04});                   // Int(4)+1 = 5 <= Int(50/10)=5
    CHECK(rollAnotherChestItem(yes.ctx, 50));
    tsup::Env no(std::initializer_list<double>{0.05});                    // Int(5)+1 = 6 > 5
    CHECK(!rollAnotherChestItem(no.ctx, 50));
    tsup::Env small(std::initializer_list<double>{0.0});                  // Int(9/10)=0, roll 1 > 0
    CHECK(!rollAnotherChestItem(small.ctx, 9));
}

TEST(item_spawn_uses_class_modifier_and_cap) {
    // MAGIC_LOCKED avoids the charm-only lock bonus, which the sources disagree on.
    tsup::Env a(std::initializer_list<double>{0.49});                     // roll 50 <= 50
    CHECK(rollChestItemSpawn(a.ctx, 0, 50, ChestLock::MAGIC_LOCKED, 10));
    tsup::Env b(std::initializer_list<double>{0.50});                     // roll 51 > 50
    CHECK(!rollChestItemSpawn(b.ctx, 0, 50, ChestLock::MAGIC_LOCKED, 10));
    tsup::Env c1(std::initializer_list<double>{0.36});                    // roll ~51 + 15 class-1 mod > 50
    CHECK(!rollChestItemSpawn(c1.ctx, 1, 50, ChestLock::MAGIC_LOCKED, 10));
    tsup::Env c3(std::initializer_list<double>{0.72});                    // roll ~72 - 25 class-3 mod = ~47 <= 50
    CHECK(rollChestItemSpawn(c3.ctx, 3, 50, ChestLock::MAGIC_LOCKED, 10));
    tsup::Env cap(std::initializer_list<double>{0.99});                   // roll ~100 capped to 90 since ItemChance != 100
    CHECK(rollChestItemSpawn(cap.ctx, 0, 95, ChestLock::MAGIC_LOCKED, 10));
}

TEST(chest_trap_needs_class_flags_and_chance_then_bit_draw) {
    tsup::Env none(std::initializer_list<double>{0.0});
    CHECK(rollChestTrap(none.ctx, 0, 64) == ChestTrap::NotTrapped);     // class 0: no chest
    tsup::Env noFlags(std::initializer_list<double>{0.0});
    CHECK(rollChestTrap(noFlags.ctx, 3, 0) == ChestTrap::NotTrapped);   // no trap flags
    tsup::Env miss(std::initializer_list<double>{0.80});                  // Int(80)+1 = 81 >= 75
    CHECK(rollChestTrap(miss.ctx, 2, 64) == ChestTrap::NotTrapped);
    tsup::Env hit(std::initializer_list<double>{0.6, 0.5});               // 61 < 75, then Int(0.5*12)=6, bit 6 set
    CHECK(rollChestTrap(hit.ctx, 2, 64) == ChestTrap::Fate);
}
