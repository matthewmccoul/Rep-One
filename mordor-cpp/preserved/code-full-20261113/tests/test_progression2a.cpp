#include <cmath>
#include <initializer_list>

#include "test_support.h"
#include "mordor/progression2a.h"

using namespace mordor;

namespace {

bool nearD(double a, double b) { return std::fabs(a - b) < 1e-9; }
i32 ri(Race r) { return static_cast<i32>(r); }
i32 gi(Guild g) { return static_cast<i32>(g); }

Vec<bool> roster(std::initializer_list<bool> flags) {
    Vec<bool> v;
    for (bool f : flags) v.push_back(f);
    return v;
}

}  // namespace

TEST(p2a_start_age_is_16_years_in_days) {
    CHECK_EQ(kAgeStartDays, 5840);
    CHECK_EQ(kAgeStartDays, 16 * 365);
}

TEST(p2a_aging_amount_constants) {
    CHECK_EQ(kKind25AgeDays, 28);
    CHECK_EQ(kSanctuaryAgeDays, 365);
}

TEST(p2a_cap_and_route_constants) {
    CHECK_EQ(kMaxHitsCap, 999);
    CHECK_EQ(kMc00BC, 999);
    CHECK_EQ(kWikiPinCapLevel, 998);
    CHECK_EQ(kGuildMaxLevel, 999);
    CHECK_EQ(kRouteConBonusMin, 17);
    CHECK_EQ(kRouteWarriorTarget, 28);
    CHECK_EQ(kRouteNomadTarget, 30);
    CHECK_EQ(kRouteWizardTarget, 41);
    CHECK_EQ(kRouteNomadMaxLevel, 30);
    CHECK_EQ(kReversalHitLoss, 28);
    CHECK(nearD(kCompanionXpFactor, 0.25));
    CHECK(nearD(kChestXpFactor, 0.5));
}

TEST(p2a_race_raw_exp_matches_table) {
    CHECK_EQ(kRaceRawExp[ri(Race::Human)], 3);
    CHECK_EQ(kRaceRawExp[ri(Race::Elf)], 7);
    CHECK_EQ(kRaceRawExp[ri(Race::Giant)], 7);
    CHECK_EQ(kRaceRawExp[ri(Race::Gnome)], 7);
    CHECK_EQ(kRaceRawExp[ri(Race::Dwarf)], 5);
    CHECK_EQ(kRaceRawExp[ri(Race::Ogre)], 6);
    CHECK_EQ(kRaceRawExp[ri(Race::Morloch)], 4);
    CHECK_EQ(kRaceRawExp[ri(Race::Osiri)], 8);
    CHECK_EQ(kRaceRawExp[ri(Race::Troll)], 9);
}

TEST(p2a_race_mult_by_raw_exp_matches_table) {
    CHECK(nearD(kRaceMultByRawExp[0], 1.0));
    CHECK(nearD(kRaceMultByRawExp[1], 1.0));
    CHECK(nearD(kRaceMultByRawExp[2], 1.0));
    CHECK(nearD(kRaceMultByRawExp[3], 1.0));
    CHECK(nearD(kRaceMultByRawExp[4], 1.05819132268));
    CHECK(nearD(kRaceMultByRawExp[5], 1.10551994263));
    CHECK(nearD(kRaceMultByRawExp[6], 1.14714602440));
    CHECK(nearD(kRaceMultByRawExp[7], 1.18544741085));
    CHECK(nearD(kRaceMultByRawExp[8], 1.22165868435));
    CHECK(nearD(kRaceMultByRawExp[9], 1.25647909518));
}

TEST(p2a_race_mult_follows_each_race_raw_exp) {
    CHECK(nearD(kRaceMultByRawExp[kRaceRawExp[ri(Race::Human)]], 1.0));
    CHECK(nearD(kRaceMultByRawExp[kRaceRawExp[ri(Race::Morloch)]], 1.05819132268));
    CHECK(nearD(kRaceMultByRawExp[kRaceRawExp[ri(Race::Dwarf)]], 1.10551994263));
    CHECK(nearD(kRaceMultByRawExp[kRaceRawExp[ri(Race::Elf)]], 1.18544741085));
    CHECK(nearD(kRaceMultByRawExp[kRaceRawExp[ri(Race::Troll)]], 1.25647909518));
}

TEST(p2a_guild_parameters_match_table) {
    CHECK_EQ(kGuildParameter[gi(Guild::Nomad)], 8);
    CHECK_EQ(kGuildParameter[gi(Guild::Warrior)], 16);
    CHECK_EQ(kGuildParameter[gi(Guild::Paladin)], 19);
    CHECK_EQ(kGuildParameter[gi(Guild::Ninja)], 21);
    CHECK_EQ(kGuildParameter[gi(Guild::Villain)], 21);
    CHECK_EQ(kGuildParameter[gi(Guild::Seeker)], 18);
    CHECK_EQ(kGuildParameter[gi(Guild::Thief)], 15);
    CHECK_EQ(kGuildParameter[gi(Guild::Scavenger)], 18);
    CHECK_EQ(kGuildParameter[gi(Guild::Mage)], 27);
    CHECK_EQ(kGuildParameter[gi(Guild::Sorcerer)], 27);
}

TEST(p2a_guild_gold_factors_match_table) {
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Nomad)], 3);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Warrior)], 1);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Paladin)], 2);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Ninja)], 5);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Villain)], 5);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Seeker)], 3);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Thief)], 9);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Scavenger)], 7);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Mage)], 3);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Sorcerer)], 3);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Wizard)], 4);
    CHECK_EQ(kGuildGoldFactor[gi(Guild::Healer)], 4);
}

TEST(p2a_post_route_order) {
    CHECK_EQ(gi(kPostRouteOrder[0]), gi(Guild::Warrior));
    CHECK_EQ(gi(kPostRouteOrder[1]), gi(Guild::Ninja));
    CHECK_EQ(gi(kPostRouteOrder[2]), gi(Guild::Paladin));
    CHECK_EQ(gi(kPostRouteOrder[3]), gi(Guild::Villain));
    CHECK_EQ(gi(kPostRouteOrder[4]), gi(Guild::Seeker));
}

TEST(p2a_route_target_levels) {
    CHECK_EQ(routeTargetLevel(Guild::Warrior), 28);
    CHECK_EQ(routeTargetLevel(Guild::Nomad), 30);
    CHECK_EQ(routeTargetLevel(Guild::Wizard), 41);
}

TEST(p2a_archive_hits_warrior_and_nomad) {
    // Warrior AH = 6 for levels up to 28.
    CHECK_EQ(archiveHitsForLevel(Guild::Warrior, 10), 6);
    CHECK_EQ(archiveHitsForLevel(Guild::Warrior, 28), 6);
    // Nomad past level 30: 1 HP per level.
    CHECK_EQ(archiveHitsForLevel(Guild::Nomad, 31), 1);
    CHECK_EQ(archiveHitsForLevel(Guild::Nomad, 45), 1);
}

TEST(p2a_archive_hits_other_guild_past_ml) {
    // Wizard ML is 41; other guilds gain 2 HP per level past ML.
    CHECK_EQ(archiveHitsForLevel(Guild::Wizard, 42), 2);
    CHECK_EQ(archiveHitsForLevel(Guild::Wizard, 60), 2);
}

TEST(p2a_bonus_hp_requires_con_17) {
    CHECK(!archiveBonusHpApplies(16, 5, 28));
    CHECK(archiveBonusHpApplies(17, 5, 28));
    CHECK(archiveBonusHpApplies(25, 5, 28));
}

TEST(p2a_bonus_hp_only_below_hits_cap_level) {
    CHECK(archiveBonusHpApplies(17, 27, 28));
    CHECK(!archiveBonusHpApplies(17, 28, 28));
    CHECK(!archiveBonusHpApplies(17, 29, 28));
}

TEST(p2a_race_join_restrictions) {
    // Elves and Osiri cannot be Warriors.
    CHECK(!raceMayJoinGuild(Race::Elf, Guild::Warrior));
    CHECK(!raceMayJoinGuild(Race::Osiri, Guild::Warrior));
    // Giants and Morlochs cannot be Wizards.
    CHECK(!raceMayJoinGuild(Race::Giant, Guild::Wizard));
    CHECK(!raceMayJoinGuild(Race::Morloch, Guild::Wizard));
    // Gnomes cannot be either.
    CHECK(!raceMayJoinGuild(Race::Gnome, Guild::Warrior));
    CHECK(!raceMayJoinGuild(Race::Gnome, Guild::Wizard));
    // Unrestricted pairs.
    CHECK(raceMayJoinGuild(Race::Human, Guild::Warrior));
    CHECK(raceMayJoinGuild(Race::Human, Guild::Wizard));
    CHECK(raceMayJoinGuild(Race::Dwarf, Guild::Warrior));
}

TEST(p2a_gold_round_half_up) {
    CHECK_EQ(goldRoundHalfUp(2.5), 3);
    CHECK_EQ(goldRoundHalfUp(0.5), 1);
    CHECK_EQ(goldRoundHalfUp(3.5), 4);
    CHECK_EQ(goldRoundHalfUp(2.4), 2);
    CHECK_EQ(goldRoundHalfUp(2.6), 3);
    CHECK_EQ(goldRoundHalfUp(10.0), 10);
}

TEST(p2a_party_xp_factor_penalty) {
    // 0.04 penalty per extra member: 1.00, 0.96, 0.92, 0.88.
    CHECK(nearD(partyXpFactor(1), 1.00));
    CHECK(nearD(partyXpFactor(2), 0.96));
    CHECK(nearD(partyXpFactor(3), 0.92));
    CHECK(nearD(partyXpFactor(4), 0.88));
}

TEST(p2a_striker_factor_companion_quarter) {
    CHECK(nearD(xpStrikerFactor(false), 1.0));
    CHECK(nearD(xpStrikerFactor(true), 0.25));
    CHECK(nearD(xpStrikerFactor(true), kCompanionXpFactor));
}

TEST(p2a_first_free_roster_slot) {
    CHECK_EQ(firstFreeRosterSlot(roster({false, true, false})), 0);
    CHECK_EQ(firstFreeRosterSlot(roster({true, true, false, false})), 2);
    CHECK_EQ(firstFreeRosterSlot(roster({true, true, true, false})), 3);
}

TEST(p2a_first_free_roster_slot_reuses_deleted) {
    // Creation order is kept; a deleted character's slot is reused first.
    CHECK_EQ(firstFreeRosterSlot(roster({true, false, true})), 1);
}

TEST(p2a_age_by_adds_days) {
    tsup::Env env({0.99, 0.99});  // high rolls: no complication
    Character ch;
    ch.race = Race::Human;
    ch.ageDays = kAgeStartDays;
    ageBy(env.ctx, ch, kKind25AgeDays);  // +28 days
    CHECK_EQ(ch.ageDays, 5868);
    ageBy(env.ctx, ch, 365);
    CHECK_EQ(ch.ageDays, 6233);
}
