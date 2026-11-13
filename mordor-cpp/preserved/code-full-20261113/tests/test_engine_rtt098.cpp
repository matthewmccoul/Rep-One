// t098: Spellbook cost and known-spell assertions from the reference (B_0061, B_0062).
// Blocked: our API declarations for the spell cost function, the spell known-check,
// Character creation with guild memberships, and spell lookup by name are not in the
// provided headers. Each case is SKIPped until those declarations are supplied (see NEED).
#include "test_support.h"

using namespace mordor;

// SKIP B_0061#80: no declaration for the spell cost function or Character with Nomad 37 / Sorcerer 37; expected Pillar of Fire = 31
// SKIP B_0061#81: no declaration for the spell cost function or Character with Nomad 37 / Sorcerer 37; expected Resist Fire = 4
// SKIP B_0061#82: no declaration for the spell cost function or Character with Nomad 37 / Sorcerer 37; expected Resist Draining = 4
// SKIP B_0061#83: no declaration for the spell cost function or Character with Nomad 37 / Sorcerer 37; expected Resist Cold = 4
// SKIP B_0061#84: no declaration for the spell cost function or Character with Nomad 37 / Sorcerer 37; expected Resist Poison = 7
// SKIP B_0061#85: no declaration for the spell cost function or Character with Nomad 37 / Sorcerer 37; expected Resist Disease = 7
// SKIP B_0062#43: no declaration for Character creation with Thief at level 4 or the spell known-check; expected Charm of Opening known = false
// SKIP B_0062#44: no declaration for Character creation with Thief at level 5 or the spell known-check; expected Charm of Opening known = true

