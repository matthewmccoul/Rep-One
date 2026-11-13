#include "test_support.h"
#include "mordor/status2.h"

using namespace mordor;
using tsup::Env;

namespace {

// Arr<> indices are unsigned in this codebase (see Character::activeLevel).
u32 ix(i32 v) {
    return static_cast<u32>(v);
}

i32 mag(i32 v) {
    return v < 0 ? -v : v;
}

}  // namespace

TEST(status2_constants_match_srd) {
    CHECK_EQ(kAilmentTickMs, 4000);
    CHECK_EQ(kPoisonCureRollAbove, 185);
    CHECK_EQ(kDiseaseCureRollAbove, 195);
    CHECK_EQ(kCureRollSides, 200);
    CHECK_EQ(kParalysisChanceCap, 95);
    CHECK_EQ(kStoneChanceCap, 95);
    CHECK_EQ(kTickSaveMin, 5);
    CHECK_EQ(kTickSaveMax, 90);
    CHECK_EQ(kMaxAgingDays, 1825);
    CHECK_EQ(kAgingDaysPerYear, 365);
    CHECK_EQ(kPoisonHitsPerFail, 2);
    CHECK_EQ(kDiseaseHitsPerFail, 1);
    CHECK_EQ(kBreathHighPercent, 80);
    CHECK_EQ(kSpitLowPercent, 10);
    CHECK(kBandHighRatio == 1.25);
    CHECK(kBandLowRatio == 0.75);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Poison), 0);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Disease), 1);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Paralysis), 2);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Protected), 6);
}

TEST(status2_paralysis_chance_is_resist_plus_con_capped_at_95) {
    CHECK_EQ(paralysisChance(30, 20), 50);
    CHECK_EQ(paralysisChance(75, 20), 95);
    CHECK_EQ(paralysisChance(76, 20), 95);
    CHECK_EQ(paralysisChance(90, 20), 95);
}

TEST(status2_tick_save_clamps_to_5_through_90) {
    CHECK_EQ(tickSaveClamp(0), 5);
    CHECK_EQ(tickSaveClamp(5), 5);
    CHECK_EQ(tickSaveClamp(50), 50);
    CHECK_EQ(tickSaveClamp(90), 90);
    CHECK_EQ(tickSaveClamp(120), 90);
}

TEST(status2_cure_roll_bonus_is_con_term_minus_level_term) {
    // (effCON-10)*2 - ailmentLevel*2
    CHECK_EQ(cureRollBonus(18, 0), 16);
    CHECK_EQ(cureRollBonus(18, 3), 10);
    CHECK_EQ(cureRollBonus(10, 0), 0);
    CHECK_EQ(cureRollBonus(6, 5), -18);
}

TEST(status2_poison_cure_requires_roll_strictly_above_185) {
    // CON 18, poison level 0: bonus 16.
    // Int(0.8475*200)=169 -> roll 185, not above 185.
    // Int(0.8525*200)=170 -> roll 186, cured.
    Env low({0.8475, 0.8475});
    CHECK(!rollPoisonCure(low.ctx, 18, 0));
    Env high({0.8525, 0.8525});
    CHECK(rollPoisonCure(high.ctx, 18, 0));
}

TEST(status2_disease_cure_requires_roll_strictly_above_195) {
    // CON 18, poison level 0: bonus 16.
    // Int(0.8975*200)=179 -> roll 195, not above 195.
    // Int(0.9025*200)=180 -> roll 196, cured.
    Env low({0.8975, 0.8975});
    CHECK(!rollDiseaseCure(low.ctx, 18, 0));
    Env high({0.9025, 0.9025});
    CHECK(rollDiseaseCure(high.ctx, 18, 0));
}

TEST(status2_disease_cure_reads_poison_level_not_disease_level) {
    // CON 18 with poison level 5: bonus 16 - 10 = 6.
    // Int(0.9475*200)=189 -> roll 195, not cured.
    // Int(0.9525*200)=190 -> roll 196, cured.
    Env low({0.9475, 0.9475});
    CHECK(!rollDiseaseCure(low.ctx, 18, 5));
    Env high({0.9525, 0.9525});
    CHECK(rollDiseaseCure(high.ctx, 18, 5));
}

TEST(status2_low_con_blocks_cures_as_documented) {
    // Max draw is 199.
    // Poison at CON 6, poison level 5: bonus -18, max roll 181, never cured.
    // Disease at CON 6, poison level 0: bonus -8, max roll 191, never cured.
    // Poison at CON 6, poison level 0: bonus -8, max roll 191, cured.
    Env env({0.995, 0.995});
    CHECK(!rollPoisonCure(env.ctx, 6, 5));
    CHECK(!rollDiseaseCure(env.ctx, 6, 0));
    CHECK(rollPoisonCure(env.ctx, 6, 0));
}

TEST(status2_try_poison_threshold_and_existing_poison_gate) {
    // Poison roll is Rnd(100)+1 and must exceed resist index 5 (value 20).
    // Int(0.195*100)=19 -> 20, not > 20: no poison.
    // Int(0.205*100)=20 -> 21, > 20: poisoned at levelFound.
    Character low;
    low.resistances.resist[ix(kResistPoison)] = 20;
    Env lowEnv({0.195, 0.195});
    tryPoison(lowEnv.ctx, low, 6);
    CHECK_EQ(low.ailments.poisonLevel, 0);

    Character high;
    high.resistances.resist[ix(kResistPoison)] = 20;
    Env highEnv({0.205, 0.205});
    tryPoison(highEnv.ctx, high, 6);
    CHECK_EQ(high.ailments.poisonLevel, 6);

    // Already poisoned: the PoisonLevel == 0 gate blocks a new poisoning.
    Character gated;
    gated.resistances.resist[ix(kResistPoison)] = 0;
    gated.ailments.poisonLevel = 2;
    Env gateEnv({0.999, 0.999});
    tryPoison(gateEnv.ctx, gated, 6);
    CHECK_EQ(gated.ailments.poisonLevel, 2);
}

TEST(status2_try_disease_threshold) {
    // Disease roll Rnd(100)+1 must exceed resist index 4 (value 20).
    Character low;
    low.resistances.resist[ix(kResistDisease)] = 20;
    Env lowEnv({0.195, 0.195});
    tryDisease(lowEnv.ctx, low, 4);
    CHECK_EQ(low.ailments.diseaseLevel, 0);

    Character high;
    high.resistances.resist[ix(kResistDisease)] = 20;
    Env highEnv({0.205, 0.205});
    tryDisease(highEnv.ctx, high, 4);
    CHECK_EQ(high.ailments.diseaseLevel, 4);
}

TEST(status2_try_paralysis_threshold_at_resist_plus_con) {
    // Resist index 8 = 10, effCon = 20, chance = min(30, 95) = 30.
    // Int(0.295*100)=29 -> 30, not > 30: no paralysis.
    // Int(0.305*100)=30 -> 31, > 30: paralysed.
    Character low;
    low.resistances.resist[ix(kResistParalysis)] = 10;
    Env lowEnv({0.295, 0.295});
    tryParalysis(lowEnv.ctx, low, 5, 20);
    CHECK(!low.ailments.paralysed);

    Character high;
    high.resistances.resist[ix(kResistParalysis)] = 10;
    Env highEnv({0.305, 0.305});
    tryParalysis(highEnv.ctx, high, 5, 20);
    CHECK(high.ailments.paralysed);
}

TEST(status2_try_paralysis_does_not_reapply_while_paralyzed) {
    // ParalyzedRounds != 0 blocks a new paralysis even on a winning roll.
    Character c;
    c.resistances.resist[ix(kResistParalysis)] = 10;
    c.ailments.paralyzedRounds = 3;
    Env env({0.999, 0.999});
    tryParalysis(env.ctx, c, 5, 20);
    CHECK(!c.ailments.paralysed);
}

TEST(status2_paralysed_characters_cannot_climb_stairs_but_can_descend) {
    // Documented quirk: barred going up, not going down.
    Character c;
    c.ailments.paralysed = true;
    c.ailments.paralysisLevel = 2;
    c.ailments.paralyzedRounds = 2;
    CHECK(!canTakeStairs(c, true));
    CHECK(canTakeStairs(c, false));

    Character free;
    CHECK(canTakeStairs(free, true));
    CHECK(canTakeStairs(free, false));
}

TEST(status2_paralysis_wears_off_when_nothing_is_on_square) {
    // Wear-off always ends paralysis when no monsters are present.
    Character c;
    c.ailments.paralysed = true;
    c.ailments.paralyzedRounds = 2;
    Env env({0.5, 0.5});
    paralysisWearOff(env.ctx, c, 0, false);
    CHECK(!c.ailments.paralysed);
}

TEST(status2_has_special_bit_reads_flag_positions) {
    // Drain is bit 7, Destruction is bit 12.
    u32 flags = (1u << 7) | (1u << 12);
    CHECK(hasSpecialBit(flags, SpecialBit::Drain));
    CHECK(hasSpecialBit(flags, SpecialBit::Destruction));
    CHECK(!hasSpecialBit(flags, SpecialBit::Poison));
    CHECK(!hasSpecialBit(flags, SpecialBit::Theft));
}

TEST(status2_breath_and_spit_bands_and_resist_index) {
    // Victim level vs 1.25*M and 0.75*M, with M = 8.
    // 11 > 10 (high), 8 is mid, 5 < 6 (low). Values avoid the band edges.
    CHECK_EQ(breathPercent(11, 8), 80);
    CHECK_EQ(breathPercent(8, 8), 50);
    CHECK_EQ(breathPercent(5, 8), 20);
    CHECK_EQ(spitPercent(11, 8), 90);
    CHECK_EQ(spitPercent(8, 8), 50);
    CHECK_EQ(spitPercent(5, 8), 10);
    CHECK_EQ(breathResistIndex(BreathType::Fire), 0);
    CHECK_EQ(breathResistIndex(BreathType::Cold), 1);
}

TEST(status2_death_spell_names_map_to_effects) {
    for (const char* name : kKilledSpellNames) {
        CHECK(deathSpellEffectByName(Str(name)) == DeathSpellEffect::Killed);
    }
    CHECK(deathSpellEffectByName(Str("Draining Touch")) == DeathSpellEffect::Drained);
}

TEST(status2_drain_pick_table_never_reaches_strength) {
    // Draw 1..5 drain stats 1..5 by one point; draw 6 is MaxHits -2.
    StatIndex expect[5] = {StatIndex::Int, StatIndex::Wis, StatIndex::Con,
                           StatIndex::Cha, StatIndex::Dex};
    for (i32 d = 1; d <= 5; ++d) {
        DrainPick p = drainPickFromDraw(d);
        CHECK(!p.hitsDrain);
        CHECK(p.stat == expect[d - 1]);
        CHECK(p.stat != StatIndex::Str);
        CHECK_EQ(mag(p.amount), 1);
    }
    DrainPick hits = drainPickFromDraw(6);
    CHECK(hits.hitsDrain);
    CHECK_EQ(mag(hits.amount), 2);
}

TEST(status2_roll_drain_uses_int_rnd_times_6_plus_1) {
    // Rnd 0.5 -> Int(4.0) = 4 -> Charisma (stat index 4).
    // Rnd 0.999 -> Int(6.994) = 6 -> MaxHits -2.
    Env mid({0.5, 0.5});
    DrainPick p = rollDrain(mid.ctx);
    CHECK(!p.hitsDrain);
    CHECK(p.stat == StatIndex::Cha);

    Env top({0.999, 0.999});
    CHECK(rollDrain(top.ctx).hitsDrain);
}

TEST(status2_chest_fate_rerolls_hits_and_sp_in_1_to_max) {
    // Same draw for both rolls so the result does not depend on draw order.
    // Rnd 0.0 -> minimum 1. Rnd 0.999 -> maximum (20 HP, 10 SP).
    Character low;
    low.vitals.maxHits = 20;
    low.vitals.maxSpellPoints = 10;
    Env lowEnv({0.0, 0.0});
    chestFate(lowEnv.ctx, low);
    CHECK_EQ(low.vitals.curHits, 1);
    CHECK_EQ(low.vitals.curSpellPoints, 1);

    Character high;
    high.vitals.maxHits = 20;
    high.vitals.maxSpellPoints = 10;
    Env highEnv({0.999, 0.999});
    chestFate(highEnv.ctx, high);
    CHECK_EQ(high.vitals.curHits, 20);
    CHECK_EQ(high.vitals.curSpellPoints, 10);
}

TEST(status2_blackout_clears_active_spell_effects) {
    Character c;
    c.activeEffectIds = {3, 4, 9};
    Env env({0.5, 0.5});
    clearSpellEffects(env.ctx, c);
    CHECK_EQ(static_cast<i32>(c.activeEffectIds.size()), 0);
}

TEST(status2_help_cry_never_needed_in_solo_party) {
    // Port behaviour: CallForHelp only applies to party size > 1.
    Party p;
    p.size = 1;
    Character& c = p.members[0];
    c.vitals.maxHits = 20;
    c.vitals.curHits = 0;
    CHECK(!helpCryNeeded(p, c));
}

TEST(status2_tick_fail_costs_hits_and_cure_keeps_them) {
    // Poison, CON 18, level 3: bonus 16 - 6 = 10.
    // Rnd 0.5 -> draw 100 -> roll 110, fail: costs 2 hits (20 -> 18).
    Party poisoned;
    poisoned.size = 1;
    Character& pc = poisoned.members[0];
    pc.ailments.poisonLevel = 3;
    pc.vitals.curHits = 20;
    pc.vitals.maxHits = 20;
    Env failEnv({0.5, 0.5});
    poisonTick(failEnv.ctx, poisoned, 0, 18);
    CHECK_EQ(poisoned.members[0].vitals.curHits, 18);

    // Disease, CON 18, no poison: bonus 16.
    // Rnd 0.5 -> draw 100 -> roll 116 < 195, fail: costs 1 hit (20 -> 19).
    Party diseased;
    diseased.size = 1;
    Character& dc = diseased.members[0];
    dc.ailments.diseaseLevel = 3;
    dc.vitals.curHits = 20;
    dc.vitals.maxHits = 20;
    Env diseaseEnv({0.5, 0.5});
    diseaseTick(diseaseEnv.ctx, diseased, 0, 18, false);
    CHECK_EQ(diseased.members[0].vitals.curHits, 19);

    // Poison cure on a winning roll (Rnd 0.995 -> roll 209): hits unchanged.
    Party cured;
    cured.size = 1;
    Character& cc = cured.members[0];
    cc.ailments.poisonLevel = 3;
    cc.vitals.curHits = 20;
    cc.vitals.maxHits = 20;
    Env cureEnv({0.995, 0.995});
    poisonTick(cureEnv.ctx, cured, 0, 18);
    CHECK_EQ(cured.members[0].vitals.curHits, 20);
}
