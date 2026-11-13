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

// ---------------------------------------------------------------------------
// Experience multiplier (R10, R11).
// The SRD notes the formula does not reproduce the table exactly for Elf,
// Osiri and Troll, so these checks use a tolerance rather than exact equality.
// ---------------------------------------------------------------------------

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

