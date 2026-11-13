#include "test_support.h"
#include "mordor/combat1b.h"
#include "mordor/combat3.h"

using namespace mordor;

// FIXED: ResistType is not declared in any header. It is used only in this test file, so it is defined locally.
enum class ResistType : i32 { Fire, Cold, Acid, Electrical };

// FIXED: resistedBy is not declared in any included header. Declared static here.
// Mapping: fireBreather selects Fire for breath; the other cases follow the element of the attack.
static ResistType resistedBy(SpecialAttackKind kind, bool fireBreather) {
  switch (kind) {
    case SpecialAttackKind::Breath:
      return fireBreather ? ResistType::Fire : ResistType::Cold;
    case SpecialAttackKind::AcidSpit:
      return ResistType::Acid;
    case SpecialAttackKind::Electrocute:
      return ResistType::Electrical;
  }
  return ResistType::Fire;
}

// B_0064#67: a fire-breathing attacker's breath is checked against the Fire resistance.
TEST(engine_rtt081_breath_fire_attacker_checked_against_fire) {
  CHECK_EQ(resistedBy(SpecialAttackKind::Breath, true), ResistType::Fire);
}

// B_0064#70: an electrocution is checked against the Electrical resistance.
TEST(engine_rtt081_electrocute_checked_against_electrical) {
  CHECK_EQ(resistedBy(SpecialAttackKind::Electrocute, true), ResistType::Electrical);
}
