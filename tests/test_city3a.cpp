#include "test_support.h"
#include "mordor/city1.h"
#include "mordor/city3a.h"

using namespace mordor;

// Documented constants (module header and SRD quirks).
TEST(city3a_documented_constants) {
    CHECK_EQ(kStoreFloorCap, 14);
    CHECK_EQ(kPenRestockCap, 9);
    CHECK_EQ(kPenStockCap, 9);
    CHECK_EQ(kPenPriceMinBase, 10);
    CHECK_EQ(kStartingCompanionCount, 10 * 2);
    CHECK_EQ(kRealignCrossFactor, 2);
}

// Store cap is one short: a fifteen-floor dungeon never stocks floor 15.
TEST(deepest_stocked_floor_stops_short_of_bottom_floor) {
    CHECK_EQ(deepestStockedFloor(15), 14);
}

// Rounded index bias: index 0 covers only the lower half-band.
TEST(rounded_index_low_draw_gives_zero) {
    tsup::Env env({0.04, 0.9});
    CHECK_EQ(roundedIndex(env.ctx, 10), 0);
}

TEST(rounded_index_band_of_one) {
    // 0.1 * 10 = 1.0 rounds to 1.
    tsup::Env env({0.1, 0.9});
    CHECK_EQ(roundedIndex(env.ctx, 10), 1);
}

// Index n matches nothing and is retried.
TEST(rounded_index_retries_when_index_equals_n) {
    // 0.96 * 10 = 9.6 rounds to 10 == n, so it is retried.
    // 0.21 * 10 = 2.1 rounds to 2.
    tsup::Env env({0.96, 0.21});
    CHECK_EQ(roundedIndex(env.ctx, 10), 2);
}

// Empty pen gives price 0 (original Log(0) trap reproduced).
TEST(pen_buy_price_empty_pen_is_zero) {
    CHECK_EQ(penBuyPrice(100, 0), 0);
    CHECK_EQ(penBuyPrice(500, 0), 0);
}

// Base value is floored at 10 before pricing.
TEST(pen_buy_price_base_floored_at_ten) {
    CHECK_EQ(penBuyPrice(3, 1), penBuyPrice(10, 1));
    CHECK_EQ(penBuyPrice(0, 4), penBuyPrice(10, 4));
}

// Stock is pen count less one, capped at 9, so pens of 10 and up price the same.
TEST(pen_buy_price_stock_capped_at_nine) {
    CHECK_EQ(penBuyPrice(100, 10), penBuyPrice(100, 11));
    CHECK_EQ(penBuyPrice(100, 10), penBuyPrice(100, 40));
}

// More penned companions means a larger ln term and a lower price.
TEST(pen_buy_price_falls_as_pen_grows) {
    CHECK(penBuyPrice(100, 2) < penBuyPrice(100, 1));
    CHECK(penBuyPrice(100, 5) < penBuyPrice(100, 2));
}

// Buy handler guard: only a pen with at least one companion can sell.
TEST(pen_buy_guard_requires_positive_count) {
    CHECK(penBuyGuard(1));
    CHECK(!penBuyGuard(0));
    CHECK(!penBuyGuard(-1));
}

// Companion sell Take has no floor check; a pen can go to -1.
TEST(pen_take_one_has_no_floor) {
    i32 pen = 0;
    penTakeOne(pen);
    CHECK_EQ(pen, -1);

    i32 other = 5;
    penTakeOne(other);
    CHECK_EQ(other, 4);
}

TEST(pen_add_allows_low_count) {
    CHECK(penAddWithinCap(0));
}

// Zero-charge combining: kinds 23-28, 30, 31 allowed; 29 is not in the list.
TEST(zero_charge_combine_kinds) {
    CHECK(storeAllowsZeroChargeCombine(23));
    CHECK(storeAllowsZeroChargeCombine(26));
    CHECK(storeAllowsZeroChargeCombine(28));
    CHECK(storeAllowsZeroChargeCombine(30));
    CHECK(storeAllowsZeroChargeCombine(31));
    CHECK(!storeAllowsZeroChargeCombine(29));
    CHECK(!storeAllowsZeroChargeCombine(22));
    CHECK(!storeAllowsZeroChargeCombine(32));
}

// A lit row hidden by a mod clears the choice; with no mod, nothing clears.
TEST(store_choice_clears_only_when_row_hidden_by_mod) {
    CHECK(storeChoiceClears(true));
    CHECK(!storeChoiceClears(false));
}

// Companion number entry: index = round_half_even(typed - 1), zero-based.
TEST(companion_index_from_typed_basic) {
    CHECK_EQ(companionIndexFromTyped(1.0), 0);
    CHECK_EQ(companionIndexFromTyped(3.0), 2);
}

// 2.5 - 1 = 1.5 rounds to 2 under half-to-even (and half-up).
TEST(companion_index_half_rounds_to_even) {
    CHECK_EQ(companionIndexFromTyped(2.5), 2);
}

// 1.5 - 1 = 0.5 rounds to 0 under half-to-even (half-up would give 1).
TEST(companion_index_half_to_even_low_tie) {
    CHECK_EQ(companionIndexFromTyped(1.5), 0);
}

// Gold CLng: halves round to even; &HFFFF reads as a 16-bit signed Integer.
TEST(clng_from_entry_rounds_halves_to_even) {
    Str two = "2.5";
    Str four = "3.5";
    CHECK_EQ(clngFromEntry(two), 2);
    CHECK_EQ(clngFromEntry(four), 4);
}

TEST(clng_from_entry_hex_reads_as_signed_integer) {
    Str hex = "&HFFFF";
    CHECK_EQ(clngFromEntry(hex), -1);
}

// The Seer fee is charged even when the search fails.
TEST(seer_fee_charged_even_when_search_fails) {
    CHECK(seerFeeCharged(false));
    CHECK(seerFeeCharged(true));
}

// Lost reading writes "?"; a found reading from a live square writes the square.
TEST(sighting_place_lost_is_unknown) {
    CHECK(sightingPlace(true, 2) == SightingPlaceKind::Unknown);
}

TEST(sighting_place_found_uses_live_square) {
    CHECK(sightingPlace(false, 2) == SightingPlaceKind::Square);
}

// Invisible creatures are skipped only when the reader is in the dungeon.
TEST(sighting_skips_invisible_only_in_dungeon) {
    CHECK(sightingSkipsInvisible(true, 1));
    CHECK(!sightingSkipsInvisible(true, 0));
    CHECK(!sightingSkipsInvisible(false, 5));
}

// Known is only ever raised by a sighting.
TEST(sighting_known_only_raised) {
    CHECK_EQ(sightingKnownAfter(1, 3), 3);
    CHECK_EQ(sightingKnownAfter(3, 1), 3);
    CHECK_EQ(sightingKnownAfter(2, 2), 2);
}

// Merge replaces the name only when the existing record has Known == 0.
TEST(merge_replaces_name_only_when_unknown) {
    CHECK(mergeReplacesName(0));
    CHECK(!mergeReplacesName(1));
    CHECK(!mergeReplacesName(3));
}

// Partial item reads below Mostly keep grade 0; Mostly and All keep their grade.
TEST(library_item_grade_below_mostly_is_zero) {
    CHECK_EQ(libraryItemGrade(Unknown), 0);
    CHECK_EQ(libraryItemGrade(Barely), 0);
    CHECK_EQ(libraryItemGrade(Partial), 0);
}

TEST(library_item_grade_mostly_and_all) {
    CHECK_EQ(libraryItemGrade(Mostly), 3);
    CHECK_EQ(libraryItemGrade(All), 4);
}

// Damage modifier shown only for weapons, and only at grade 4.
TEST(damage_modifier_weapon_at_grade_four_only) {
    CHECK(showsDamageModifier(true, 4));
    CHECK(!showsDamageModifier(true, 3));
    CHECK(!showsDamageModifier(false, 4));
}

// Coverage from other members takes exactly the shortfall from their bank accounts.
TEST(cover_shortfall_takes_from_giver_bank_only) {
    Party p{};
    p.size = 2;
    p.members[0].wallet.goldInBank = 0;
    p.members[1].wallet.goldInBank = 500;
    p.members[1].wallet.goldOnHand = 1000;
    (void)coverShortfallFromBanks(p, 0, 300);
    CHECK_EQ(p.members[1].wallet.goldInBank, 200);
    CHECK_EQ(p.members[1].wallet.goldOnHand, 1000);
    CHECK_EQ(p.members[0].wallet.goldInBank, 0);
}
