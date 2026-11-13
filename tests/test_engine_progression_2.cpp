#include "test_support.h"
#include "mordor/progression2.h"

using namespace mordor;

// B_0061 chest experience cases, mapped to chestAward(partySize, disarmChance, monsterGuildLevel, firstCharGuildLevel).
// The 1000 base award is passed as disarmChance; monster guild level is 2 in every case.

// SKIP B_0061#142: needs RaceData MaxAge for Human; race lifespan field names are not in the provided headers
// SKIP B_0061#143: needs RaceData MaxAge for Elf; race lifespan field names are not in the provided headers
// SKIP B_0061#144: needs RaceData MaxAge for Dwarf; race lifespan field names are not in the provided headers
