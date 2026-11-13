#include "test_support.h"
#include "mordor/casting2.h"
#include "mordor/casting3.h"

#include <cmath>

// Helpers (file scope, unique names to avoid clashes with the harness).

static bool c3_near(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

// Sets stats STR, INT, WIS and gives every stat a max of 18 so that no
// clamping against a zero max can alter the values under test.
static void c3_setStats(mordor::Character& c, int str, int intel, int wis) {
    for (unsigned k = 0; k < 6; ++k) {
        c.abilities.statMax[k] = 18;
    }
    c.abilities.stats[0] = str;   // StatIndex::Str
    c.abilities.stats[1] = intel; // StatIndex::Int
    c.abilities.stats[2] = wis;   // StatIndex::Wis
}

// ---- Spell cost: published and engine whole-number rules (R10) ----

TEST(casting3_published_cost_floors_with_epsilon) {
    // floor(209 - 0.0001) = 208: the documented 209 -> 208 quirk.
    CHECK_EQ(mordor::costFromProductPublished(209.0), 208);
    // floor(12.5 - 0.0001) = 12.
    CHECK_EQ(mordor::costFromProductPublished(12.5), 12);
    // floor(50.00005 - 0.0001) = floor(49.99995) = 49.
    CHECK_EQ(mordor::costFromProductPublished(50.00005), 49);
}

TEST(casting3_engine_cost_subtracts_one_when_whole) {
    // Whole product 209: floor 209, then price -= 1 -> 208.
    CHECK_EQ(mordor::costFromProductEngine(209.0), 208);
    // Non-whole product 208.5: floor only -> 208.
    CHECK_EQ(mordor::costFromProductEngine(208.5), 208);
    // Non-whole product 12.3: floor only -> 12.
    CHECK_EQ(mordor::costFromProductEngine(12.3), 12);
}

TEST(casting3_published_and_engine_agree_on_fractions) {
    CHECK_EQ(mordor::costFromProductPublished(12.3), mordor::costFromProductEngine(12.3));
    CHECK_EQ(mordor::costFromProductPublished(208.5), mordor::costFromProductEngine(208.5));
}

// ---- Cost minimum clamp (R8, engine rule) ----

TEST(casting3_cost_at_least_minimum_clamps_up) {
    CHECK_EQ(mordor::costAtLeastMinimum(5, 10), 10);
    CHECK_EQ(mordor::costAtLeastMinimum(15, 10), 15);
    CHECK_EQ(mordor::costAtLeastMinimum(10, 10), 10);
}

// ---- Archive cost product: StartSpellLevel * SpellPointRatio * BaseMod^2 * 1.9 / SL (R7) ----

TEST(casting3_archive_product_squares_base_mod) {
    // 2 * 3 * 2^2 * 1.9 / 4 = 24 * 1.9 / 4 = 11.4
    CHECK(c3_near(mordor::costProductArchive(2, 3, 2, 4), 11.4));
    // Doubling BaseMod quadruples the product: 2 * 3 * 4^2 * 1.9 / 4 = 45.6
    CHECK(c3_near(mordor::costProductArchive(2, 3, 4, 4), 45.6));
}

TEST(casting3_cost_factor_constant) {
    CHECK(c3_near(mordor::kCostFactor, 1.9));
}

// ---- Spell level from guild level (R37: ceil(L/2) equivalent forms) ----

TEST(casting3_spell_level_from_guild_level_boundaries) {
    CHECK_EQ(mordor::spellLevelFromGuildLevel(1), 1);
    CHECK_EQ(mordor::spellLevelFromGuildLevel(2), 1);
    CHECK_EQ(mordor::spellLevelFromGuildLevel(3), 2);
    CHECK_EQ(mordor::spellLevelFromGuildLevel(10), 5);
    CHECK_EQ(mordor::spellLevelFromGuildLevel(11), 6);
}

// ---- Guild level equivalent: Spell level * 2 - 1 (R9 stated formula) ----

TEST(casting3_guild_level_equivalent_is_two_n_minus_one) {
    CHECK_EQ(mordor::guildLevelEquivalent(1), 1);
    CHECK_EQ(mordor::guildLevelEquivalent(3), 5);
    CHECK_EQ(mordor::guildLevelEquivalent(10), 19);
}

// ---- Max spell points: (INT + WIS) * 5 (R33) ----

TEST(casting3_max_spell_points_is_int_plus_wis_times_five) {
    mordor::Character c;
    c3_setStats(c, 0, 10, 6);
    CHECK_EQ(mordor::maxSpellPoints(c), 80);
    CHECK_EQ(mordor::kMaxSpellPointMultiplier, 5);
}

TEST(casting3_max_spell_points_zero_stats) {
    mordor::Character c;
    c3_setStats(c, 0, 0, 0);
    CHECK_EQ(mordor::maxSpellPoints(c), 0);
}

// ---- Monster low-level formula: VbInt(L - L/3.0) (R13 engine) ----

TEST(casting3_monster_low_level_engine_values) {
    CHECK_EQ(mordor::monsterLowLevelEngine(1), 0);   // 1 - 0.333 = 0.667 -> 0
    CHECK_EQ(mordor::monsterLowLevelEngine(3), 2);   // 3 - 1 = 2
    CHECK_EQ(mordor::monsterLowLevelEngine(9), 6);   // 9 - 3 = 6
    CHECK_EQ(mordor::monsterLowLevelEngine(10), 6);  // 10 - 3.333 = 6.667 -> 6
}

// ---- Cast gate rounding: half-to-even (R32) ----

TEST(casting3_cast_gate_rounds_half_to_even) {
    CHECK_EQ(mordor::castGateRound(2.5), 2);
    CHECK_EQ(mordor::castGateRound(3.5), 4);
    CHECK_EQ(mordor::castGateRound(1.5), 2);
    CHECK_EQ(mordor::castGateRound(2.4), 2);
    CHECK_EQ(mordor::castGateRound(2.6), 3);
}

// ---- Charm refused with no free companion slot (R30, c6 rule) ----

TEST(casting3_charm_refused_only_when_no_slot_and_kill) {
    CHECK(mordor::charmRefusedNoSlot(0, true));
    CHECK(!mordor::charmRefusedNoSlot(0, false));
    CHECK(!mordor::charmRefusedNoSlot(3, true));
    CHECK(!mordor::charmRefusedNoSlot(2, false));
}

// ---- Combat state gating (R14, R15, engine rules) ----

TEST(casting3_combat_spell_needs_monsters_at_low_level) {
    // No monsters: Combat spell refused.
    CHECK(!mordor::castAllowedByCombatState(mordor::CastScope::Combat, 3, false, false));
    // Monsters present but not engaged, level 3 (<= 7): allowed.
    CHECK(mordor::castAllowedByCombatState(mordor::CastScope::Combat, 3, true, false));
}

TEST(casting3_peaced_limit_boundary_at_level_seven_and_eight) {
    // kPeacedCastLevelLimit = 7: level 7 allowed while peaced...
    CHECK_EQ(mordor::kPeacedCastLevelLimit, 7);
    CHECK(mordor::castAllowedByCombatState(mordor::CastScope::Combat, 7, true, false));
    // ...level 8 refused while peaced (not engaged)...
    CHECK(!mordor::castAllowedByCombatState(mordor::CastScope::Combat, 8, true, false));
    // ...but allowed once engaged.
    CHECK(mordor::castAllowedByCombatState(mordor::CastScope::Combat, 8, true, true));
}

TEST(casting3_noncombat_refused_only_when_engaged) {
    CHECK(!mordor::castAllowedByCombatState(mordor::CastScope::NonCombat, 3, true, true));
    CHECK(mordor::castAllowedByCombatState(mordor::CastScope::NonCombat, 3, true, false));
    CHECK(mordor::castAllowedByCombatState(mordor::CastScope::NonCombat, 3, false, false));
    // The peaced level limit does not apply to NonCombat spells.
    CHECK(mordor::castAllowedByCombatState(mordor::CastScope::NonCombat, 9, true, false));
}

// ---- Monsters present (engine count rule, R16) ----

TEST(casting3_monsters_present_engine_counts) {
    CHECK(!mordor::monstersPresentEngine(0));
    CHECK(mordor::monstersPresentEngine(1));
    CHECK(mordor::monstersPresentEngine(4));
}

// ---- Protection base for Sorceror and Wizard (R18, manual and archive) ----

TEST(casting3_protection_base_for_sorceror_and_wizard) {
    CHECK_EQ(mordor::protectionBaseForGuild(mordor::Guild::Sorcerer), 3);
    CHECK_EQ(mordor::protectionBaseForGuild(mordor::Guild::Wizard), 3);
    CHECK_EQ(mordor::kProtectionBaseSorWiz, 3);
}

// ---- Stat minimums for item-spell gating (R25, engine rule) ----

TEST(casting3_stat_minimums_compare_each_stat) {
    mordor::Character c;
    c3_setStats(c, 0, 10, 0);

    auto req = c.abilities.stats;  // Arr copy, same type as the parameter
    for (unsigned k = 0; k < 6; ++k) {
        req[k] = 0;
    }
    req[1] = 10;  // INT exactly meets the minimum
    CHECK(mordor::meetsRequiredStats(c, req));

    req[1] = 11;  // INT one short
    CHECK(!mordor::meetsRequiredStats(c, req));
}

// ---- Item spell level (R6 engine: negative item level uses caster guild SL) ----

TEST(casting3_negative_item_level_uses_caster_guild_spell_level) {
    CHECK_EQ(mordor::itemSpellLevel(-1, 5), 5);
    CHECK_EQ(mordor::itemSpellLevel(-3, 9), 9);
}

// ---- Level-0 item spell level (R5 / c6 engine, cast path) ----

TEST(casting3_zero_level_item_spell_level_free_uses_one) {
    CHECK_EQ(mordor::zeroLevelItemSpellLevel(true, 6), 1);
    CHECK_EQ(mordor::zeroLevelItemSpellLevel(false, 6), 6);
    CHECK_EQ(mordor::zeroLevelItemSpellLevel(false, 2), 2);
}

// ---- Level-0 item SP charge (R4 formula: charge only when itemLevel == 0) ----

TEST(casting3_level_zero_item_cast_charges_sp) {
    tsup::Env env({0.5, 0.9});
    mordor::Character who;
    who.vitals.curSpellPoints = 20;
    mordor::chargeLevelZeroItemCast(env.ctx, who, 0, 7);
    CHECK_EQ(who.vitals.curSpellPoints, 13);
}

TEST(casting3_nonzero_item_cast_charges_no_sp) {
    tsup::Env env({0.5, 0.9});
    mordor::Character who;
    who.vitals.curSpellPoints = 20;
    mordor::chargeLevelZeroItemCast(env.ctx, who, 2, 7);
    CHECK_EQ(who.vitals.curSpellPoints, 20);
}

// ---- Life Domination bind effect of 150 sets ForLife (R29, engine) ----

TEST(casting3_bind_effect_150_sets_for_life) {
    tsup::Env env({0.5, 0.9});
    mordor::Companion comp;
    CHECK_EQ(mordor::kBindLifeDomination, 150);
    CHECK(!comp.forLife);
    mordor::applyBindEffect(env.ctx, comp, 150);
    CHECK(comp.forLife);
}

// ---- Constants used by the cast rules ----

TEST(casting3_constants_match_source_values) {
    CHECK_EQ(mordor::kSpellLevelCap, 255);
    CHECK_EQ(mordor::kBindArchiveBinding, 100);
    CHECK_EQ(mordor::kFireboltBaseLevel, 1);
    CHECK_EQ(mordor::kBlueFlameBaseLevel, 3);
    CHECK_EQ(mordor::kCauseWoundsBaseLevel, 1);
}

// ---- Documented quirk: the published formula can drop a whole product by one ----

TEST(casting3_whole_product_quirk_documented_in_both_paths) {
    // Both helpers yield one less than the whole product for 209 (R10).
    CHECK_EQ(mordor::costFromProductPublished(209.0), 208);
    CHECK_EQ(mordor::costFromProductEngine(209.0), 208);
}
