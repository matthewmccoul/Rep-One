#include "test_support.h"
#include "mordor/races.h"

#include <cmath>

namespace m = mordor;

// Tolerance comparison for the f64 results.
static bool near_(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

// Character with only the race set; other fields keep their defaults.
static m::Character make_char(m::Race race) {
    m::Character c;
    c.race = race;
    return c;
}

// ---------------------------------------------------------------------------
// Enum and constant layout (SRD R1, R3, R7)
// ---------------------------------------------------------------------------

TEST(race_count_and_enum_order) {
    // Nine playable races: Human, Elf, Giant, Gnome, Dwarf, Ogre, Morloch, Osiri, Troll.
    CHECK_EQ(m::kRaceCount, 9);
    CHECK_EQ(static_cast<int>(m::Race::Human), 0);
    CHECK_EQ(static_cast<int>(m::Race::Elf), 1);
    CHECK_EQ(static_cast<int>(m::Race::Troll), 8);
}

TEST(resist_category_index_order) {
    // R7: Fire, Cold, Electrical, Mind, Disease, Poison, Magic, Stone,
    // Paralysis, Drain, Acid. Index 3 = Mind, index 6 = Magic.
    CHECK_EQ(static_cast<int>(m::ResistCategory::Fire), 0);
    CHECK_EQ(static_cast<int>(m::ResistCategory::Mind), 3);
    CHECK_EQ(static_cast<int>(m::ResistCategory::Magic), 6);
    CHECK_EQ(static_cast<int>(m::ResistCategory::Acid), 10);
}

TEST(stat_index_order_and_ceiling_offset) {
    // Stat order Str, Int, Wis, Con, Cha, Dex (T2).
    CHECK_EQ(static_cast<int>(m::StatIndex::Str), 0);
    CHECK_EQ(static_cast<int>(m::StatIndex::Wis), 2);
    CHECK_EQ(static_cast<int>(m::StatIndex::Dex), 5);
    // R3: ceiling = race max + 5.
    CHECK_EQ(m::kStatCeilingOffset, 5);
}

// ---------------------------------------------------------------------------
// Stat ceiling (R3): ceiling = Races(r).M553C(stat) + 5
// ---------------------------------------------------------------------------

TEST(stat_ceiling_human_strength_worked_example) {
    // SRD worked example: Human STR max 17, so 17 + 5 = 22.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::statCeiling(env.ctx, m::Race::Human, m::StatIndex::Str), 22);
}

TEST(stat_ceiling_elf_intelligence) {
    // Elf INT max 20 (T2), so 25.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::statCeiling(env.ctx, m::Race::Elf, m::StatIndex::Int), 25);
}

TEST(stat_ceiling_giant_strength_high_max) {
    // Giant STR max 25 (T2), so 30.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::statCeiling(env.ctx, m::Race::Giant, m::StatIndex::Str), 30);
}

TEST(stat_ceiling_dwarf_wisdom) {
    // Dwarf WIS max 19 (T2), so 24.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::statCeiling(env.ctx, m::Race::Dwarf, m::StatIndex::Wis), 24);
}

TEST(stat_ceiling_osiri_and_morloch_dexterity) {
    // Osiri DEX max 22 -> 27. Morloch DEX max 20 -> 25.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::statCeiling(env.ctx, m::Race::Osiri, m::StatIndex::Dex), 27);
    CHECK_EQ(m::statCeiling(env.ctx, m::Race::Morloch, m::StatIndex::Dex), 25);
}

// ---------------------------------------------------------------------------
// raiseStat (R3): play-time raises stop at the ceiling
// ---------------------------------------------------------------------------

TEST(raise_stat_below_ceiling_adds_full_amount) {
    // Human STR 10 + 3 = 13, which is under the ceiling of 22.
    tsup::Env env({0.5, 0.9});
    m::Character ch = make_char(m::Race::Human);
    ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)] = 10;
    m::raiseStat(env.ctx, ch, m::StatIndex::Str, 3);
    CHECK_EQ(ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)], 13);
}

TEST(raise_stat_clamps_at_human_strength_ceiling) {
    // Human STR 21 + 5 = 26, capped at 22.
    tsup::Env env({0.5, 0.9});
    m::Character ch = make_char(m::Race::Human);
    ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)] = 21;
    m::raiseStat(env.ctx, ch, m::StatIndex::Str, 5);
    CHECK_EQ(ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)], 22);
}

TEST(raise_stat_at_ceiling_does_not_move) {
    // Already at 22; any further raise leaves it at 22.
    tsup::Env env({0.5, 0.9});
    m::Character ch = make_char(m::Race::Human);
    ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)] = 22;
    m::raiseStat(env.ctx, ch, m::StatIndex::Str, 1);
    CHECK_EQ(ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)], 22);
}

TEST(raise_stat_uses_race_specific_ceiling) {
    // Giant STR ceiling is 30. From 28 a raise of 5 caps at 30.
    tsup::Env env({0.5, 0.9});
    m::Character ch = make_char(m::Race::Giant);
    ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)] = 28;
    m::raiseStat(env.ctx, ch, m::StatIndex::Str, 5);
    CHECK_EQ(ch.abilities.stats[static_cast<unsigned>(m::StatIndex::Str)], 30);
}

// ---------------------------------------------------------------------------
// Race-guild grid (R4, Table T4, sheet [c71]).
// Enforcement is UNKNOWN; these tests check the grid values only.
// ---------------------------------------------------------------------------

TEST(guild_grid_human_allows_all_twelve) {
    tsup::Env env({0.5, 0.9});
    const m::Guild all[] = {
        m::Guild::Nomad, m::Guild::Warrior, m::Guild::Paladin, m::Guild::Ninja,
        m::Guild::Villain, m::Guild::Seeker, m::Guild::Thief, m::Guild::Scavenger,
        m::Guild::Mage, m::Guild::Sorcerer, m::Guild::Wizard, m::Guild::Healer,
    };
    for (m::Guild g : all) {
        CHECK(m::guildAllowed(env.ctx, m::Race::Human, g));
    }
}

TEST(guild_grid_dwarf_uses_sheet_not_wiki) {
    // Dwarf per [c71] (preferred over [c96] "all guilds"):
    // NOM, WAR, SEE, THI, SCA, SOR, WIZ, HEA.
    tsup::Env env({0.5, 0.9});
    CHECK(m::guildAllowed(env.ctx, m::Race::Dwarf, m::Guild::Warrior));
    CHECK(m::guildAllowed(env.ctx, m::Race::Dwarf, m::Guild::Wizard));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Dwarf, m::Guild::Paladin));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Dwarf, m::Guild::Mage));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Dwarf, m::Guild::Villain));
}

TEST(guild_grid_gnome_cannot_be_warrior_or_wizard) {
    // SRD T4 note: Gnome cannot be Warrior or Wizard [c96], matching [c71].
    tsup::Env env({0.5, 0.9});
    CHECK(!m::guildAllowed(env.ctx, m::Race::Gnome, m::Guild::Warrior));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Gnome, m::Guild::Wizard));
    CHECK(m::guildAllowed(env.ctx, m::Race::Gnome, m::Guild::Mage));
    CHECK(m::guildAllowed(env.ctx, m::Race::Gnome, m::Guild::Villain));
}

TEST(guild_grid_troll_small_list) {
    // Troll: NOM, NIN, VIL, THI, SCA only.
    tsup::Env env({0.5, 0.9});
    CHECK(m::guildAllowed(env.ctx, m::Race::Troll, m::Guild::Ninja));
    CHECK(m::guildAllowed(env.ctx, m::Race::Troll, m::Guild::Scavenger));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Troll, m::Guild::Paladin));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Troll, m::Guild::Warrior));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Troll, m::Guild::Healer));
}

TEST(guild_grid_giant_only_three_guilds) {
    // Giant: NOM, WAR, SEE only.
    tsup::Env env({0.5, 0.9});
    CHECK(m::guildAllowed(env.ctx, m::Race::Giant, m::Guild::Seeker));
    CHECK(m::guildAllowed(env.ctx, m::Race::Giant, m::Guild::Warrior));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Giant, m::Guild::Thief));
    CHECK(!m::guildAllowed(env.ctx, m::Race::Giant, m::Guild::Mage));
}

// ---------------------------------------------------------------------------
// Alignment mask (R5, Table T1, [c62, c71] preferred over conflicts).
// Enforcement point is UNKNOWN; these check the mask only.
// ---------------------------------------------------------------------------

TEST(alignment_mask_human_and_elf_allow_all) {
    // Mask XXX for Human and Elf: consistent across sources.
    tsup::Env env({0.5, 0.9});
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Human, m::Align::Good));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Human, m::Align::Neutral));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Human, m::Align::Evil));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Elf, m::Align::Good));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Elf, m::Align::Neutral));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Elf, m::Align::Evil));
}

TEST(alignment_mask_osiri_neutral_only) {
    // Osiri mask -X- (N only) per [c62, c71].
    tsup::Env env({0.5, 0.9});
    CHECK(!m::alignmentAllowed(env.ctx, m::Race::Osiri, m::Align::Good));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Osiri, m::Align::Neutral));
    CHECK(!m::alignmentAllowed(env.ctx, m::Race::Osiri, m::Align::Evil));
}

TEST(alignment_mask_morloch_good_and_evil) {
    // Morloch mask X-X (G and E, not N) per [c62, c71].
    tsup::Env env({0.5, 0.9});
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Morloch, m::Align::Good));
    CHECK(!m::alignmentAllowed(env.ctx, m::Race::Morloch, m::Align::Neutral));
    CHECK(m::alignmentAllowed(env.ctx, m::Race::Morloch, m::Align::Evil));
}

// ---------------------------------------------------------------------------
// Resistances (R6 index order; Table T3 from [c62]).
// How percentages are applied is UNKNOWN, so applyResistance is not tested.
// ---------------------------------------------------------------------------

TEST(resistance_elf_mind_and_magic) {
    // Elf: Mind 50, Magic 80, Fire 0.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Elf, m::ResistCategory::Mind), 50);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Elf, m::ResistCategory::Magic), 80);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Elf, m::ResistCategory::Fire), 0);
}

TEST(resistance_dwarf_cold_and_drain) {
    // Dwarf: Cold 80, Drain 90, Fire 45.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Dwarf, m::ResistCategory::Cold), 80);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Dwarf, m::ResistCategory::Drain), 90);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Dwarf, m::ResistCategory::Fire), 45);
}

TEST(resistance_osiri_high_values) {
    // Osiri: Electrical 90, Mind 90, Stone 90, Acid 90.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Osiri, m::ResistCategory::Electrical), 90);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Osiri, m::ResistCategory::Mind), 90);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Osiri, m::ResistCategory::Stone), 90);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Osiri, m::ResistCategory::Acid), 90);
}

TEST(resistance_morloch_cold_uses_test_value) {
    // Morloch Cold 35 per [c62, c82]; [c87] "53" is treated as a typo.
    // Disease and Poison 0 per [c62], which wins over the manual.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Morloch, m::ResistCategory::Cold), 35);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Morloch, m::ResistCategory::Disease), 0);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Morloch, m::ResistCategory::Poison), 0);
}

TEST(resistance_human_all_zero_and_unlisted_are_zero) {
    // R6: categories not listed for a race are 0. Human lists zero everywhere.
    tsup::Env env({0.5, 0.9});
    const m::ResistCategory all[] = {
        m::ResistCategory::Fire, m::ResistCategory::Cold, m::ResistCategory::Electrical,
        m::ResistCategory::Mind, m::ResistCategory::Disease, m::ResistCategory::Poison,
        m::ResistCategory::Magic, m::ResistCategory::Stone, m::ResistCategory::Paralysis,
        m::ResistCategory::Drain, m::ResistCategory::Acid,
    };
    for (m::ResistCategory c : all) {
        CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Human, c), 0);
    }
}

TEST(resistance_troll_drain_and_ogre_acid) {
    // Troll Drain 65; Ogre Acid 15; Giant Stone 25.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Troll, m::ResistCategory::Drain), 65);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Ogre, m::ResistCategory::Acid), 15);
    CHECK_EQ(m::resistancePercent(env.ctx, m::Race::Giant, m::ResistCategory::Stone), 25);
}

// ---------------------------------------------------------------------------
// Experience multiplier (R10, R11).
// The SRD notes the formula does not reproduce the table exactly for Elf,
// Osiri and Troll, so these checks use a tolerance rather than exact equality.
// ---------------------------------------------------------------------------

TEST(exp_multiplier_human_is_exactly_one) {
    // Human Exp 3: ln(3)/ln(3) - 1 = 0, so the multiplier is 1.0.
    tsup::Env env({0.5, 0.9});
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Human), 1.0, 1e-9));
}

TEST(exp_multiplier_tracks_documented_values) {
    // Each value is within 0.005 of the T5 table (formula gives e.g. Elf
    // about 1.18656 versus the table's 1.18545).
    tsup::Env env({0.5, 0.9});
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Elf), 1.18544741085, 0.005));
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Dwarf), 1.10551994263, 0.005));
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Ogre), 1.14714602440, 0.005));
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Morloch), 1.05819132268, 0.005));
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Osiri), 1.22165868435, 0.005));
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Troll), 1.25647909518, 0.005));
}

TEST(exp_multiplier_troll_hand_check) {
    // Troll Exp 9: ln9/ln3 = 2, so numerator 1.
    // Denominator 4.50 - 5 * (11/90) = 3.888889, giving 1/3.888889 = 0.257143.
    // Multiplier is about 1.25714, so the test allows 0.001 slack.
    tsup::Env env({0.5, 0.9});
    CHECK(near_(m::expMultiplier(env.ctx, m::Race::Troll), 1.257143, 0.001));
}

TEST(racial_penalty_matches_archive_table) {
    // R11 / header table: Human 1.0, Troll 1.25647909518, Osiri 1.22165868435.
    CHECK(near_(m::racialPenalty(m::Race::Human), 1.0, 1e-12));
    CHECK(near_(m::racialPenalty(m::Race::Troll), 1.25647909518, 1e-9));
    CHECK(near_(m::racialPenalty(m::Race::Osiri), 1.22165868435, 1e-9));
    CHECK(near_(m::racialPenalty(m::Race::Morloch), 1.05819132268, 1e-9));
}

// ---------------------------------------------------------------------------
// Base stat generation (R2): values fall in the T2 min-max range.
// The generation procedure is UNKNOWN, so only the range invariant is checked.
// ---------------------------------------------------------------------------

TEST(base_stats_stay_within_race_bounds) {
    // Human STR 4-17, Giant STR 12-25, Osiri DEX 10-22 (T2).
    tsup::Env env(777);
    for (int i = 0; i < 200; ++i) {
        int h = m::generateBaseStat(env.ctx, m::Race::Human, m::StatIndex::Str);
        CHECK(h >= 4 && h <= 17);
        int g = m::generateBaseStat(env.ctx, m::Race::Giant, m::StatIndex::Str);
        CHECK(g >= 12 && g <= 25);
        int o = m::generateBaseStat(env.ctx, m::Race::Osiri, m::StatIndex::Dex);
        CHECK(o >= 10 && o <= 22);
    }
}
