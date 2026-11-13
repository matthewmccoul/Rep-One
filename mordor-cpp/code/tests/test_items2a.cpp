#include "test_support.h"
#include "mordor/attributes.h"
#include "mordor/items2a.h"

#include <cmath>

using namespace mordor;

TEST(pack_slot_numbering_maps_player_slots) {
    CHECK_EQ(packIndexFromPlayerSlot(1), 0);
    CHECK_EQ(packIndexFromPlayerSlot(40), 39);
    CHECK_EQ(playerSlotFromPackIndex(0), 1);
    CHECK_EQ(playerSlotFromPackIndex(39), 40);
}

TEST(usable_pack_index_range_is_1_to_39) {
    CHECK(!isUsablePackIndex(0));
    CHECK(isUsablePackIndex(1));
    CHECK(isUsablePackIndex(39));
    CHECK(!isUsablePackIndex(40));
}

TEST(hands_type_and_type0_equip_quirk) {
    CHECK(typeIsHands(0));
    CHECK(!typeIsHands(1));
    // Quirk: type 0 is absent from the equip-required list.
    CHECK(!typeNeedsEquipToUse(0));
    CHECK(typeNeedsEquipToUse(1));
    CHECK(typeNeedsEquipToUse(22));
    CHECK(!typeNeedsEquipToUse(23));
}

TEST(equip_required_and_consumed_groups) {
    CHECK(typeNeedsEquipToUse(29));
    CHECK(typeNeedsEquipToUse(32));
    CHECK(typeNeedsEquipToUse(34));
    CHECK(!typeNeedsEquipToUse(30));
    CHECK(!typeNeedsEquipToUse(31));

    CHECK(typeConsumedWhenEmpty(23));
    CHECK(typeConsumedWhenEmpty(28));
    CHECK(typeConsumedWhenEmpty(30));
    CHECK(typeConsumedWhenEmpty(31));
    CHECK(!typeConsumedWhenEmpty(22));
    CHECK(!typeConsumedWhenEmpty(29));
    CHECK(!typeConsumedWhenEmpty(32));
}

TEST(wear_class_caps_and_no_cap_minus_one) {
    // Cap 1: one item per class.
    CHECK(!wearClassFull(1, 0));
    CHECK(wearClassFull(1, 1));
    // Cap 2.
    CHECK(!wearClassFull(2, 1));
    CHECK(wearClassFull(2, 2));
    // Cap -1: no cap, so a full class never reports full.
    CHECK(!wearClassFull(-1, 0));
    CHECK(!wearClassFull(-1, 7));
}

TEST(special_stat_mods_and_spell_points) {
    CHECK(specialAppliesStatMods(1));
    CHECK(specialAppliesStatMods(13));
    CHECK(specialAppliesStatMods(14));
    CHECK(!specialAppliesStatMods(11));
    CHECK(!specialAppliesStatMods(12));
    CHECK(!specialAppliesStatMods(0));

    CHECK_EQ(specialSpellPointGain(11), 50);
    CHECK_EQ(specialSpellPointGain(12), 200);
    CHECK_EQ(specialSpellPointGain(1), 0);
    CHECK_EQ(specialSpellPointGain(13), 0);
}

TEST(special_age_days_signed_deltas) {
    CHECK_EQ(specialAgeDays(13), -365);
    CHECK_EQ(specialAgeDays(14), -3650);
    CHECK_EQ(specialAgeDays(1), 0);
    CHECK_EQ(specialAgeDays(11), 0);
}

TEST(guild_crest_types_500_to_511) {
    CHECK(isGuildCrestType(500));
    CHECK(isGuildCrestType(511));
    CHECK(!isGuildCrestType(499));
    CHECK(!isGuildCrestType(512));
    CHECK_EQ(guildCrestType(0), 500);
    CHECK_EQ(guildCrestType(11), 511);
}

TEST(fit_counts_effects_only_for_full_and_unaligned_is_fine) {
    CHECK(fitCountsEffects(FitClass::Full));
    CHECK(!fitCountsEffects(FitClass::Half));
    CHECK(!fitCountsEffects(FitClass::Subtract));
    // Unaligned items are always a fine fit.
    CHECK(alignFit(Align::Neutral, ItemAlign::Unaligned) == FitClass::Full);
}

TEST(attack_defence_fit_rounds_half_to_even) {
    CHECK_EQ(adjustAdForFit(FitClass::Full, 7), 7);
    CHECK_EQ(adjustAdForFit(FitClass::Half, 5), 2);   // 2.5 -> 2 (even)
    CHECK_EQ(adjustAdForFit(FitClass::Half, 7), 4);   // 3.5 -> 4 (even)
    CHECK_EQ(adjustAdForFit(FitClass::Half, 4), 2);
    CHECK_EQ(adjustAdForFit(FitClass::Subtract, 6), -6);
}

TEST(stat_mod_fit_floors_halves) {
    CHECK_EQ(statModWorth(9, FitClass::Full), 9);
    CHECK_EQ(statModWorth(5, FitClass::Half), 2);  // floor(2.5)
    CHECK_EQ(statModWorth(7, FitClass::Half), 3);  // floor(3.5)
    CHECK_EQ(statModWorth(4, FitClass::Subtract), -4);
}

TEST(id_level_from_original_counts_down) {
    CHECK(idLevelFromOriginal(0) == IdLevel::All);
    CHECK(idLevelFromOriginal(1) == IdLevel::Mostly);
    CHECK(idLevelFromOriginal(2) == IdLevel::Partial);
    CHECK(idLevelFromOriginal(3) == IdLevel::Barely);
}

TEST(item_use_factor_nomad_and_healer) {
    CHECK(std::fabs(itemUseFactorFor(Guild::Nomad) - 0.8) < 1e-9);
    CHECK(std::fabs(itemUseFactorFor(Guild::Healer) - 1.5) < 1e-9);
}

TEST(guild_use_min_level_has_floor_of_one) {
    // Engine formula: max(1, (f + f * 2.97) * levelScale), unrounded.
    CHECK(std::fabs(guildUseMinLevel(0.8, 0) - 1.0) < 1e-9);
    CHECK(std::fabs(guildUseMinLevel(1.5, 10) - 59.55) < 1e-6);
}

TEST(clamp_item_chance_per_kind) {
    // Identify 10..90
    CHECK_EQ(clampItemChance(ItemChance::Identify, 5), 10);
    CHECK_EQ(clampItemChance(ItemChance::Identify, 95), 90);
    CHECK_EQ(clampItemChance(ItemChance::Identify, 50), 50);
    // Recognise 5..80
    CHECK_EQ(clampItemChance(ItemChance::Recognise, 3), 5);
    CHECK_EQ(clampItemChance(ItemChance::Recognise, 85), 80);
    // Rename 10..90
    CHECK_EQ(clampItemChance(ItemChance::Rename, 10), 10);
    // Destroy 25..90
    CHECK_EQ(clampItemChance(ItemChance::Destroy, 20), 25);
    CHECK_EQ(clampItemChance(ItemChance::Destroy, 90), 90);
}

TEST(slime_threshold_boundary_dungeon_level_0) {
    // roll = range(1,100): Rnd 0.485 -> 49 (< 50, takes effect).
    tsup::Env envYes({0.485, 0.9});
    CHECK(slimeTakesEffect(envYes.ctx, 0));
    // Rnd 0.495 -> 50 (not < 50, no effect).
    tsup::Env envNo({0.495, 0.9});
    CHECK(!slimeTakesEffect(envNo.ctx, 0));
}

TEST(slime_threshold_rises_two_per_level) {
    // dungeonLevel 10 -> threshold 70.
    tsup::Env envYes({0.685, 0.9});  // roll 69 < 70
    CHECK(slimeTakesEffect(envYes.ctx, 10));
    tsup::Env envNo({0.695, 0.9});   // roll 70, not < 70
    CHECK(!slimeTakesEffect(envNo.ctx, 10));
}

#include "mordor/items3a.h"

TEST(theft_slot_spans_2_to_40) {
    tsup::Env envLow({0.0, 0.9});
    CHECK_EQ(drawTheftSlot(envLow.ctx), 2);
    tsup::Env envMid({0.5, 0.9});       // Int(19.5) + 2 = 21
    CHECK_EQ(drawTheftSlot(envMid.ctx), 21);
    tsup::Env envHigh({0.999, 0.9});    // Int(38.961) + 2 = 40
    CHECK_EQ(drawTheftSlot(envHigh.ctx), 40);
}

TEST(seer_band_and_carrier_count) {
    tsup::Env env0({0.0, 0.9});
    CHECK_EQ(rollSeerBand(env0.ctx), 0);
    tsup::Env env1({0.34, 0.9});        // Int(1.02) = 1
    CHECK_EQ(rollSeerBand(env1.ctx), 1);
    tsup::Env env2({0.7, 0.9});         // Int(2.1) = 2
    CHECK_EQ(rollSeerBand(env2.ctx), 2);

    CHECK_EQ(seerCarrierCount(0), 1);
    CHECK_EQ(seerCarrierCount(-4), 1);
    CHECK_EQ(seerCarrierCount(2), 2);
    CHECK_EQ(seerCarrierCount(3), 3);
    CHECK_EQ(seerCarrierCount(7), 3);
}

TEST(asmodeus_band_is_three_levels) {
    // itemDropLevel 10: band is 8..10.
    CHECK(inAsmodeusBand(10, 8));
    CHECK(inAsmodeusBand(10, 10));
    CHECK(!inAsmodeusBand(10, 7));
    CHECK(!inAsmodeusBand(10, 11));
}

TEST(potion_stat_clamp_floor_and_silent_ceiling) {
    // raceMin 3, raceMax 18, ceiling = raceMax + 5 = 23.
    CHECK_EQ(clampStatForPotion(1, 3, 18), 3);
    CHECK_EQ(clampStatForPotion(10, 3, 18), 10);
    CHECK_EQ(clampStatForPotion(23, 3, 18), 23);
    CHECK_EQ(clampStatForPotion(30, 3, 18), 23);
}

TEST(buffer_classification_and_spell_mapping) {
    CHECK(classifyBuffer(-1) == BufferKind::Empty);
    CHECK(classifyBuffer(0) == BufferKind::ForItem);
    CHECK(classifyBuffer(39) == BufferKind::ForItem);
    CHECK(classifyBuffer(40) == BufferKind::ForSpell);
    CHECK(classifyBuffer(45) == BufferKind::ForSpell);

    CHECK_EQ(bufferForSpell(0), 40);
    CHECK_EQ(bufferForSpell(7), 47);
    CHECK_EQ(spellIndexFromBuffer(47), 7);
}

TEST(realign_full_spectrum_when_index_gap_exceeds_one) {
    CHECK(!realignIsFullSpectrum(0, 1));
    CHECK(!realignIsFullSpectrum(1, 0));
    CHECK(!realignIsFullSpectrum(1, 1));
    CHECK(realignIsFullSpectrum(0, 2));
    CHECK(realignIsFullSpectrum(2, 0));
}

TEST(realign_cost_is_0_72_times_price_doubled_when_full) {
    CHECK_EQ(realignCost(100, false), 72);
    CHECK_EQ(realignCost(100, true), 144);
    CHECK_EQ(realignCost(50, false), 36);
    CHECK_EQ(realignCost(1, false), 1);   // 0.72 -> 1
    CHECK_EQ(realignCost(3, true), 4);    // 4.32 -> 4
}
