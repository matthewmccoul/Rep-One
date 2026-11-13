#include "test_support.h"

// Batch t099: Spellbook.Known for Charm of Opening, Nomad guild, level 22 and 23.
// Both cases need a Character factory (new human, all stats 18, Nomad at a given level)
// and a Spellbook.Known equivalent. Neither is declared in the shown headers, so both are skipped.
// Expected values are kept here for when the declarations exist:
//   B_0062#45: Nomad level 22 -> knows Charm of Opening == false
//   B_0062#46: Nomad level 23 -> knows Charm of Opening == true

// SKIP B_0062#45: no declaration for Character.Create or a spellbook Known(Character, spells) function in the shown headers (expected false)
// SKIP B_0062#46: no declaration for Character.Create or a spellbook Known(Character, spells) function in the shown headers (expected true)

