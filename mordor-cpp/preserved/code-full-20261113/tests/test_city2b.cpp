// Tests for city2b: ratio, rest days, shelf/pen caps, vault slots, library
// day phrasing, companion price bands, and the constant table values.
// Expected values are derived from the SRD text in the module notes.
#include "test_support.h"
#include "mordor/city2b.h"

#include <cmath>
#include <cstring>

using namespace mordor;

// R74: ratio_raw = Price(Giant Spider) / Price(Kobold) = 109163 / 49216 ~ 2.22
TEST(ratio_raw_giant_spider_over_kobold) {
    double r = ratioRaw(109163, 49216);
    // 109163 / 49216 = 2.21804...
    CHECK(std::fabs(r - 2.218) < 0.001);
    CHECK(std::fabs(r - 2.22) < 0.01);
}

// R3 / T-K: days = round_half_even(100 / 3.25) with 0 hits left -> 31 days.
TEST(rest_days_zero_hits_is_31) {
    CHECK_EQ(cityRestDays(0, 15), 31);
}

// T-K: a tenth left -> missing 90 percent -> round_half_even(90 / 3.25) = 28.
// 90 / 3.25 = 27.69..., rounds to 28.
TEST(rest_days_tenth_left_is_28) {
    CHECK_EQ(cityRestDays(1, 10), 28);
}

// Full hits: nothing missing, so no rest days.
TEST(rest_days_full_hits_is_zero) {
    CHECK_EQ(cityRestDays(15, 15), 0);
}

// T-K rest constants as documented.
TEST(rest_constants_match_srd) {
    CHECK(kRestNumerator == 100);
    CHECK(kRestDivisor == 3.25);
}

// T-J / R16 / R18: shelf cap 9, deepest stocked 14 = floor limit 15 - 1.
TEST(shelf_and_floor_constants) {
    CHECK_EQ(kShelfCap, 9);
    CHECK_EQ(kFloorLimit, 15);
    CHECK_EQ(kDeepestStocked, 14);
    CHECK_EQ(kDeepestStocked, kFloorLimit - 1);
}

// T-J: deepest stocked is 14, so a deeper reach is capped at 14.
TEST(capped_shelf_depth_caps_at_14) {
    CHECK_EQ(cappedShelfDepth(15), 14);
    CHECK_EQ(cappedShelfDepth(14), 14);
    CHECK_EQ(cappedShelfDepth(3), 3);
}

// T-J: pen shelf cap is 9 per pen. Adding to 8 reaches 9; adding at 9 is refused.
TEST(pen_add_one_respects_shelf_cap) {
    i32 n = 8;
    CHECK(penAddOne(n) == true);
    CHECK_EQ(n, 9);

    i32 full = 9;
    CHECK(penAddOne(full) == false);
    CHECK_EQ(full, 9);
}

// T-I: vault slots 1..39 are accepted; slot <= 0 and slot >= 40 are refused.
TEST(vault_slot_bounds) {
    CHECK(vaultSlotAccepted(0) == false);
    CHECK(vaultSlotAccepted(-1) == false);
    CHECK(vaultSlotAccepted(1) == true);
    CHECK(vaultSlotAccepted(39) == true);
    CHECK(vaultSlotAccepted(40) == false);
    CHECK(vaultSlotAccepted(41) == false);
}

// T-H: today == 0 means "it is not known how many days ago", whatever days is.
// This check comes first, so it wins even when days == 0.
TEST(library_day_phrase_unknown_when_today_zero) {
    CHECK(libraryDayPhrase(0, 5) == LibraryDayPhrase::NotKnown);
    CHECK(libraryDayPhrase(0, 0) == LibraryDayPhrase::NotKnown);
}

// T-H: days == 0 -> "just Today."
TEST(library_day_phrase_today) {
    CHECK(libraryDayPhrase(10, 0) == LibraryDayPhrase::Today);
}

// T-H: days == 1 -> "just 1 day ago."
TEST(library_day_phrase_one_day) {
    CHECK(libraryDayPhrase(10, 1) == LibraryDayPhrase::OneDay);
}

// T-H: any other count -> "{days} days ago."
TEST(library_day_phrase_plural_days) {
    CHECK(libraryDayPhrase(10, 7) == LibraryDayPhrase::Days);
    CHECK(libraryDayPhrase(10, 2) == LibraryDayPhrase::Days);
}

// libraryDaysAgo is taken as today minus seen serial.
TEST(library_days_ago_is_difference) {
    CHECK_EQ(libraryDaysAgo(5, 12), 7);
    CHECK_EQ(libraryDaysAgo(12, 12), 0);
}

// T-M: item scheme Good=1, Neutral=2, Evil=3 maps to pen index 0, 1, 2.
// Index 0 in the pen is Good; the schemes differ by one.
TEST(pen_align_index_maps_item_scheme) {
    CHECK_EQ(penAlignIndex(ItemAlign::Good), 0);
    CHECK_EQ(penAlignIndex(ItemAlign::Neutral), 1);
    CHECK_EQ(penAlignIndex(ItemAlign::Evil), 2);
}

// T-R: level 1 band runs from 1,000 (Slave) to 714,870 (Aboleth).
TEST(companion_band_level_1) {
    CompanionPriceBand b = companionPriceBand(1);
    CHECK_EQ(b.level, 1);
    CHECK_EQ(b.lowest, 1000);
    CHECK_EQ(b.highest, 714870);
    CHECK(std::strcmp(b.lowestName, "Slave") == 0);
    CHECK(std::strcmp(b.highestName, "Aboleth") == 0);
}

// T-R: level 16 band runs from 2,855,101 (T'Hota) to 15,511,619 (Asmodeus).
TEST(companion_band_level_16) {
    CompanionPriceBand b = companionPriceBand(16);
    CHECK_EQ(b.level, 16);
    CHECK_EQ(b.lowest, 2855101);
    CHECK_EQ(b.highest, 15511619);
    CHECK(std::strcmp(b.lowestName, "T'Hota") == 0);
    CHECK(std::strcmp(b.highestName, "Asmodeus") == 0);
}

// T-R: level 20 is the only entry, The Void at 99,999,999 for both ends.
TEST(companion_band_level_20_is_the_void) {
    CompanionPriceBand b = companionPriceBand(20);
    CHECK_EQ(b.lowest, 99999999);
    CHECK_EQ(b.highest, 99999999);
    CHECK(std::strcmp(b.lowestName, "The Void") == 0);
    CHECK(std::strcmp(b.highestName, "The Void") == 0);
}

// T-R table: entries are numbered 1..20 in order, and lowest never exceeds highest.
TEST(companion_price_table_shape) {
    for (i32 i = 0; i < 20; ++i) {
        CHECK_EQ(kCompanionPriceBands[i].level, i + 1);
        CHECK(kCompanionPriceBands[i].lowest <= kCompanionPriceBands[i].highest);
    }
}

// Buy and realign coefficients from T-K.
TEST(price_coefficients_match_srd) {
    CHECK(kBuyFactor == 1.5);
    CHECK(kBuyDivisor == 1.15);
    CHECK(kRealignFactor == 0.72);
}

// Gold input is held in 1/10000 units: 1.0 gold -> 10000, 2.5 gold -> 25000.
TEST(gold_input_to_currency_units) {
    CHECK_EQ(goldInputToCurrency(1.0), 10000);
    CHECK_EQ(goldInputToCurrency(2.5), 25000);
}
