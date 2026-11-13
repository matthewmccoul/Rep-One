#include <cmath>
#include <string>

#include "test_support.h"
#include "mordor/misc.h"

using namespace mordor;

namespace {

bool hasText(const Str& s, const std::string& needle) {
    return std::string(s).find(needle) != std::string::npos;
}

bool strIs(const Str& s, const std::string& want) {
    return std::string(s) == want;
}

}  // namespace

// ---------------------------------------------------------------- Asmodeus (R1, R3, Asmodeus log line)

TEST(asmodeus_alone_wording_uses_single_handed_phrase) {
    Str name = "Mager";
    Str line = asmodeusLogLine(name, false);
    CHECK(hasText(line, "single handedly defeated Asmodeus"));
    CHECK(hasText(line, "Mager"));
    CHECK(!hasText(line, "was part of the party"));
}

TEST(asmodeus_party_wording_uses_party_phrase) {
    Str name = "Theshal";
    Str line = asmodeusLogLine(name, true);
    CHECK(hasText(line, "was part of the party that defeated"));
    CHECK(hasText(line, "Theshal"));
    CHECK(!hasText(line, "single handedly"));
}

TEST(asmodeus_log_line_depends_on_in_party_flag) {
    Str name = "Orgal";
    Str withParty = asmodeusLogLine(name, true);
    Str alone = asmodeusLogLine(name, false);
    CHECK(!strIs(withParty, std::string(alone)));
}

// ---------------------------------------------------------------- Teleporter trap (R6-R8, wall flags)

TEST(teleport_constants_match_srd) {
    CHECK_EQ(kTeleportMaxTries, 10000);
    CHECK_EQ(kTeleportAxisMin, 1);
    CHECK_EQ(kTeleportAxisMax, 30);
    CHECK_EQ(kWallBitRock, 19);
    CHECK_EQ(kWallBitTeleporter, 14);
    CHECK_EQ(kWallMaskRock, u32{1} << 19);
    CHECK_EQ(kWallMaskTeleporter, u32{1} << 14);
}

TEST(teleport_square_rejects_rock_and_teleporter_walls) {
    CHECK(isValidTeleportSquare(0u));
    CHECK(isValidTeleportSquare(u32{1} << 13));          // neighbouring bit, not rejected
    CHECK(isValidTeleportSquare((u32{1} << 18) | (u32{1} << 20)));
    CHECK(!isValidTeleportSquare(u32{1} << 14));         // Teleporter wall
    CHECK(!isValidTeleportSquare(u32{1} << 19));         // Rock wall
    CHECK(!isValidTeleportSquare((u32{1} << 19) | 1u));  // Rock plus another bit
    CHECK(!isValidTeleportSquare((u32{1} << 19) | (u32{1} << 14)));
}

// ---------------------------------------------------------------- Hall of Records (R16-R17, check formula)

TEST(hall_record_beaten_only_by_strictly_greater_value) {
    const GameData& data = tsup::data();
    HallRecord rec;
    rec.value = 14;
    rec.holder = "Crashland";
    rec.day = 0;
    CHECK(!hallRecordBeaten(data, RecordKind::Strength, rec, 13));
    CHECK(!hallRecordBeaten(data, RecordKind::Strength, rec, 14));  // equal does not beat
    CHECK(hallRecordBeaten(data, RecordKind::Strength, rec, 15));
}

TEST(hall_deadliest_equal_power_does_not_beat_record) {
    const GameData& data = tsup::data();
    HallRecord rec;
    rec.value = 1;  // monster id 1, compared by power
    rec.holder = "Crashland";
    rec.day = 0;
    CHECK(!hallRecordBeaten(data, RecordKind::Deadliest, rec, 1));
}

TEST(hall_seed_table_matches_srd_starting_records) {
    CHECK_EQ(kRecordKindCount, 12);
    CHECK(kHallSeeds[0].kind == RecordKind::Strength);
    CHECK_EQ(kHallSeeds[0].value, i64{14});
    CHECK(std::string(kHallSeeds[0].holder) == "Crashland");
    CHECK_EQ(kHallSeeds[1].value, i64{15});
    CHECK(std::string(kHallSeeds[1].holder) == "Mager");
    CHECK_EQ(kHallSeeds[2].value, i64{18});
    CHECK(std::string(kHallSeeds[2].holder) == "Theshal");
    CHECK_EQ(kHallSeeds[6].value, i64{1});
    CHECK(kHallSeeds[6].kind == RecordKind::Deadliest);
    CHECK_EQ(kHallSeeds[7].value, i64{1322451});
    CHECK_EQ(kHallSeeds[8].value, i64{5466453});
    CHECK(std::string(kHallSeeds[8].holder) == "Nimblefingers");
    CHECK_EQ(kHallSeeds[10].value, i64{6});
    CHECK(std::string(kHallSeeds[10].holder) == "Ge'nal");
    CHECK_EQ(kOriginalRecordDate, i64{0});
}

// ---------------------------------------------------------------- Monster power (R20)

TEST(count_low_bits_counts_only_requested_range) {
    CHECK_EQ(countLowBits(0xFFFFFFFFu, 14), 14);
    CHECK_EQ(countLowBits(0xFFFFFFFFu, 18), 18);
    CHECK_EQ(countLowBits(0x0Bu, 2), 2);   // bits 0,1,3 set; only 0 and 1 counted
    CHECK_EQ(countLowBits(0x0Bu, 3), 2);   // bit 2 clear
    CHECK_EQ(countLowBits(u32{1} << 14, 14), 0);  // bit 14 is outside 0..13
    CHECK_EQ(countLowBits(u32{1} << 17, 18), 1);
    CHECK_EQ(countLowBits(u32{1} << 18, 18), 0);
}

TEST(monster_power_first_term_uses_bankers_rounding_of_hits) {
    CHECK_EQ(kPowerSpecialBits, 14);
    CHECK_EQ(kPowerSpellBits, 18);
    CHECK_EQ(kPowerSpecialWeight, i64{75});
    CHECK_EQ(kPowerSpellWeight, i64{100});

    Creature m;
    m.hits = 5;   // 0.5 rounds to 0 (even)
    CHECK_EQ(monsterPower(m), i64{0});
    m.hits = 15;  // 1.5 rounds to 2 (even)
    CHECK_EQ(monsterPower(m), i64{2});
    m.hits = 25;  // 2.5 rounds to 2 (even)
    CHECK_EQ(monsterPower(m), i64{2});
    m.hits = 35;  // 3.5 rounds to 4 (even)
    CHECK_EQ(monsterPower(m), i64{4});
}

TEST(monster_power_composition_of_stats) {
    // first = round_even(3*2 + 2*10 + 10/10.0) = round_even(27.0) = 27
    // power = 27 + DEX*2 + GuildLevel = 27 + 8 + 6 = 41
    Creature m;
    m.att = 3;
    m.str = 2;
    m.hits = 10;
    m.dex = 4;
    m.guildLevel = 6;
    CHECK_EQ(monsterPower(m), i64{41});

    Creature onlyDex;
    onlyDex.dex = 3;  // 0 + 0 + 0 + 6 + 0
    CHECK_EQ(monsterPower(onlyDex), i64{6});

    Creature onlyAtt;
    onlyAtt.att = 1;  // 1*2 = 2
    CHECK_EQ(monsterPower(onlyAtt), i64{2});
}

// ---------------------------------------------------------------- Skill indexes (R21-R22)

TEST(fighting_index_scales_total_over_160) {
    Arr<f32, 6> r{};
    CHECK_EQ(fightingSkillIndex(r), 0);

    r[4] = 160.0f;  // total 160 -> 100
    CHECK_EQ(fightingSkillIndex(r), 100);

    Arr<f32, 6> half{};
    half[4] = 80.0f;  // 80/160*100 = 50
    CHECK_EQ(fightingSkillIndex(half), 50);

    Arr<f32, 6> tenth{};
    tenth[4] = 10.0f;  // 10/160*100 = 6.25 -> 6
    CHECK_EQ(fightingSkillIndex(tenth), 6);
}

TEST(fighting_index_adds_weighted_ratings_before_scaling) {
    // rating(2)=5 -> total = round_even(0 + 5*0.2) = 1; the rest add nothing.
    // result = round_even(1/160*100) = round_even(0.625) = 1
    Arr<f32, 6> r{};
    r[2] = 5.0f;
    CHECK_EQ(fightingSkillIndex(r), 1);
}

TEST(thieving_index_uses_rating_times_effective_dex) {
    CHECK_EQ(thievingSkillIndex(35.0f, 100), 100);   // 3500/3500*100
    CHECK_EQ(thievingSkillIndex(10.0f, 175), 50);    // 1750/3500*100
    CHECK_EQ(thievingSkillIndex(10.0f, 1000), 286);  // 10000/3500*100 = 285.7 -> 286
    CHECK_EQ(thievingSkillIndex(1.0f, 0), 0);        // product floored at 1, 1/3500*100 -> 0
}

// ---------------------------------------------------------------- Pluralisation (R25-R26)

TEST(pluralise_count_one_or_less_is_unchanged) {
    CHECK(strIs(pluraliseName("Dwarf", 1), "Dwarf"));
    CHECK(strIs(pluraliseName("Orc", 0), "Orc"));
}

TEST(pluralise_unchanged_endings) {
    CHECK(strIs(pluraliseName("Fish", 2), "Fish"));    // ends ish
    CHECK(strIs(pluraliseName("Tree", 2), "Tree"));    // ends ee
    CHECK(strIs(pluraliseName("Ghouls", 2), "Ghouls")); // ends s
}

TEST(pluralise_replacement_endings) {
    CHECK(strIs(pluraliseName("Dwarf", 2), "Dwarves"));  // arf -> ves
    CHECK(strIs(pluraliseName("Thief", 2), "Thieves"));  // ief -> ves
    CHECK(strIs(pluraliseName("Elf", 2), "Elves"));      // Elf -> ves
    CHECK(strIs(pluraliseName("Wolf", 2), "Wolves"));    // olf -> ves
    CHECK(strIs(pluraliseName("Fairy", 2), "Fairies"));  // ry -> ies
    CHECK(strIs(pluraliseName("Harpy", 2), "Harpies"));  // py -> ies
    CHECK(strIs(pluraliseName("Mummy", 2), "Mummies"));  // mmy -> ies
    CHECK(strIs(pluraliseName("Leech", 2), "Leeches"));  // Leech gets es
    CHECK(strIs(pluraliseName("Witch", 2), "Witchs"));   // other ch words get s
}

TEST(pluralise_man_and_bus_rules) {
    CHECK(strIs(pluraliseName("Shaman", 2), "Shamen"));   // man -> men
    CHECK(strIs(pluraliseName("omnibus", 2), "omnibi"));  // bus: last 2 -> i
}

TEST(pluralise_default_adds_s) {
    CHECK(strIs(pluraliseName("Orc", 2), "Orcs"));
    CHECK(strIs(pluraliseName("Troll", 2), "Trolls"));
}

// ---------------------------------------------------------------- Currency (R27-R28)

TEST(currency_from_double_scales_to_ten_thousandths) {
    CurrencyFromDoubleResult pos = currencyFromDoubleChecked(1.5);
    CHECK(!pos.overflow);
    CHECK_EQ(pos.value, i64{15000});

    CurrencyFromDoubleResult neg = currencyFromDoubleChecked(-1.5);
    CHECK(!neg.overflow);
    CHECK_EQ(neg.value, i64{-15000});
}

TEST(currency_from_double_rounds_half_to_even_at_four_places) {
    // 0.03125 * 10000 = 312.5 exactly -> 312 (even)
    CurrencyFromDoubleResult a = currencyFromDoubleChecked(0.03125);
    CHECK(!a.overflow);
    CHECK_EQ(a.value, i64{312});

    // 0.09375 * 10000 = 937.5 exactly -> 938 (even)
    CurrencyFromDoubleResult b = currencyFromDoubleChecked(0.09375);
    CHECK(!b.overflow);
    CHECK_EQ(b.value, i64{938});
}

TEST(currency_from_double_overflow_bounds) {
    CurrencyFromDoubleResult fits = currencyFromDoubleChecked(1e14);  // raw 1e18 < 2^63
    CHECK(!fits.overflow);
    CHECK_EQ(fits.value, i64{1000000000000000000LL});

    CurrencyFromDoubleResult big = currencyFromDoubleChecked(1e15);   // raw 1e19 > 2^63
    CHECK(big.overflow);

    CurrencyFromDoubleResult nan = currencyFromDoubleChecked(std::nan(""));
    CHECK(nan.overflow);
}

// ---------------------------------------------------------------- Extended precision (R29)

TEST(x87_store_to_long_rounds_half_to_even) {
    CHECK_EQ(x87StoreToLong(x87FromDouble(2.5)), i64{2});
    CHECK_EQ(x87StoreToLong(x87FromDouble(3.5)), i64{4});
    CHECK_EQ(x87StoreToLong(x87FromDouble(0.5)), i64{0});
    CHECK_EQ(x87StoreToLong(x87FromDouble(-2.5)), i64{-2});
    CHECK_EQ(x87StoreToLong(x87FromDouble(1.4)), i64{1});
}

TEST(x87_arithmetic_then_store_rounds_result) {
    // 1.25 + 2.25 = 3.5 -> 4
    CHECK_EQ(x87StoreToLong(x87Add(x87FromDouble(1.25), x87FromDouble(2.25))), i64{4});
    // 3.0 * 2.5 = 7.5 -> 8 (even)
    CHECK_EQ(x87StoreToLong(x87Mul(x87FromDouble(3.0), x87FromDouble(2.5))), i64{8});
    // 10 / 4 = 2.5 -> 2 (even)
    CHECK_EQ(x87StoreToLong(x87Div(x87FromDouble(10.0), x87FromDouble(4.0))), i64{2});
    // 7 / 2 = 3.5 -> 4
    CHECK_EQ(x87StoreToLong(x87Div(x87FromDouble(7.0), x87FromDouble(2.0))), i64{4});
}

TEST(x87_value_is_mantissa_times_power_of_two) {
    // value = M * 2^E with M = 2^63 and E = -63, so value = 1
    X87Value one;
    one.mantissa = u64{1} << 63;
    one.exponent = -63;
    one.negative = false;
    CHECK_EQ(x87StoreToLong(one), i64{1});

    X87Value minusOne = one;
    minusOne.negative = true;
    CHECK_EQ(x87StoreToLong(minusOne), i64{-1});
}
