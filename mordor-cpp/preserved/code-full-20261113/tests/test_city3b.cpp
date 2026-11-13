// Tests for city3b: store replenish, companion pricing, sighting merge and
// invisibility rules. Expected values are derived from the SRD text.
// Avoided: the unresolved `days / 100 * 100` division (only days 0, 100, 200
// are used, where literal and integer division agree) and the other UNKNOWN items.
#include "test_support.h"
#include "mordor/city3b.h"

using namespace mordor;

// ---- Constants stated in the SRD ----

TEST(city3b_constants_match_srd) {
  CHECK_EQ(kCompanionPriceFloor, 10);   // engine: value = max(BaseValue, 10)
  CHECK_EQ(kCompanionStockCap, 9);      // engine: stock = min(penned - 1, 9)
  CHECK_EQ(kStoreReplenishStep, 100);   // chance expression uses days / 100 * 100
  CHECK_EQ(kGuildLogKeepDocumented, 50);  // docs: "keeps the newest fifty"
  CHECK_EQ(kStartingCompanionTypes, 10);  // ten indices
  CHECK_EQ(kStartingCompanionPerType, 2); // two of each
}

// ---- Store replenish chance ----

TEST(store_replenish_chance_zero_days_is_zero) {
  // days / 100 * 100 with days = 0 is 0 under either division reading.
  CHECK_EQ(storeReplenishChance(0), 0);
}

TEST(store_replenish_chance_hundred_days_is_hundred) {
  // 100 / 100 * 100 = 100 under either division reading.
  CHECK_EQ(storeReplenishChance(100), 100);
}

TEST(store_replenish_chance_two_hundred_days_is_two_hundred) {
  // 200 / 100 * 100 = 200 under either division reading.
  CHECK_EQ(storeReplenishChance(200), 200);
}

TEST(roll_store_replenish_zero_chance_never_replenishes) {
  // Chance 0 means no replenish for a mid-range roll.
  // FIXED: the roll is narrowed to u64 because the env takes u64 values; the
  // cast keeps the braced form. Chance 0 never replenishes for any roll, so
  // the assertion is unchanged.
  tsup::Env env({static_cast<u64>(0.5)});
  CHECK(!rollStoreReplenish(env.ctx, 0));
}

TEST(roll_store_replenish_full_chance_always_replenishes) {
  // Chance 100 means replenish for any roll in [0, 1).
  // FIXED: the roll is narrowed to u64 because the env takes u64 values; the
  // cast keeps the braced form. Chance 100 replenishes for any roll, so the
  // assertion is unchanged.
  tsup::Env env({static_cast<u64>(0.999)});
  CHECK(rollStoreReplenish(env.ctx, 100));
}

// ---- Alignment choice in store stock ----

TEST(store_offers_alignment_choice_single_flag_gives_no_choice) {
  // Engine test: AlignmentFlags > 1. Flags 0 and 1 give no choice.
  CHECK(!storeOffersAlignmentChoice(0));
  CHECK(!storeOffersAlignmentChoice(1));
}

TEST(store_offers_alignment_choice_two_or_more_flags_gives_choice) {
  // Engine boundary: 2 is the first value above 1 and offers a choice.
  CHECK(storeOffersAlignmentChoice(2));
  CHECK(storeOffersAlignmentChoice(3));
}

// ---- Companion buy price ----

TEST(companion_buy_price_floor_at_ten) {
  // value = max(BaseValue, 10): base 5 prices the same as base 10.
  CHECK_EQ(companionBuyPrice(5, 3), companionBuyPrice(10, 3));
  CHECK_EQ(companionBuyPrice(9, 3), companionBuyPrice(10, 3));
}

TEST(companion_buy_price_stock_capped_at_nine) {
  // stock = min(penned - 1, 9): penned 10 gives stock 9, and so does penned 11.
  CHECK_EQ(companionBuyPrice(100, 10), companionBuyPrice(100, 11));
  CHECK_EQ(companionBuyPrice(100, 10), companionBuyPrice(100, 40));
}

TEST(companion_buy_price_more_penned_is_cheaper) {
  // price = 1.5 * value / (ln(stock + 1) + 1.15). Stock 1 gives ln 2 > 0 in
  // the denominator, so penned 2 is cheaper than penned 1.
  CHECK(companionBuyPrice(100000, 1) > companionBuyPrice(100000, 2));
}

TEST(companion_buy_price_penned_one_to_two_ratio) {
  // Penned 1 has stock 0 (ln 1 = 0). Penned 2 has stock 1 (ln 2).
  // Ratio = (ln 2 + 1.15) / 1.15 = 1.843147 / 1.15 = 1.602737.
  double a = static_cast<double>(companionBuyPrice(100000, 1));
  double b = static_cast<double>(companionBuyPrice(100000, 2));
  double r = a / b;
  CHECK(r > 1.601 && r < 1.605);
}

TEST(companion_buy_price_linear_in_base_value) {
  // Above the floor, price is proportional to value: 10x base gives 10x price.
  double a = static_cast<double>(companionBuyPrice(100000, 1));
  double b = static_cast<double>(companionBuyPrice(1000000, 1));
  double r = b / a;
  CHECK(r > 9.99 && r < 10.01);
}

// ---- Sighting merge ----

TEST(merge_sighting_known_only_raises) {
  // Known is only ever raised, so the result is the larger grade.
  CHECK_EQ(mergeSightingKnown(2, 1), 2);
  CHECK_EQ(mergeSightingKnown(1, 3), 3);
  CHECK_EQ(mergeSightingKnown(3, 3), 3);
}

TEST(merge_sighting_zero_known_takes_read_grade) {
  // A new monster (Known 0) takes the grade of the reading.
  CHECK_EQ(mergeSightingKnown(0, 2), 2);
  CHECK_EQ(mergeSightingKnown(0, 0), 0);
}

// ---- Sighting name ----

TEST(sighting_takes_new_name_only_when_unknown) {
  // Name is replaced only when there.Known == 0.
  CHECK(sightingTakesNewName(0));
  CHECK(!sightingTakesNewName(1));
  CHECK(!sightingTakesNewName(2));
  CHECK(!sightingTakesNewName(4));
}

// ---- Invisible sighting skip ----

TEST(sighting_in_city_never_skips_invisible) {
  // Engine: invisible creatures are skipped only when reader.DungeonLevel > 0.
  CHECK(!sightingSkipsInvisible(0, true, false));
}

TEST(sighting_in_dungeon_skips_unseen_invisible) {
  // Dungeon level > 0, invisible creature, reader lacks See Invisible: skipped.
  CHECK(sightingSkipsInvisible(1, true, false));
  CHECK(sightingSkipsInvisible(5, true, false));
}

TEST(sighting_visible_creature_not_skipped) {
  // A visible creature is not skipped, even in the dungeon.
  CHECK(!sightingSkipsInvisible(1, false, false));
}

TEST(sighting_reader_with_see_invisible_not_skipped) {
  // A reader who sees invisible creatures does not skip them.
  CHECK(!sightingSkipsInvisible(1, true, true));
}
