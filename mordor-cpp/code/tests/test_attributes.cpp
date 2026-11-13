#include "test_support.h"
#include "mordor/attributes.h"

using namespace mordor;

// Builds a stat array in index order STR, INT, WIS, CON, CHA, DEX (R1).
static Arr<i32, kStatCount> attr_arr(i32 str, i32 intl, i32 wis, i32 con, i32 cha, i32 dex) {
    Arr<i32, kStatCount> a{};
    a[0] = str;
    a[1] = intl;
    a[2] = wis;
    a[3] = con;
    a[4] = cha;
    a[5] = dex;
    return a;
}

TEST(attr_constants_match_srd) {
    // R11, R2, R5, R19, R22 constants.
    CHECK_EQ(kBaseAD, 10);
    CHECK_EQ(kCreationPoolBase, 30);
    CHECK_EQ(kFreePerStat, 5);
    CHECK_EQ(kStartHits, 15);
    CHECK_EQ(kHitsFloor, 15);
    CHECK_EQ(kStartGoldWhole, 1500);
    CHECK_EQ(kStartArea, -1);
    CHECK_EQ(kConLossBase, 50);
    CHECK_EQ(kConLossPerPoint, 2);
    CHECK_EQ(kAvgHitsDivisor, 5);
}

TEST(attr_stats_ad_band_table) {
    // R14 band table: STR 8..12 w2, DEX 8..12 w2, INT 8..14 w1, WIS 8..14 w1.
    CHECK(kStatsAdBands[0].stat == StatIndex::Str);
    CHECK_EQ(kStatsAdBands[0].lo, 8);
    CHECK_EQ(kStatsAdBands[0].hi, 12);
    CHECK_EQ(kStatsAdBands[0].weight, 2);
    CHECK(kStatsAdBands[1].stat == StatIndex::Dex);
    CHECK_EQ(kStatsAdBands[1].lo, 8);
    CHECK_EQ(kStatsAdBands[1].hi, 12);
    CHECK_EQ(kStatsAdBands[1].weight, 2);
    CHECK(kStatsAdBands[2].stat == StatIndex::Int);
    CHECK_EQ(kStatsAdBands[2].lo, 8);
    CHECK_EQ(kStatsAdBands[2].hi, 14);
    CHECK_EQ(kStatsAdBands[2].weight, 1);
    CHECK(kStatsAdBands[3].stat == StatIndex::Wis);
    CHECK_EQ(kStatsAdBands[3].lo, 8);
    CHECK_EQ(kStatsAdBands[3].hi, 14);
    CHECK_EQ(kStatsAdBands[3].weight, 1);
}

TEST(attr_term_zero_inside_band) {
    // R14 Term: zero at and between the band edges.
    CHECK_EQ(term(8, 8, 12, 2), 0);
    CHECK_EQ(term(10, 8, 12, 2), 0);
    CHECK_EQ(term(12, 8, 12, 2), 0);
    CHECK_EQ(term(14, 8, 14, 1), 0);
}

TEST(attr_term_above_band) {
    // R14 Term: w * (v - hi) above the band.
    CHECK_EQ(term(13, 8, 12, 2), 2);
    CHECK_EQ(term(15, 8, 12, 2), 6);
    CHECK_EQ(term(15, 8, 14, 1), 1);
    CHECK_EQ(term(16, 8, 14, 1), 2);
}

TEST(attr_stats_ad_all_fifteen) {
    // R15 check: all four relevant stats at 15 gives 14.
    // CON and CHA are excluded from Stats A/D (R14), so their values must not matter.
    CHECK_EQ(statsAD(attr_arr(15, 15, 15, 3, 99, 15)), 14);
}

TEST(attr_stats_ad_mixed_values) {
    // STR 13: 2*(13-12)=2. INT 16: 1*(16-14)=2. WIS 8: in band, 0. DEX 12: in band, 0.
    // Total 4. CON and CHA set to 0 to show they are ignored.
    CHECK_EQ(statsAD(attr_arr(13, 16, 8, 0, 0, 12)), 4);
}

TEST(attr_guild_ad_adds_base) {
    // R12: guildAtt = BaseAD + ga, guildDef = BaseAD + gd.
    AdPair best;
    best.att = 3;
    best.def = 2;
    AdPair g = guildAD(best);
    CHECK_EQ(g.att, 13);
    CHECK_EQ(g.def, 12);
}

TEST(attr_guild_ad_zero_rating_is_base) {
    // R11/R12: a zero guild award leaves the base 10/10.
    AdPair none;
    AdPair g = guildAD(none);
    CHECK_EQ(g.att, 10);
    CHECK_EQ(g.def, 10);
}

TEST(attr_fit_matched_and_unaligned_full) {
    // R9 table: matched or unaligned gives Full.
    CHECK(fitFor(ItemAlign::Good, Align::Good) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Neutral, Align::Neutral) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Evil, Align::Evil) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Unaligned, Align::Evil) == FitClass::Full);
}

TEST(attr_fit_adjacent_is_half) {
    // R9: Good-Neutral and Neutral-Evil are adjacent.
    CHECK(fitFor(ItemAlign::Good, Align::Neutral) == FitClass::Half);
    CHECK(fitFor(ItemAlign::Neutral, Align::Good) == FitClass::Half);
    CHECK(fitFor(ItemAlign::Neutral, Align::Evil) == FitClass::Half);
    CHECK(fitFor(ItemAlign::Evil, Align::Neutral) == FitClass::Half);
}

TEST(attr_fit_opposed_is_subtract) {
    // R9: Good and Evil are opposed.
    CHECK(fitFor(ItemAlign::Good, Align::Evil) == FitClass::Subtract);
    CHECK(fitFor(ItemAlign::Evil, Align::Good) == FitClass::Subtract);
}

TEST(attr_stat_with_mods_adds_gear_no_floor) {
    // R6: display = natural + StatMods, with no floor. Natural value is not overwritten (R1).
    Character c;
    c.abilities.stats[0] = 10;
    c.abilities.statMods[0] = 3;
    CHECK_EQ(statWithMods(c, StatIndex::Str), 13);
    CHECK_EQ(c.abilities.stats[0], 10);

    c.abilities.stats[2] = 2;
    c.abilities.statMods[2] = -5;
    CHECK_EQ(statWithMods(c, StatIndex::Wis), -3);
}

TEST(attr_stat_index_order_matches_storage) {
    // R1: STR=0, INT=1, WIS=2, CON=3, CHA=4, DEX=5.
    Character c;
    c.abilities.stats[5] = 12;
    CHECK_EQ(statWithMods(c, StatIndex::Dex), 12);
    c.abilities.stats[3] = 19;
    CHECK_EQ(statWithMods(c, StatIndex::Con), 19);
}

TEST(attr_creation_base_is_min_plus_five) {
    // R2 with Human (R23/R24): STR min 4 -> 9, CON min 6 -> 11, CHA min 5 -> 10.
    // Gnome CHA min 9 -> 14 (max 22, so the min(Max, Min+5) cap does not bind).
    tsup::Env env(1);
    CHECK_EQ(createBase(env.ctx, Race::Human, StatIndex::Str), 9);
    CHECK_EQ(createBase(env.ctx, Race::Human, StatIndex::Con), 11);
    CHECK_EQ(createBase(env.ctx, Race::Human, StatIndex::Cha), 10);
    CHECK_EQ(createBase(env.ctx, Race::Gnome, StatIndex::Cha), 14);
}

TEST(attr_creation_pool_relative_to_spent) {
    // R3: lowering one base by 2 raises spent-refund pool by exactly 2.
    // The racial BonusPoints term is unresolved, so compare two pools instead of absolutes.
    tsup::Env env(1);
    Arr<i32, kStatCount> base1 = attr_arr(9, 9, 9, 11, 10, 11);
    Arr<i32, kStatCount> base2 = attr_arr(7, 9, 9, 11, 10, 11);
    i32 p1 = creationPool(env.ctx, Race::Human, base1);
    i32 p2 = creationPool(env.ctx, Race::Human, base2);
    CHECK_EQ(p2 - p1, 2);
}

