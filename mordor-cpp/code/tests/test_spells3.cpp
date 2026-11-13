// Tests for spells3: movement/utility spells, damage and healing formulas,
// and crest-granted spells. Expected values are derived by hand from the SRD
// formulas (Real Spell Damage / Healing Formula, crest table, movement table).
// UNKNOWN items (stat requirements, Teleport impale formula, Feather Essence
// failure, rounding after resistance, SL 0 behaviour) are deliberately untested.

#include "test_support.h"
#include "mordor/spells3.h"

#include <cmath>
#include <initializer_list>  // FIXED: needed for explicit std::initializer_list<double> rolls

using namespace mordor;

namespace {

bool closeTo(double a, double b, double tol = 1e-3) {
    return std::fabs(a - b) < tol;
}

}  // namespace

// ---------------------------------------------------------------------------
// Movement and utility spell table (SRD R1, R2, table c106)
// ---------------------------------------------------------------------------

TEST(spells3_movement_scope) {
    // Only Teleport is marked for combat; everything else is non-combat.
    CHECK(movementScopeOf(MovementSpell::DetectRock) == MovementScope::NonCombat);
    CHECK(movementScopeOf(MovementSpell::FeatherEssence) == MovementScope::NonCombat);
    CHECK(movementScopeOf(MovementSpell::EtherealPortal) == MovementScope::NonCombat);
    CHECK(movementScopeOf(MovementSpell::Displacement) == MovementScope::NonCombat);
    CHECK(movementScopeOf(MovementSpell::MassLevitate) == MovementScope::NonCombat);
    CHECK(movementScopeOf(MovementSpell::Teleport) == MovementScope::CombatAndNonCombat);
    CHECK(movementScopeOf(MovementSpell::Sanctuary) == MovementScope::NonCombat);
}

TEST(spells3_movement_base_table) {
    CHECK_EQ(movementSpellBase(MovementSpell::DetectRock), 1);
    CHECK_EQ(movementSpellBase(MovementSpell::FeatherEssence), 3);
    CHECK_EQ(movementSpellBase(MovementSpell::EtherealPortal), 5);
    CHECK_EQ(movementSpellBase(MovementSpell::Displacement), 8);
    CHECK_EQ(movementSpellBase(MovementSpell::MassLevitate), 8);
    CHECK_EQ(movementSpellBase(MovementSpell::Teleport), 12);
    CHECK_EQ(movementSpellBase(MovementSpell::Sanctuary), 12);
}

TEST(spells3_only_teleport_combat_marked) {
    i32 marked = 0;
    bool markedIsTeleport = false;
    for (const auto& row : kMovementSpellRows) {
        if (row.combatMarked) {
            ++marked;
            markedIsTeleport = (row.spell == MovementSpell::Teleport);
        }
    }
    CHECK_EQ(marked, 1);
    CHECK(markedIsTeleport);
}

TEST(spells3_teleport_constants) {
    CHECK_EQ(kTeleportMaxLevels, 3);
    CHECK_EQ(kTeleportCombatMaxMonsters, 4);
    CHECK_EQ(kTeleportCombatMaxGroups, 4);
}

// ---------------------------------------------------------------------------
// Modifier1 (SRD "Real Spell Damage Formula" and "Real Healing Formula")
//   damage:  Base + Base*LN(SL+1)/LN(5) + Int((SL/10)*(Power-1))
//   healing: Base + Base*LN(SL)/LN(10)  + Int((SL/10)*(Power-1))
// ---------------------------------------------------------------------------

TEST(spells3_damage_modifier1_example) {
    // ln11/ln5 = 1.4898961; SL/10 = 1, Power-1 = 10 -> +10.
    // 1 + 1.4898961 + 10 = 12.4898961
    CHECK(closeTo(damageModifier1(1, 10, 11), 12.4899));
}

TEST(spells3_healing_modifier1_example) {
    // LN(10)/LN(10) = 1, so 4 + 4 + Int(1*10) = 18.
    CHECK(closeTo(healingModifier1(4, 10, 11), 18.0));
}

TEST(spells3_damage_vs_healing_log_base) {
    // Same inputs at SL 10, Power 1 (no power term).
    // Damage: 4 + 4*ln11/ln5 = 9.959584
    // Healing: 4 + 4*ln10/ln10 = 8
    CHECK(closeTo(damageModifier1(4, 10, 1), 9.9596));
    CHECK(closeTo(healingModifier1(4, 10, 1), 8.0));
}

TEST(spells3_healing_power_term_truncates) {
    // ln15/ln10 = 1.1760913; 4 * that = 4.704365.
    // Power term: Int(1.5 * 1) = 1. Total = 4 + 4.704365 + 1 = 9.704365.
    CHECK(closeTo(healingModifier1(4, 15, 2), 9.7044));
}

// ---------------------------------------------------------------------------
// SL cap at 255 (SRD R6 legend: "SL: spell level, capped at 255")
// ---------------------------------------------------------------------------

TEST(spells3_effective_level_cap) {
    CHECK_EQ(effectiveSpellLevel(100), 100);
    CHECK_EQ(effectiveSpellLevel(255), 255);
    CHECK_EQ(effectiveSpellLevel(256), 255);
    CHECK_EQ(effectiveSpellLevel(1000), 255);
}

// ---------------------------------------------------------------------------
// Modifier2 = Int(Rnd * (Modifier1 - ((SL-1)/SL^2)) / 2) + 1
// SL = 1 makes the subtracted term exactly 0, avoiding integer/double ambiguity.
// ---------------------------------------------------------------------------

TEST(spells3_modifier2_mid_roll) {
    // Rnd 0.5, M1 10: 0.5 * 10 / 2 = 2.5 -> 2, +1 = 3.
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: roll is a double; braced u64 narrowed
    CHECK_EQ(spellModifier2(env.ctx, 10.0, 1), 3);
}

TEST(spells3_modifier2_roll_bounds) {
    // Rnd 0.0 gives the minimum: Int(0) + 1 = 1.
    {
        tsup::Env env(std::initializer_list<double>{0.0});  // FIXED: see above
        CHECK_EQ(spellModifier2(env.ctx, 10.0, 1), 1);
    }
    // Rnd 0.999: 0.999 * 10 / 2 = 4.995 -> 4, +1 = 5.
    {
        tsup::Env env(std::initializer_list<double>{0.999});  // FIXED: see above
        CHECK_EQ(spellModifier2(env.ctx, 10.0, 1), 5);
    }
}

// ---------------------------------------------------------------------------
// Damage pipeline (SRD R6): Damage = Int(M1 - M2), then * (1 - Resistance/100)
// Example: base 12, SL 1, Power 1 -> M1 = 17.168119; Rnd 0.5 -> M2 = 5;
// Damage = Int(12.168119) = 12.
// ---------------------------------------------------------------------------

TEST(spells3_damage_example) {
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: see above
    CHECK_EQ(spellDamage(env.ctx, 12, 1, 1, 0), 12);
}

TEST(spells3_damage_resistance_half) {
    // 12 * (1 - 50/100) = 6, an exact integer.
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: see above
    CHECK_EQ(spellDamage(env.ctx, 12, 1, 1, 50), 6);
}

TEST(spells3_damage_resistance_full) {
    // 100% resistance removes all damage.
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: see above
    CHECK_EQ(spellDamage(env.ctx, 12, 1, 1, 100), 0);
}

// ---------------------------------------------------------------------------
// Healing pipeline (SRD R7): no resistance step.
// Example: base 4, SL 10, Power 11 -> M1 = 18.
//   Rnd 0.5: M2 = Int(0.5*(18 - 0.09)/2) + 1 = Int(4.4775) + 1 = 5; Healing = 13.
//   Rnd 0.0: M2 = 1; Healing = 17.
// ---------------------------------------------------------------------------

TEST(spells3_healing_example) {
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: see above
    CHECK_EQ(spellHealing(env.ctx, 4, 10, 11), 13);
}

TEST(spells3_healing_roll_zero) {
    tsup::Env env(std::initializer_list<double>{0.0});  // FIXED: see above
    CHECK_EQ(spellHealing(env.ctx, 4, 10, 11), 17);
}

// ---------------------------------------------------------------------------
// Crest-granted spells (SRD table c106 and crest grants)
// ---------------------------------------------------------------------------

TEST(spells3_crest_grant_lookup) {
    CrestGrantRow paladin = crestGrantFor(Guild::Paladin);
    CHECK(paladin.spell == CrestSpell::DispelUndead);
    CHECK_EQ(paladin.charges, 12);

    CrestGrantRow seeker = crestGrantFor(Guild::Seeker);
    CHECK(seeker.spell == CrestSpell::Displacement);
    CHECK_EQ(seeker.charges, 6);

    CrestGrantRow healer = crestGrantFor(Guild::Healer);
    CHECK(healer.spell == CrestSpell::Restoration);
    CHECK_EQ(healer.charges, 6);
}

TEST(spells3_crest_table_matches_grants) {
    // Every row of the constant table must agree with crestGrantFor.
    // Paladin has 12 charges; every other listed crest has 6.
    for (const auto& row : kCrestGrants) {
        CrestGrantRow got = crestGrantFor(row.crest);
        CHECK(got.spell == row.spell);
        CHECK_EQ(got.charges, row.charges);
        CHECK_EQ(row.charges, row.crest == Guild::Paladin ? 12 : 6);
    }
}

TEST(spells3_cast_crest_sorcerer) {
    // Crest casts its spell at the caster's current spell level (7 here).
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: see above
    Character caster;
    caster.spellLevel = 7;
    CrestCastResult r = castCrestSpell(env.ctx, caster, Guild::Sorcerer);
    CHECK(r.cast);
    CHECK(r.spell == CrestSpell::PillarOfFire);
    CHECK_EQ(r.charges, 6);
    CHECK_EQ(r.spellLevel, 7);
}

TEST(spells3_cast_crest_without_spell) {
    // The Warrior crest grants no spell.
    tsup::Env env(std::initializer_list<double>{0.5});  // FIXED: see above
    Character caster;
    caster.spellLevel = 7;
    CrestCastResult r = castCrestSpell(env.ctx, caster, Guild::Warrior);
    CHECK(!r.cast);
    CHECK(r.spell == CrestSpell::None);
}
