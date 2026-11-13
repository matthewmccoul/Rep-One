#include "test_support.h"

#include "mordor/companions2b.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

using namespace mordor;
using tsup::Env;

// Binding step cost. Engine formula preferred per SRD conflict note 1:
// f = (a + 4 > 12) ? 12 - a : a + 4. Cost is linear in current.

// Chance to leave.

// Raise chance is min(98, 85 + CON).

// Turning rules.

// Breath stop rule: fire is never stopped by an earlier breath; cold is.

// Breath roll fires only on a roll below the chance (chance is 20, 50 or 80).

// Special range extras: breath a quarter of range; spit and electrocution a half.

// Stone follow-up can land on the next monster in the group that has hits left.

// Release keeps the last occupant's record.

// Drag swap is refused if either end is dead.

// Purchase hits are capped at the definition's max.

// Joiners from a fight take the front monster's hits and maximum.

// Only group 0's join offer is ever shown.

// Join offer admits a group whose alignment matches the viewer.
// (Alignment-1 admission deliberately not tested: SRD conflict 19.)

// Bind spells do not work on companions that joined of their own free will.

// Only Id 307 is shared with the Morgul.

// No monster in MDATA5 has spawn mode 5, 6 or 7.

// Companion casts at guild level / 6.

// Heals with M5B1D = 0 reach nobody.

// While a morgue body is carried, the carry choice is skipped.

