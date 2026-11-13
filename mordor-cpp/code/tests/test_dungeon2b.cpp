#include "test_support.h"
#include "mordor/dungeon2b.h"

#include <cmath>

// Helper for floating-point comparisons in the rescue and disarm formulas.
static bool nearly(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

// R2 / c66: the stated chute float threshold is 7 at level 8.
TEST(chute_float_threshold_at_level_8) {
    CHECK_EQ(mordor::kChuteFloatLevel, 8);
    CHECK_EQ(mordor::kChuteFloatPercent, 7);
    CHECK_EQ(mordor::chuteFloatPercent(8), 7);
}

// c30 / c45: a chute depth of 0 means no floor change.
TEST(chute_landing_depth_zero_keeps_level) {
    CHECK_EQ(mordor::chuteLandingLevel(6, 0), 6);
    CHECK_EQ(mordor::chuteLandingLevel(1, 0), 1);
}

// R7: for pit size 2 the engine comment gives ln(size + 2) = ln(4) = 1.386...,
// which rounds to 1 under every rounding named in the dossier.
TEST(pit_damage_size_two_rounds_to_one) {
    CHECK_EQ(mordor::pitDamage(2), 1);
}

// R31 / c17: base chance is max(16 - L, 0.1). Level 10 gives 6.
TEST(rescue_chance_base_formula) {
    CHECK(nearly(mordor::rescueChance(10, false), 6.0));
}

// R31 / c17: at level 16 the base term is 0, so the 0.1 floor applies.
TEST(rescue_chance_floor_at_level_16) {
    CHECK(nearly(mordor::rescueChance(16, false), 0.1));
    CHECK(nearly(mordor::kRescueMinChance, 0.1));
}

// R31 / c17: beyond level 16 the floor still holds without easy find.
TEST(rescue_chance_floor_above_level_16) {
    CHECK(nearly(mordor::rescueChance(20, false), 0.1));
}

// R31 / c17: easy find adds 16 - L after the floor.
TEST(rescue_chance_easy_find_adds_difference) {
    CHECK(nearly(mordor::rescueChance(10, true), 12.0));
}

// c17 quirk: the easy-find addition is applied after the 0.1 floor, so above
// level 16 the chance goes negative. Level 17: 0.1 + (16 - 17) = -0.9.
// Level 20: 0.1 + (16 - 20) = -3.9.
TEST(rescue_chance_easy_find_goes_negative_above_16) {
    CHECK(nearly(mordor::rescueChance(17, true), -0.9));
    CHECK(nearly(mordor::rescueChance(20, true), -3.9));
    CHECK(mordor::rescueChance(20, true) < 0.0);
}

// R21: a dungeon with fewer than 15 levels is the limited build.
// The full build has at least kFullBuildLevelCount = 15 levels.
TEST(limited_build_boundary_at_fifteen_levels) {
    CHECK_EQ(mordor::kFullBuildLevelCount, 15);
    CHECK(mordor::isLimitedBuild(14));
    CHECK(!mordor::isLimitedBuild(15));
}

// R17 / c35: the limited build clamps the teleport spell level to 3.
// The full build leaves the level unchanged.
TEST(limited_spell_level_clamp) {
    CHECK_EQ(mordor::kLimitedBuildTeleportCap, 3);
    CHECK_EQ(mordor::limitedSpellLevel(false, 10), 10);
    CHECK_EQ(mordor::limitedSpellLevel(true, 10), 3);
    CHECK_EQ(mordor::limitedSpellLevel(true, 2), 2);
}

// R17 / c44: the limited build's teleporter square denies destinations beyond
// its last level. A destination equal to the last level is not beyond it.
TEST(teleporter_square_denies_beyond_last_level_on_limited_build) {
    CHECK(mordor::teleporterSquareDenied(true, 5, 4));
    CHECK(!mordor::teleporterSquareDenied(true, 4, 4));
    CHECK(!mordor::teleporterSquareDenied(true, 3, 4));
}

// R17: the full build does not deny teleporter destinations on this rule.
TEST(teleporter_square_not_denied_on_full_build) {
    CHECK(!mordor::teleporterSquareDenied(false, 5, 4));
}

// R18 / c22: a level is rock when it exceeds MaxDungeonLevel or the dungeon
// level count.
TEST(teleport_level_rock_when_past_either_limit) {
    CHECK(mordor::teleportLevelIsRock(11, 10, 15));  // past max
    CHECK(mordor::teleportLevelIsRock(9, 10, 8));    // past count
    CHECK(!mordor::teleportLevelIsRock(10, 10, 15)); // equal to max
    CHECK(!mordor::teleportLevelIsRock(5, 10, 8));   // within both
}

// R14: the three setters named by both sources (teleporter square, Teleport
// spell, lone chest trap) set the forced flag. The enum has exactly these three.
TEST(forced_flag_set_by_three_setters) {
    CHECK(mordor::forcedFlagSetBy(mordor::ForcedFlagSetter::TeleporterSquare));
    CHECK(mordor::forcedFlagSetBy(mordor::ForcedFlagSetter::TeleportSpell));
    CHECK(mordor::forcedFlagSetBy(mordor::ForcedFlagSetter::LoneChestTrap));
}

// R23 / c44: the preferred square-effect order places the pit before the
// stairs caption, the teleporter after the caption, and the chute last.
TEST(square_effect_order_follows_c44) {
    CHECK_EQ(mordor::kSquareEffectOrder[0], 0);
    CHECK_EQ(mordor::kSquareEffectOrder[5], 5);
    CHECK_EQ(mordor::kSquareEffectOrder[6], 6);
    CHECK_EQ(mordor::kSquareEffectOrder[7], 7);
    CHECK_EQ(mordor::kSquareEffectOrder[8], 8);
}

// Disarm stat: WIS/4 + DEX/2. WIS 20 gives 5, DEX 18 gives 9, total 14.
TEST(trap_disarm_stat_formula) {
    CHECK(nearly(mordor::trapDisarmStat(20, 18), 14.0));
    CHECK(nearly(mordor::trapDisarmStat(4, 2), 2.0));
}

// R33 / c48: dead-body facing is (lead.Facing + 3) & 3.
// Facing 1 gives 0. Facing 2 gives 1.
TEST(dead_body_facing_formula) {
    CHECK_EQ(mordor::deadBodyFacing(1), 0);
    CHECK_EQ(mordor::deadBodyFacing(2), 1);
}

// R32 / c11: only Resurrect undoes death from rock (engine statement).
TEST(rock_undone_only_by_resurrect) {
    CHECK(mordor::rockUndoneBy(mordor::RaiseKind::Resurrect));
    CHECK(!mordor::rockUndoneBy(mordor::RaiseKind::None));
}

// Perception ahead: WIS + StatMods/4 (MAINMOD2 1962, c17/c31).
// WIS 12 plus StatMods 8 / 4 = 12 + 2 = 14.
TEST(perception_ahead_rating_uses_wis_and_statmods) {
    mordor::Character leader;
    leader.abilities.stats[static_cast<mordor::u32>(mordor::StatIndex::Wis)] = 12;
    leader.abilities.statMods[static_cast<mordor::u32>(mordor::StatIndex::Wis)] = 8;
    CHECK_EQ(mordor::perceptionAheadRating(leader), 14);
}

// Constants stated in the header for the rescue and build rules.
TEST(rescue_and_build_constants_match_srd) {
    CHECK_EQ(mordor::kRescueBaseLevel, 16);
    CHECK_EQ(mordor::kShareMonsterItemReach, 16);
    CHECK(nearly(mordor::kRescueCeilingChance, 100.0));
}
