#include "test_support.h"
#include "mordor/casting2.h"

using namespace mordor;

// Spell use classification: Teleport (7) and Cure Paralysis (10) are Either,
// checked on the raw effect before anything else.
TEST(spell_use_teleport_and_cure_paralysis_are_either) {
    CHECK(spellUseClass(7, 5, 0) == SpellUse::Either);
    CHECK(spellUseClass(10, 0, 0) == SpellUse::Either);
    // Either is the first row, so it wins over the Heal category rule.
    CHECK(spellUseClass(7, 0, 9) == SpellUse::Either);
}

TEST(spell_use_noncombat_for_zero_damage_or_heal_otherwise_combat) {
    CHECK(spellUseClass(3, 0, 0) == SpellUse::NonCombat);   // Damage2 == 0
    CHECK(spellUseClass(3, 5, 9) == SpellUse::NonCombat);   // Category 9 (Heal)
    CHECK(spellUseClass(3, 5, 0) == SpellUse::Combat);      // else
    CHECK(spellUseClass(3, 5, 8) == SpellUse::Combat);      // not Heal
}

// SL = floor(guild_level / 2 + 0.5), min 1, max 255.
TEST(spell_level_from_guild_level_table_and_clamps) {
    CHECK_EQ(spellLevelFromGuildLevel(1), 1);
    CHECK_EQ(spellLevelFromGuildLevel(2), 1);
    CHECK_EQ(spellLevelFromGuildLevel(3), 2);
    CHECK_EQ(spellLevelFromGuildLevel(4), 2);
    CHECK_EQ(spellLevelFromGuildLevel(17), 9);
    CHECK_EQ(spellLevelFromGuildLevel(18), 9);
    CHECK_EQ(spellLevelFromGuildLevel(19), 10);
    CHECK_EQ(spellLevelFromGuildLevel(20), 10);
    // Min 1: floor(0 + 0.5) = 0 is raised to 1.
    CHECK_EQ(spellLevelFromGuildLevel(0), 1);
    // Max 255: floor(500.5) = 500 is capped at 255.
    CHECK_EQ(spellLevelFromGuildLevel(1000), 255);
}

// Worked example: Teleport (L12), Wizard Movement rate 1.
// Wizard 22 has SL 11 and cannot cast it; Wizard 23 has SL 12 and can.
TEST(level_reaches_teleport_wizard_22_cannot_23_can) {
    CHECK(!levelReachesSpell(22, 1.0, 12));
    CHECK(levelReachesSpell(23, 1.0, 12));
}

// Guild level must be > 0 and rate must be > 0.
// Here round_even(1 * 0) = 0 <= SL(30) = 15, so only the rate guard refuses.
TEST(level_reaches_requires_positive_level_and_rate) {
    CHECK(!levelReachesSpell(30, 0.0, 1));
    CHECK(!levelReachesSpell(0, 1.0, 1));
}

// Charm of Opening: Thief rate 3 (spell level 3) learnable at guild level 5;
// Nomad rate 12 (spell level 12) learnable at guild level 23.
TEST(level_reaches_charm_of_opening_thief_and_nomad) {
    CHECK(!levelReachesSpell(4, 3.0, 1));   // SL 2 < 3
    CHECK(levelReachesSpell(5, 3.0, 1));    // SL 3 >= 3
    CHECK(!levelReachesSpell(22, 12.0, 1)); // SL 11 < 12
    CHECK(levelReachesSpell(23, 12.0, 1));  // SL 12 >= 12
}

// Unreachable if spell.Level > ceiling.
TEST(spell_unreachable_is_strictly_above_ceiling) {
    CHECK(!spellUnreachable(15, 15));
    CHECK(spellUnreachable(16, 15));
    CHECK(!spellUnreachable(1, 15));
}

// NoSpellsHere = ANTIMAGIC OR EXTINGUISHER. AntiMagicHere = ANTIMAGIC only.
// Town refusal is a plain pass-through of the in-town flag.
TEST(cast_blocked_by_square_town_and_antimagic_quirk) {
    CHECK(!castBlockedBySquare(false, false));
    CHECK(castBlockedBySquare(true, false));
    CHECK(castBlockedBySquare(false, true));
    // An extinguisher blocks casting but is not anti-magic.
    CHECK(!antiMagicHere(false));
    CHECK(antiMagicHere(true));
    CHECK(castBlockedInTown(true));
    CHECK(!castBlockedInTown(false));
}

// Binding: effects 40-42 or 150. Group: AffectGroup >= 6.
TEST(effect_ids_binding_and_group_boundaries) {
    CHECK(isBindingEffect(40));
    CHECK(isBindingEffect(41));
    CHECK(isBindingEffect(42));
    CHECK(isBindingEffect(150));
    CHECK(!isBindingEffect(39));
    CHECK(!isBindingEffect(43));
    CHECK(!isBindingEffect(149));
    CHECK(!isBindingEffect(151));

    CHECK(isGroupSpell(6));
    CHECK(!isGroupSpell(5));
}

// Effect >= 200 is the whole-party variant; real effect = Effect - 200.
TEST(whole_party_variant_and_real_effect) {
    CHECK(isPartyWide(200));
    CHECK(!isPartyWide(199));
    CHECK_EQ(realEffect(207), 7);
    CHECK_EQ(realEffect(7), 7);
}

// Charm: 75-99, index = Effect - 75. Resist: 50-60, index = Effect - 50.
TEST(charm_range_and_resist_index) {
    CHECK(isCharmEffect(75));
    CHECK(isCharmEffect(99));
    CHECK(!isCharmEffect(74));
    CHECK(!isCharmEffect(100));
    CHECK_EQ(charmIndex(75), 0);
    CHECK_EQ(charmIndex(84), 9);   // Charm Undead
    CHECK_EQ(resistIndexForEffect(50), 0);
    CHECK_EQ(resistIndexForEffect(51), 1);
    CHECK_EQ(resistIndexForEffect(60), 10);
}

// gap >= 0 -> 0; gap <= -15 -> 3; gap <= -5 -> 2; otherwise 1.
TEST(charm_identify_level_boundaries) {
    CHECK_EQ(charmIdentifyLevel(0), 0);
    CHECK_EQ(charmIdentifyLevel(5), 0);
    CHECK_EQ(charmIdentifyLevel(-1), 1);
    CHECK_EQ(charmIdentifyLevel(-4), 1);
    CHECK_EQ(charmIdentifyLevel(-5), 2);
    CHECK_EQ(charmIdentifyLevel(-14), 2);
    CHECK_EQ(charmIdentifyLevel(-15), 3);
    CHECK_EQ(charmIdentifyLevel(-40), 3);
}

// Accept rule: Damage2 > 0 and SpellEffect > 0.
TEST(monster_spell_accept_rule) {
    CHECK(monsterSpellAccepted(1, 1));
    CHECK(!monsterSpellAccepted(0, 5));
    CHECK(!monsterSpellAccepted(3, 0));
}

// Base 50%; 80% if top > monLvl * 1.25; 20% if top < monLvl * 0.75.
// With monLvl 8: thresholds are 10 (strict) and 6 (strict).
TEST(breath_chance_thresholds_are_strict) {
    CHECK_EQ(breathChancePercent(10, 8), 50);
    CHECK_EQ(breathChancePercent(11, 8), 80);
    CHECK_EQ(breathChancePercent(6, 8), 50);
    CHECK_EQ(breathChancePercent(5, 8), 20);
}

// With a 20% chance: a roll of 0.1 succeeds, 0.5 fails (clear of boundaries).
TEST(breath_roll_follows_chance) {
    tsup::Env hit({0.1, 0.1});
    CHECK(rollBreath(hit.ctx, 5, 8));
    tsup::Env miss({0.5, 0.5});
    CHECK(!rollBreath(miss.ctx, 5, 8));
}

// Cast sets Int(SL/2)+1; recompute sets Int(GuildLevel/2)+1.
TEST(protection_values_from_cast_and_recompute) {
    CHECK_EQ(protectionFromCast(13), 7);
    CHECK_EQ(protectionFromCast(1), 1);
    CHECK_EQ(protectionRecompute(26), 14);
    CHECK_EQ(protectionRecompute(25), 13);
}

// Documented quirk: the later recompute overwrites the cast value.
TEST(protection_recompute_overwrites_cast_value) {
    Character who;
    who.activeGuild = Guild::Wizard;
    who.topGuildLevel = 26;
    who.guilds[static_cast<u32>(Guild::Wizard)].level = 26;
    tsup::Env env({0.5, 0.9});
    setProtectionOnCast(env.ctx, who, 13);
    CHECK_EQ(who.combat.protection, 7);
    recomputeProtection(env.ctx, who);
    CHECK_EQ(who.combat.protection, 14);
}

// Worked example: Help's tutorial Nomad (INT 12, WIS 12) has 120 max SP.
TEST(max_spell_points_worked_example) {
    CHECK_EQ(maxSpellPoints(12, 12), 120);
}

// Required stats are minimums: StatWithMods(i) >= Required[i].
TEST(meets_required_stats_is_inclusive_minimum) {
    Character who;
    who.abilities.stats[static_cast<u32>(StatIndex::Int)] = 12;
    Arr<i32, kStatCount> req{};
    CHECK(meetsRequiredStats(who, req));                 // all zero: met
    req[static_cast<u32>(StatIndex::Int)] = 12;
    CHECK(meetsRequiredStats(who, req));                 // equal: met
    req[static_cast<u32>(StatIndex::Int)] = 13;
    CHECK(!meetsRequiredStats(who, req));                // one short: refused
}

TEST(consumable_item_types_are_potion_through_cube) {
    CHECK(itemIsConsumable(ItemType::Potion));
    CHECK(itemIsConsumable(ItemType::Scroll));
    CHECK(itemIsConsumable(ItemType::Tome));
    CHECK(itemIsConsumable(ItemType::Dust));
    CHECK(itemIsConsumable(ItemType::Crystal));
    CHECK(itemIsConsumable(ItemType::Rod));
    CHECK(itemIsConsumable(ItemType::Sphere));
    CHECK(itemIsConsumable(ItemType::Cube));
    CHECK(!itemIsConsumable(ItemType::Other));
}

// A consumable is removed when its last charge is spent, not before.
TEST(consumable_removed_only_when_last_charge_spent) {
    tsup::Env env({0.5, 0.9});
    Item it;
    it.type = ItemType::Scroll;
    it.charges = 2;
    SpellChargeResult first = spendItemCharge(env.ctx, it);
    CHECK(!first.removed);
    CHECK_EQ(first.chargesLeft, 1);
    SpellChargeResult second = spendItemCharge(env.ctx, it);
    CHECK(second.removed);
    CHECK_EQ(second.chargesLeft, 0);
}

TEST(charge_spell_points_subtracts_cost) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.vitals.curSpellPoints = 20;
    chargeSpellPoints(env.ctx, who, 8);
    CHECK_EQ(who.vitals.curSpellPoints, 12);
}

// Monster no-cast: flat 10% for any caster; non-mage adds a 50% chance.
// Rolls are kept clear of the thresholds.
TEST(monster_no_cast_thresholds) {
    tsup::Env low({0.05, 0.05});
    CHECK(rollMonsterNoCast(low.ctx, true));
    tsup::Env mid({0.5, 0.5});
    CHECK(!rollMonsterNoCast(mid.ctx, true));
    tsup::Env nonMageHit({0.3, 0.3});
    CHECK(rollMonsterNoCast(nonMageHit.ctx, false));
    tsup::Env nonMageMiss({0.7, 0.7});
    CHECK(!rollMonsterNoCast(nonMageMiss.ctx, false));
}

// Standing order out of SP: the order becomes Fight.
TEST(standing_order_out_of_sp_becomes_fight) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.order.kind = CombatOrderKind::Buffer;
    dropOrderOutOfSp(env.ctx, who);
    CHECK(who.order.kind == CombatOrderKind::Fight);
}
