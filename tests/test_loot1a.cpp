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
