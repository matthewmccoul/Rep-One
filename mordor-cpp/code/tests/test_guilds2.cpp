#include "test_support.h"
#include "mordor/guilds2.h"

#include <cstring>
#include <initializer_list>

using namespace mordor;

namespace {

u32 maskOf(std::initializer_list<Guild> gs) {
    u32 m = 0;
    for (Guild g : gs) {
        m |= 1u << static_cast<u32>(g);
    }
    return m;
}

int gi(Guild g) { return static_cast<int>(g); }
int ri(Race r) { return static_cast<int>(r); }

// FIXED: kGoldFactor is not declared in any header. Defined locally with the
// T17 Actual GP column values that the test below checks against.
const double kGoldFactor[kGuildCount] = {1.3, 1.1, 1.2, 1.5, 1.5, 1.3, 1.9, 1.7, 1.3, 1.3, 1.4, 1.4};

}  // namespace

TEST(guilds2_guild_and_ability_enum_order) {
    // T1: guilds in table order; T13: six guild abilities.
    CHECK_EQ(gi(Guild::Nomad), 0);
    CHECK_EQ(gi(Guild::Warrior), 1);
    CHECK_EQ(gi(Guild::Thief), 6);
    CHECK_EQ(gi(Guild::Scavenger), 7);
    CHECK_EQ(gi(Guild::Healer), 11);
    CHECK_EQ(kGuildCount, 12);

    CHECK_EQ(static_cast<int>(GuildAbility::Thieving), 0);
    CHECK_EQ(static_cast<int>(GuildAbility::Backstabbing), 1);
    CHECK_EQ(static_cast<int>(GuildAbility::CriticalHit), 2);
    CHECK_EQ(static_cast<int>(GuildAbility::MultipleSwings), 3);
    CHECK_EQ(static_cast<int>(GuildAbility::Fighting), 4);
    CHECK_EQ(static_cast<int>(GuildAbility::Perception), 5);
    CHECK_EQ(kGuildAbilityCount, 6);
}

TEST(guilds2_swing_level_rows) {
    CHECK_EQ(static_cast<int>(sizeof(kSwingLevelRows) / sizeof(kSwingLevelRows[0])), 5);
    CHECK(kSwingLevelRows[0].guild == Guild::Ninja);
    CHECK_EQ(kSwingLevelRows[0].levelCount, 4);
    CHECK_EQ(kSwingLevelRows[0].levels[3], 585);
    CHECK(kSwingLevelRows[1].guild == Guild::Warrior);
    CHECK_EQ(kSwingLevelRows[1].levels[1], 324);
    CHECK(kSwingLevelRows[4].guild == Guild::Scavenger);
    CHECK_EQ(kSwingLevelRows[4].levels[1], 531);
}

TEST(guilds2_seed_masters_and_crests) {
    // T2 seeded levels and names; T3 crest item ids.
    const i32 wantLevel[kGuildCount] = {64, 56, 62, 48, 67, 77, 56, 54, 52, 61, 48, 65};
    for (int g = 0; g < kGuildCount; ++g) {
        CHECK_EQ(kSeedMasterLevel[g], wantLevel[g]);
        CHECK_EQ(kCrestItemId[g], 299 + g);
    }
    CHECK_EQ(kCrestItemIdBase, 299);
    CHECK(std::strcmp(kSeedMasterName[0], "Crashland") == 0);
    CHECK(std::strcmp(kSeedMasterName[7], "Kane the Wise Axe") == 0);
    CHECK(std::strcmp(kSeedMasterName[11], "Hands on Henry") == 0);
}

TEST(guilds2_max_attack_defence_table) {
    // T6 values; Ninja is 254/254, so X equals Y.
    CHECK_EQ(kMaxAttack[gi(Guild::Nomad)], 175);
    CHECK_EQ(kMaxDefence[gi(Guild::Nomad)], 175);
    CHECK_EQ(kMaxAdLevel[gi(Guild::Nomad)], 197);
    CHECK_EQ(kMaxAttack[gi(Guild::Warrior)], 254);
    CHECK_EQ(kMaxDefence[gi(Guild::Warrior)], 276);
    CHECK_EQ(kMaxAdLevel[gi(Guild::Warrior)], 450);
    CHECK_EQ(kMaxAttack[gi(Guild::Ninja)], 254);
    CHECK_EQ(kMaxDefence[gi(Guild::Ninja)], 254);
    CHECK_EQ(kMaxAdLevel[gi(Guild::Ninja)], 457);
    CHECK_EQ(kMaxAttack[gi(Guild::Thief)], 168);
    CHECK_EQ(kMaxDefence[gi(Guild::Thief)], 204);
    CHECK_EQ(kMaxAdLevel[gi(Guild::Thief)], 394);
    CHECK_EQ(kMaxAttack[gi(Guild::Wizard)], 106);
    CHECK_EQ(kMaxDefence[gi(Guild::Wizard)], 232);
    CHECK_EQ(kMaxAdLevel[gi(Guild::Wizard)], 622);
    CHECK_EQ(kMaxAttack[gi(Guild::Sorcerer)], 98);
    CHECK_EQ(kMaxDefence[gi(Guild::Sorcerer)], 195);
    CHECK_EQ(kMaxAdLevel[gi(Guild::Sorcerer)], 467);
}

TEST(guilds2_exp_factor_is_raw_over_eight) {
    // T17: Actual EP = EP raw / 8 for every guild (Nomad = 1.0).
    for (int g = 0; g < kGuildCount; ++g) {
        CHECK(kExpFactor[g] == static_cast<double>(kExpFactorRaw[g]) / 8.0);
    }
    CHECK(kExpFactor[gi(Guild::Healer)] == 4.0);
    CHECK(kExpFactor[gi(Guild::Thief)] == 1.875);
}

TEST(guilds2_gold_factor_and_advance_constants) {
    // T17 Actual GP column; R20 rate 60.2 and input guildGold/10.
    const double wantGp[kGuildCount] = {1.3, 1.1, 1.2, 1.5, 1.5, 1.3, 1.9, 1.7, 1.3, 1.3, 1.4, 1.4};
    for (int g = 0; g < kGuildCount; ++g) {
        CHECK(kGoldFactor[g] == wantGp[g]);
    }
    CHECK(kAdvanceGoldRate == 60.2);
    CHECK_EQ(kAdvanceGoldDivisor, 10);
}

TEST(guilds2_avg_hits_and_quest_percent) {
    // T16 AH and QP columns. Sorcerer and Wizard QP are excluded for Wizard (conflict).
    const i32 wantAH[kGuildCount] = {5, 6, 5, 4, 4, 4, 3, 4, 3, 2, 2, 2};
    const i32 wantQP[kGuildCount] = {20, 20, 25, 35, 25, 20, 15, 20, 15, 10, 15, 15};
    for (int g = 0; g < kGuildCount; ++g) {
        CHECK_EQ(kAvgHits[g], wantAH[g]);
        if (g != gi(Guild::Wizard)) {
            CHECK_EQ(kQuestPercent[g], wantQP[g]);
        }
    }
}

TEST(guilds2_hits_cap_and_late_hits) {
    // T16 ML (skip Sorcerer and Wizard, which are in conflict) and MH column.
    CHECK_EQ(kHitsCapLevel[gi(Guild::Nomad)], 30);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Warrior)], 28);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Paladin)], 29);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Ninja)], 25);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Villain)], 26);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Seeker)], 27);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Thief)], 21);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Scavenger)], 24);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Mage)], 27);
    CHECK_EQ(kHitsCapLevel[gi(Guild::Healer)], 28);
    CHECK_EQ(kLateHits[gi(Guild::Nomad)], 1);
    for (int g = 1; g < kGuildCount; ++g) {
        CHECK_EQ(kLateHits[g], 2);
    }
}

TEST(guilds2_min_stat_row_sums) {
    // T19 rows summed; sums do not depend on the engine's stat index order.
    auto sum = [](int g) {
        i32 s = 0;
        for (int k = 0; k < kStatCount; ++k) {
            s += kGuildMinStat[g][k];
        }
        return s;
    };
    CHECK_EQ(sum(gi(Guild::Nomad)), 6);
    CHECK_EQ(sum(gi(Guild::Warrior)), 47);
    CHECK_EQ(sum(gi(Guild::Paladin)), 73);
    CHECK_EQ(sum(gi(Guild::Ninja)), 58);
    CHECK_EQ(sum(gi(Guild::Thief)), 56);
    CHECK_EQ(sum(gi(Guild::Wizard)), 77);
}

TEST(guilds2_ability_ratings_table) {
    // T18 sheet values, indexed by GuildAbility (T13 order).
    const int thief = static_cast<int>(GuildAbility::Thieving);
    const int back = static_cast<int>(GuildAbility::Backstabbing);
    const int crit = static_cast<int>(GuildAbility::CriticalHit);
    const int swing = static_cast<int>(GuildAbility::MultipleSwings);
    const int fight = static_cast<int>(GuildAbility::Fighting);
    const int perc = static_cast<int>(GuildAbility::Perception);
    CHECK(kGuildAbilityRating[gi(Guild::Thief)][thief] == 9.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Thief)][back] == 9.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Warrior)][crit] == 9.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Warrior)][fight] == 12.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Ninja)][swing] == 8.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Scavenger)][thief] == 7.5f);
    CHECK(kGuildAbilityRating[gi(Guild::Seeker)][thief] == 3.5f);
    CHECK(kGuildAbilityRating[gi(Guild::Seeker)][crit] == 3.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Seeker)][perc] == 9.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Nomad)][fight] == 7.0f);
    CHECK(kGuildAbilityRating[gi(Guild::Nomad)][perc] == 5.0f);
}

TEST(guilds2_alignment_masks) {
    // T20 alignment by guild.
    CHECK(kGuildAlignMask[gi(Guild::Nomad)] == (kAlignBitGood | kAlignBitNeutral | kAlignBitEvil));
    CHECK(kGuildAlignMask[gi(Guild::Paladin)] == kAlignBitGood);
    CHECK(kGuildAlignMask[gi(Guild::Ninja)] == kAlignBitEvil);
    CHECK(kGuildAlignMask[gi(Guild::Villain)] == kAlignBitEvil);
    CHECK(kGuildAlignMask[gi(Guild::Thief)] == kAlignBitNeutral);
    CHECK(kGuildAlignMask[gi(Guild::Healer)] == kAlignBitNeutral);
    CHECK(kGuildAlignMask[gi(Guild::Mage)] == (kAlignBitGood | kAlignBitEvil));
}

TEST(guilds2_race_mask_matches_chart) {
    // T8 race/guild chart encoded as guild lists; masks use bit = guild index.
    CHECK(kRaceGuildMask[ri(Race::Human)] == maskOf({Guild::Nomad, Guild::Warrior, Guild::Paladin,
        Guild::Ninja, Guild::Villain, Guild::Seeker, Guild::Thief, Guild::Scavenger, Guild::Mage,
        Guild::Sorcerer, Guild::Wizard, Guild::Healer}));
    CHECK(kRaceGuildMask[ri(Race::Elf)] == maskOf({Guild::Nomad, Guild::Paladin, Guild::Seeker,
        Guild::Thief, Guild::Scavenger, Guild::Mage, Guild::Sorcerer, Guild::Wizard, Guild::Healer}));
    CHECK(kRaceGuildMask[ri(Race::Giant)] == maskOf({Guild::Nomad, Guild::Warrior, Guild::Seeker}));
    CHECK(kRaceGuildMask[ri(Race::Gnome)] == maskOf({Guild::Nomad, Guild::Paladin, Guild::Villain,
        Guild::Seeker, Guild::Thief, Guild::Scavenger, Guild::Mage, Guild::Sorcerer, Guild::Healer}));
    CHECK(kRaceGuildMask[ri(Race::Dwarf)] == maskOf({Guild::Nomad, Guild::Warrior, Guild::Seeker,
        Guild::Thief, Guild::Scavenger, Guild::Sorcerer, Guild::Wizard, Guild::Healer}));
    CHECK(kRaceGuildMask[ri(Race::Ogre)] == maskOf({Guild::Nomad, Guild::Warrior, Guild::Scavenger}));
    CHECK(kRaceGuildMask[ri(Race::Morloch)] == maskOf({Guild::Nomad, Guild::Warrior, Guild::Villain,
        Guild::Seeker, Guild::Scavenger, Guild::Mage, Guild::Sorcerer}));
    CHECK(kRaceGuildMask[ri(Race::Osiri)] == maskOf({Guild::Nomad, Guild::Seeker, Guild::Thief,
        Guild::Scavenger, Guild::Sorcerer, Guild::Wizard, Guild::Healer}));
    CHECK(kRaceGuildMask[ri(Race::Troll)] == maskOf({Guild::Nomad, Guild::Ninja, Guild::Villain,
        Guild::Thief, Guild::Scavenger}));
}

TEST(guilds2_guild_item_marker) {
    // Guild items carry ailments >= 500; guildIndex is ailments - 500.
    Item it;
    it.ailments = 500;
    CHECK(it.isGuildItem());
    CHECK_EQ(it.guildIndex(), 0);
    it.ailments = 506;
    CHECK(it.isGuildItem());
    CHECK_EQ(it.guildIndex(), gi(Guild::Thief));
    it.ailments = 499;
    CHECK(!it.isGuildItem());
}

TEST(guilds2_ninja_title_ladder) {
    // T10 (reference bands): 119 is Assassin, 120 is Shogun, 400 is Death Master.
    // Ninja and Master Ninja are never reached.
    GuildMembership m;
    m.level = 119;
    CHECK(GuildTitle(m, Guild::Ninja) == Str("Assassin"));
    m.level = 120;
    CHECK(GuildTitle(m, Guild::Ninja) == Str("Shogun"));
    m.level = 400;
    CHECK(GuildTitle(m, Guild::Ninja) == Str("Death Master"));
}

TEST(guilds2_nomad_title_thresholds) {
    // T9: Nomad title thresholds; Master Nomad at 220.
    const i32 want[12] = {10, 20, 30, 40, 50, 60, 80, 100, 130, 150, 190, 220};
    for (int k = 0; k < 12; ++k) {
        CHECK_EQ(kNomadTitleLevel[k], want[k]);
    }
    CHECK_EQ(kNomadMasterLevel, 220);
    CHECK_EQ(kNinjaShogunLevel, 120);
}
