#include "test_support.h"
#include "mordor/combat3.h"

#include <cstring>

using namespace mordor;

// ---- Defend (R8, R9) -------------------------------------------------------

TEST(defend_bonus_constant_and_no_effect) {
    // R9: the intended +50% is never applied; the value passes through unchanged.
    CHECK_EQ(kDefendIntendedBonus, 0.5);
    CHECK_EQ(applyDefendBonus(40, true), 40);
    CHECK_EQ(applyDefendBonus(40, false), 40);
    CHECK_EQ(applyDefendBonus(0, true), 0);
}

// ---- Paralysis and stat part (R5, R6) --------------------------------------

TEST(stat_part_held_until_paralysed) {
    // R5: a paralysed character has its stat part shifted out.
    Character c;
    CHECK(statPartHeld(c));
    c.ailments.paralysed = true;
    CHECK(!statPartHeld(c));
}

TEST(attack_and_defence_never_negative) {
    // R2: both totals are floored at zero, so they are never negative.
    tsup::Env env({0.5, 0.9});
    Character c;
    Inventory inv;
    CHECK(attackOf(env.ctx, c, inv) >= 0);
    CHECK(defenseOf(env.ctx, c, inv) >= 0);
}

// ---- Counting items (R11) --------------------------------------------------

TEST(counts_for_wearer_when_alignment_matches) {
    // R11: an item counts when it is of the wearer's own alignment.
    Character c;
    c.alignment = Align::Good;
    Item it;
    it.alignment = ItemAlign::Good;
    it.aligned = true;
    it.alignFlags = kAlignBitGood;
    CHECK(countsForWearer(c, it));
}

TEST(counts_for_wearer_false_when_alignment_differs) {
    Character c;
    c.alignment = Align::Good;
    Item it;
    it.alignment = ItemAlign::Evil;
    it.aligned = true;
    it.alignFlags = kAlignBitEvil;
    CHECK(!countsForWearer(c, it));
}

// ---- Gear resistance replacement (R10, R13) --------------------------------

TEST(gear_resist_sets_90_when_item_resists) {
    // R10 step 2: a counting resisting item sets the value to 90.
    CHECK_EQ(gearResistReplace(30, true), 90);
    CHECK_EQ(gearResistReplace(0, true), 90);
}

TEST(gear_resist_keeps_base_without_item) {
    // R10 step 1 only: no resisting item leaves the race base alone.
    CHECK_EQ(gearResistReplace(30, false), 30);
    CHECK_EQ(gearResistReplace(0, false), 0);
}

TEST(gear_resist_replaces_even_when_base_higher) {
    // R13 quirk: 90 overrides the race base even when the base is higher.
    CHECK_EQ(gearResistReplace(95, true), 90);
}

TEST(gear_resist_constant_is_90) {
    CHECK_EQ(kCountingResistPercent, 90);
}

// ---- Item resistance bitmask (R14) -----------------------------------------

TEST(item_resist_bits_one_per_kind) {
    // R14: bit i corresponds to kind i, eleven flags 0x001..0x400.
    for (i32 i = 0; i < kResistKinds; ++i) {
        CHECK_EQ(kItemResistBits[i], static_cast<u32>(1) << i);
    }
}

TEST(item_resists_kind_uses_bit_for_kind) {
    CHECK(itemResistsKind(0x001, DamageKind::Fire));
    CHECK(!itemResistsKind(0x001, DamageKind::Cold));
    CHECK(itemResistsKind(0x100, DamageKind::Paralysis));
    CHECK(itemResistsKind(0x200, DamageKind::Drain));
    CHECK(itemResistsKind(0x400, DamageKind::Acid));
    CHECK(!itemResistsKind(0, DamageKind::Fire));
}

TEST(item_resists_each_kind_by_its_own_bit) {
    for (i32 k = 0; k < kResistKinds; ++k) {
        CHECK(itemResistsKind(kItemResistBits[k], static_cast<DamageKind>(k)));
    }
}

// ---- Resistance-raising spells (R15, R16) ----------------------------------

TEST(resist_raise_constant_is_95) {
    CHECK_EQ(kResistRaisePercent, 95);
}

TEST(resist_spell_rows_match_srd) {
    // R16 rows: name, kind, base level, See, Wiz, Hea levels.
    CHECK(std::strcmp(kResistSpells[0].name, "Resist Fire") == 0);
    CHECK(kResistSpells[0].kind == DamageKind::Fire);
    CHECK_EQ(kResistSpells[0].baseLevel, 3);
    CHECK_EQ(kResistSpells[0].seerLevel, 9);
    CHECK_EQ(kResistSpells[0].wizardLevel, 9);
    CHECK_EQ(kResistSpells[0].healerLevel, 6);

    CHECK(kResistSpells[3].kind == DamageKind::Poison);
    CHECK_EQ(kResistSpells[3].baseLevel, 4);
    CHECK_EQ(kResistSpells[3].seerLevel, 12);
    CHECK_EQ(kResistSpells[3].wizardLevel, 12);
    CHECK_EQ(kResistSpells[3].healerLevel, 8);

    CHECK(kResistSpells[7].kind == DamageKind::Magic);
    CHECK_EQ(kResistSpells[7].baseLevel, 5);
    CHECK_EQ(kResistSpells[7].seerLevel, 15);
    CHECK_EQ(kResistSpells[7].wizardLevel, 15);
    CHECK_EQ(kResistSpells[7].healerLevel, 10);

    // Mental Resistance: SRD lists only Sor and Wiz (18); no See or Hea entry.
    CHECK(kResistSpells[8].kind == DamageKind::Mind);
    CHECK_EQ(kResistSpells[8].baseLevel, 6);
    CHECK_EQ(kResistSpells[8].wizardLevel, 18);
    CHECK_EQ(kResistSpells[8].seerLevel, 0);
    CHECK_EQ(kResistSpells[8].healerLevel, 0);
}

TEST(resist_spell_table_covers_each_kind_once) {
    CHECK_EQ(kResistSpellCount, 11);
    CHECK_EQ(kResistSpellCount, kResistKinds);
    for (i32 k = 0; k < kResistKinds; ++k) {
        i32 hits = 0;
        for (i32 r = 0; r < kResistSpellCount; ++r) {
            if (static_cast<i32>(kResistSpells[r].kind) == k) ++hits;
        }
        CHECK_EQ(hits, 1);
    }
}

// ---- Class to resistance type (R17) ----------------------------------------

// ---- Monster chance to hit (R18) -------------------------------------------

TEST(monster_hit_constants_match_srd) {
    CHECK_EQ(kMonHitEqualChance, 50.0);
    CHECK_EQ(kMonHitSpread, 50.0);
    CHECK_EQ(kMonHitFloor, 2.0);
    CHECK_EQ(kMonHitCeiling, 98.0);
}

TEST(monster_hit_equal_attack_and_defence_is_50) {
    CHECK_EQ(monsterHitChance(10, 10), 50.0);
}

TEST(monster_hit_above_and_below_formula) {
    // MonAtk > CharDef: 50 + 50*(20-10)/20 = 75
    CHECK_EQ(monsterHitChance(20, 10), 75.0);
    // MonAtk < CharDef: 50 - 50*(20-10)/20 = 25
    CHECK_EQ(monsterHitChance(10, 20), 25.0);
}

TEST(monster_hit_clamps_to_floor_and_ceiling) {
    // 50 - 50*(25-1)/25 = 2 exactly, the floor.
    CHECK_EQ(monsterHitChance(1, 25), 2.0);
    // 50 - 50*(1000-1)/1000 = 0.05, clamped up to 2.
    CHECK_EQ(monsterHitChance(1, 1000), 2.0);
    // 50 + 50*(25-1)/25 = 98 exactly, the ceiling.
    CHECK_EQ(monsterHitChance(25, 1), 98.0);
    // 50 + 49.95 = 99.95, clamped down to 98.
    CHECK_EQ(monsterHitChance(1000, 1), 98.0);
}

TEST(monster_hit_roll_at_50_percent) {
    // Integer chance 50: roll 0.25 -> value 25 < 50 hits; roll 0.5 -> 50 misses.
    // FIXED: single-element braced list narrowed double to u64; use the two-element form used elsewhere in this file.
    tsup::Env hit({0.25, 0.9});
    CHECK(monsterHitRoll(hit.ctx, 10, 10));
    tsup::Env miss({0.5, 0.9});
    CHECK(!monsterHitRoll(miss.ctx, 10, 10));
}

TEST(monster_hit_roll_at_floor_chance) {
    // Chance 2: roll 0.0 -> value 0 hits; roll 0.5 -> value 50 misses.
    // FIXED: single-element braced list narrowed double to u64; use the two-element form used elsewhere in this file.
    tsup::Env hit({0.0, 0.9});
    CHECK(monsterHitRoll(hit.ctx, 1, 25));
    tsup::Env miss({0.5, 0.9});
    CHECK(!monsterHitRoll(miss.ctx, 1, 25));
}

// ---- Player attack call (R19) ----------------------------------------------

