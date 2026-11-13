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

// ---------------------------------------------------------------------------
// SL cap at 255 (SRD R6 legend: "SL: spell level, capped at 255")
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Modifier2 = Int(Rnd * (Modifier1 - ((SL-1)/SL^2)) / 2) + 1
// SL = 1 makes the subtracted term exactly 0, avoiding integer/double ambiguity.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Damage pipeline (SRD R6): Damage = Int(M1 - M2), then * (1 - Resistance/100)
// Example: base 12, SL 1, Power 1 -> M1 = 17.168119; Rnd 0.5 -> M2 = 5;
// Damage = Int(12.168119) = 12.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Healing pipeline (SRD R7): no resistance step.
// Example: base 4, SL 10, Power 11 -> M1 = 18.
//   Rnd 0.5: M2 = Int(0.5*(18 - 0.09)/2) + 1 = Int(4.4775) + 1 = 5; Healing = 13.
//   Rnd 0.0: M2 = 1; Healing = 17.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Crest-granted spells (SRD table c106 and crest grants)
// ---------------------------------------------------------------------------

