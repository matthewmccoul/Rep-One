#include "test_support.h"
#include "mordor/spells2.h"
#include "mordor/spells1a.h"

using namespace mordor;

// Rnd = 0 gives cut = 1 in the character and monster damage formulas (R7, R10).
// Env scripts repeat their last value, so {0.0, 0.0} means Rnd is always 0.

// ---- Character/companion damage (R7, R8) ----

TEST(player_damage_base_case) {
    // SL=1: Ln(2)/Ln(5) = 0.43068. mod1 = 10 + 4.3068 = 14.3068.
    // mod2 = Int(0 * ...) + 1 = 1. dmg = Int(14.3068 - 1) = 13. No resist.
    tsup::Env env({0.0, 0.0});
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 0, 1, 0), 13);
}

TEST(player_damage_log_and_power_terms) {
    // SL=10, SpecialEffect=5 (power 6).
    // mod1 = 10 + 10*ln11/ln5 + Int((10/10)*5) = 10 + 14.899 + 5 = 29.899.
    // mod2 = 1, dmg = Int(28.899) = 28.
    tsup::Env env({0.0, 0.0});
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 5, 10, 0), 28);
}

TEST(player_damage_random_cut_at_max_draw) {
    // SL=1, Rnd=0.999: mod2 = Int(0.999 * 14.3068 / 2) + 1 = Int(7.146) + 1 = 8.
    // dmg = Int(14.3068 - 8) = 6.
    tsup::Env env({0.999, 0.999});
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 0, 1, 0), 6);
}

TEST(player_damage_resist_rounding_and_clamp) {
    // Round half even (R7). SL=1 gives dmg 13: 13 * 0.5 = 6.5 -> 6.
    // SL=2 gives dmg 15: mod1 = 10 + 10*ln3/ln5 = 16.826, dmg = Int(15.826) = 15.
    //   15 * 0.5 = 7.5 -> 8.
    // Resist is clamped to 0..100: 150 -> 100 (no damage), -20 -> 0 (full damage).
    tsup::Env env({0.0, 0.0});
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 0, 1, 50), 6);
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 0, 2, 50), 8);
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 0, 1, 150), 0);
    CHECK_EQ(playerSpellDamage(env.ctx, 10, 0, 1, -20), 13);
}

// ---- Monster damage (R10, R39) ----

TEST(monster_damage_base_and_resist_floor) {
    // lvl=4: l042C = ln5/ln5 = 1. power = 10 + 10 + Int(0.4*0) = 20.
    // Rnd=0: cut = 1, dmg = Int(19) = 19.
    tsup::Env env({0.0, 0.0});
    CHECK_EQ(monsterSpellDamage(env.ctx, 4, 10, 0, false, 0), 19);
    // Resistance counts only when ResistIndex > -1 (hasResist true).
    // 19 * (1 - 0.5) = 9.5, and VbInt floors it to 9 (not half-even).
    CHECK_EQ(monsterSpellDamage(env.ctx, 4, 10, 0, true, 50), 9);
    // hasResist false: the resistance percent is ignored.
    CHECK_EQ(monsterSpellDamage(env.ctx, 4, 10, 0, false, 50), 19);
}

TEST(monster_damage_power_term_and_zero_floor) {
    // lvl=5, SpecialEffect=3: Int(5/10.0 * 3) = Int(1.5) = 1.
    // l042C = ln6/ln5 = 1.11328. power = 10 + 11.1328 + 1 = 22.1328. Rnd=0: dmg = Int(21.13) = 21.
    tsup::Env env({0.0, 0.0});
    CHECK_EQ(monsterSpellDamage(env.ctx, 5, 10, 3, false, 0), 21);
    // lvl=1, damage2=1: power = 1 + 0.4307 = 1.4307. dmg = Int(0.4307) = 0. Below 1 becomes 0.
    CHECK_EQ(monsterSpellDamage(env.ctx, 1, 1, 0, false, 0), 0);
}

// ---- Turn damage and group loop (R11, R12, R13, R14) ----

TEST(turn_damage_is_assigned_not_added) {
    // Monster spell damage replaces the turn's total (R11).
    i32 total = 50;
    setTurnDamageFromSpell(total, 12);
    CHECK_EQ(total, 12);
}

TEST(damage_loop_counting_and_empty_slots) {
    // Processed groups bump both counters. Empty slots bump only the step.
    DamageLoopCounters c{};
    CHECK(damageLoopContinues(c, 3));
    advanceDamageLoop(c, true);    // group 1, step 1
    advanceDamageLoop(c, false);   // empty slot: step 2 only
    CHECK_EQ(c.groupCount, 1);
    CHECK_EQ(c.step, 2);
    CHECK(damageLoopContinues(c, 3));   // 1 < 3
    advanceDamageLoop(c, true);    // group 2, step 3
    advanceDamageLoop(c, true);    // group 3, step 4
    CHECK_EQ(c.groupCount, 3);
    CHECK_EQ(c.step, 4);
    CHECK(!damageLoopContinues(c, 3));  // 3 is not below 3

    // Two skips in a row never raise the group count.
    DamageLoopCounters d{};
    advanceDamageLoop(d, false);
    advanceDamageLoop(d, false);
    CHECK_EQ(d.groupCount, 0);
    CHECK_EQ(d.step, 2);
    CHECK(damageLoopContinues(d, 1));

    // Damage1 = 1 stops after one processed group.
    DamageLoopCounters e{};
    advanceDamageLoop(e, true);
    CHECK(!damageLoopContinues(e, 1));
}

TEST(start_group_moves_off_dead_aim) {
    // Aimed group 0 has nobody living, so the first living group (1) is used (R13).
    // A living aim is kept. Both livingCount and front.alive are set so the test is consistent.
    Encounter enc{};
    enc.groups[0].livingCount = 0;
    enc.groups[0].front.alive = false;
    enc.groups[1].livingCount = 1;
    enc.groups[1].front.alive = true;
    enc.groups[2].livingCount = 1;
    enc.groups[2].front.alive = true;
    CHECK_EQ(spellStartGroup(enc, 0), 1);
    CHECK_EQ(spellStartGroup(enc, 2), 2);
}

// ---- Protection (R28, R29, R30) ----

TEST(protect_values_and_defence_bonus) {
    // Cast value: Int(SL/2) + 1.
    CHECK_EQ(protectValueAtCast(5), 3);   // Int(2.5) + 1
    CHECK_EQ(protectValueAtCast(1), 1);   // Int(0.5) + 1
    CHECK_EQ(protectValueAtCast(6), 4);   // 3 + 1
    // Recompute value: Int(guild level / 2) + 1.
    CHECK_EQ(protectValueOnRecompute(7), 4);  // Int(3.5) + 1
    CHECK_EQ(protectValueOnRecompute(0), 1);
    // Defence bonus: B(3 * ln(P)^2), B = VB Integer rounding (half even).
    CHECK_EQ(protectionDefenceBonus(1), 0);   // ln1 = 0
    CHECK_EQ(protectionDefenceBonus(2), 1);   // 3 * 0.48045 = 1.4414 -> 1
    CHECK_EQ(protectionDefenceBonus(3), 4);   // 3 * 1.20695 = 3.6208 -> 4
    CHECK_EQ(protectionDefenceBonus(4), 6);   // 3 * 1.92181 = 5.7654 -> 6
}

// ---- Caster levels (R32, R38) ----

TEST(caster_level_clamps) {
    // Companion: clamp(Floor(GL/6), 1, 255).
    CHECK_EQ(companionCasterLevel(0), 1);       // 0 raised to the minimum 1
    CHECK_EQ(companionCasterLevel(11), 1);      // Floor(1.83) = 1
    CHECK_EQ(companionCasterLevel(12), 2);      // Floor(2.0) = 2
    CHECK_EQ(companionCasterLevel(1600), 255);  // 266 capped at 255
    // Monster: max(1, Floor(GL/4)), capped at 255.
    CHECK_EQ(monsterCasterLevel(0), 1);
    CHECK_EQ(monsterCasterLevel(3), 1);         // Floor(0.75) = 0, raised to 1
    CHECK_EQ(monsterCasterLevel(8), 2);
    CHECK_EQ(monsterCasterLevel(1100), 255);    // 275 capped at 255
}

// ---- Resistance grant (R31) ----

TEST(spell_resistance_grant_does_not_stack) {
    // Kind 0 is SpellResist slot 0. Kind 2 is SpellResist slot 2.
    // A grant sets SpellResist to 95 only when the current value is below 95.
    tsup::Env env({0.0, 0.0});
    Character c{};
    c.resistances.resist[0] = 0;
    c.resistances.resist[2] = 97;
    castResistSpell(env.ctx, c.resistances, static_cast<SpellResistKind>(0));
    CHECK_EQ(c.resistances.resist[0], 95);
    castResistSpell(env.ctx, c.resistances, static_cast<SpellResistKind>(2));
    CHECK_EQ(c.resistances.resist[2], 97);
}

// ---- Healing (R24, R25) ----

TEST(heal_minimum_and_random_cut) {
    // SL=1 makes Ln(SL)/Ln(10) = 0, so mod1 = base and the power term is 0.
    // mod2 = Int(Rnd * base / 2) + 1.
    tsup::Env low({0.0, 0.0});
    CHECK_EQ(spellHealRoll(low.ctx, 10, 0, 1), 9);   // mod2 = 1, Int(10 - 1) = 9
    CHECK_EQ(spellHealRoll(low.ctx, 1, 0, 1), 1);    // Int(1 - 1) = 0, minimum heal is 1

    tsup::Env hi({0.999, 0.999});
    CHECK_EQ(spellHealRoll(hi.ctx, 10, 0, 1), 5);    // mod2 = Int(4.995) + 1 = 5, heal = 5
    CHECK_EQ(spellHealRoll(hi.ctx, 1, 0, 1), 1);     // mod2 = 1, Int(0) = 0, minimum 1
}

// ---- Acid spit (R40, R41) ----

TEST(acid_chance_draw_and_damage) {
    // Chance bands for monster level 20: above 25 -> 90, below 15 -> 10, otherwise 50.
    CHECK_EQ(acidSpitChancePercent(20, 26), 90);
    CHECK_EQ(acidSpitChancePercent(20, 25), 50);   // 25 is not above 25
    CHECK_EQ(acidSpitChancePercent(20, 15), 50);   // 15 is not below 15
    CHECK_EQ(acidSpitChancePercent(20, 14), 10);
    CHECK_EQ(acidSpitChancePercent(20, 20), 50);

    // Target = Int(Rnd * 5): 0 is the character, 1..4 are companion slots.
    tsup::Env ea({0.0, 0.0});
    CHECK_EQ(rollAcidTargetSlot(ea.ctx), 0);
    tsup::Env eb({0.5, 0.5});
    CHECK_EQ(rollAcidTargetSlot(eb.ctx), 2);       // Int(2.5)
    tsup::Env ec({0.999, 0.999});
    CHECK_EQ(rollAcidTargetSlot(ec.ctx), 4);       // Int(4.995)

    // Monster level 20, character level 0: l046C = 21, range = Int(57.75 + 1) = 58.
    // Rnd=0: Int(0.5) = 0, (0 + 29) * 0.7 = 20.3 -> 20.
    // Rnd=0.999: Int(29.47) = 29, (29 + 29) * 0.7 = 40.6 -> 40 (VbInt floors).
    tsup::Env low({0.0, 0.0});
    CHECK_EQ(acidSpitDamage(low.ctx, 20, 0), 20);
    tsup::Env high({0.999, 0.999});
    CHECK_EQ(acidSpitDamage(high.ctx, 20, 0), 40);
}

// ---- Special hits (R37) ----

TEST(special_hit_range_levels) {
    // l040C = attacker + 1 = 10. Range = Int((ln10 + 10/15) / ln1.08 - target/10 + 1).
    // (2.302585 + 0.666667) / 0.076961 = 38.581. Target 0 -> Int(39.581) = 39.
    // Target 20 -> Int(38.581 - 2 + 1) = Int(37.581) = 37.
    CHECK_EQ(specialHitRange(9, 0), 39);
    CHECK_EQ(specialHitRange(9, 20), 37);
}

TEST(breath_sweep_guild_level_gate) {
    // Breath sweep needs SpecialAttackFlags bit 3 and guild level under 40 (R36).
    const unsigned breath = 1u << 3;
    CHECK(breathSweepApplies(breath, 39));
    CHECK(!breathSweepApplies(breath, 40));
    CHECK(!breathSweepApplies(0u, 10));
}

// ---- Immunity rules (R34, R35) ----

TEST(companion_immunity_rules) {
    // R35: party damage is zeroed when the target companion has bit 2 (value 4).
    CHECK_EQ(applyTargetImmunityToPartyDamage(17, 4u), 0);
    CHECK_EQ(applyTargetImmunityToPartyDamage(17, 0u), 17);
    CHECK_EQ(applyTargetImmunityToPartyDamage(17, 8u), 17);  // bit 3 is not immunity

    // R34: the immunity test reads the caster companion, not the target.
    SpellCasterInfo immune{};
    immune.isCompanion = true;
    immune.magicImmune = true;
    SpellCasterInfo normal{};
    normal.isCompanion = true;
    normal.magicImmune = false;
    CHECK(companionCastBlockedByOwnImmunity(immune));
    CHECK(!companionCastBlockedByOwnImmunity(normal));
}

// ---- Charm room (R43) ----

TEST(charm_needs_free_companion_slot) {
    // kCompanionSlots = 4. Charm is refused only when all four slots are occupied.
    CHECK(charmHasRoom(0));
    CHECK(charmHasRoom(3));
    CHECK(!charmHasRoom(4));
}

// ---- Typed offsets (R50, R51) ----

TEST(typed_offsets_read_by_kind) {
    // Teleport reads Right, Forward, Down.
    TypedOffset t{9, 9, 9};
    Vec<i32> three{1, 2, 3};
    readTypedOffsets(three, true, t);
    CHECK_EQ(t.right, 1);
    CHECK_EQ(t.forward, 2);
    CHECK_EQ(t.down, 3);

    // Displacement reads Right, Forward.
    TypedOffset d{};
    Vec<i32> two{4, 5};
    readTypedOffsets(two, false, d);
    CHECK_EQ(d.right, 4);
    CHECK_EQ(d.forward, 5);

    // Fewer than two values: nothing happens, so the output is unchanged.
    TypedOffset s{9, 9, 9};
    Vec<i32> one{7};
    readTypedOffsets(one, true, s);
    CHECK_EQ(s.right, 9);
    CHECK_EQ(s.forward, 9);
    CHECK_EQ(s.down, 9);
}

TEST(offset_rotates_with_facing) {
    // A 180-degree facing change negates the displacement. A zero offset leaves the square as is.
    Location from{};
    from.x = 15;
    from.y = 15;
    from.dungeonLevel = 5;
    TypedOffset off{2, 3, 0};
    Location n = offsetFromFacing(from, off, Facing::N);
    Location s = offsetFromFacing(from, off, Facing::S);
    CHECK_EQ(s.x - 15, -(n.x - 15));
    CHECK_EQ(s.y - 15, -(n.y - 15));
    CHECK_EQ(n.dungeonLevel, 5);
    CHECK_EQ(s.dungeonLevel, 5);

    Location still = offsetFromFacing(from, TypedOffset{}, Facing::E);
    CHECK_EQ(still.x, 15);
    CHECK_EQ(still.y, 15);
}

// ---- Arrival outcome (R53) ----

TEST(arrival_outcome_table) {
    // Order: level 0 or below falls; above 15 is rock; rock square is rock; else arrives.
    CHECK(arrivalOutcome(0, false) == ArrivalOutcome::FallsFromTheSky);
    CHECK(arrivalOutcome(-1, false) == ArrivalOutcome::FallsFromTheSky);
    CHECK(arrivalOutcome(1, false) == ArrivalOutcome::Arrives);
    CHECK(arrivalOutcome(15, false) == ArrivalOutcome::Arrives);   // 15 is not above 15
    CHECK(arrivalOutcome(16, false) == ArrivalOutcome::IntoRock);
    CHECK(arrivalOutcome(1, true) == ArrivalOutcome::IntoRock);
    CHECK(arrivalOutcome(0, true) == ArrivalOutcome::FallsFromTheSky);  // fall is checked first
}

// ---- Class gate (R18) ----

TEST(class_gate_only_applies_to_effects_75_to_99) {
    // Effect 75 needs class 0. Effect 99 needs class 24 (effect - 75).
    CHECK(classGatePasses(75, 0));
    CHECK(!classGatePasses(75, 1));
    CHECK(classGatePasses(99, 24));
    CHECK(!classGatePasses(99, 23));
    // Effects outside 75..99 are not gated by class.
    CHECK(classGatePasses(74, 3));
}

// ---- Victim verdicts (R17, R19) ----

TEST(victim_verdicts_immunity_and_all_or_nothing) {
    // Magic immune (bit 2 set) takes nothing.
    Creature m{};
    m.hits = 10;
    m.alive = true;
    m.magicImmune = true;
    m.specialPropertyFlags = 4u;
    CHECK(judgeSpellVictim(m, 1, 50, 0, false) == VictimVerdict::MagicImmune);

    // Effect 100 counts only if the roll kills (damage >= hits).
    Creature f{};
    f.hits = 10;
    f.alive = true;
    CHECK(judgeSpellVictim(f, 100, 5, 0, false) == VictimVerdict::AllOrNothingMiss);
    CHECK(judgeSpellVictim(f, 100, 12, 0, false) == VictimVerdict::Hit);
}
