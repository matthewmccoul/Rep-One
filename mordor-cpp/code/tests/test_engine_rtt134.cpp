// Batch t134: reference assertions from CombatSounds.OfLines and CompanionCarry.Run.
// No TEST bodies yet: our code declares none of the APIs these cases call (see NEED above).

#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0065#8: CombatSounds::OfLines not declared in our code (needs CombatLine/OpensChest, Character watched, noPartySound)
// SKIP B_0065#12: CombatSounds::OfLines not declared in our code (needs CombatLine, Character, noPartySound)
// SKIP B_0065#71: CombatSounds::OfLines not declared in our code (needs CombatLine, Character victim, noPartySound)
// SKIP B_0065#7: CombatSounds::OfLines not declared in our code (needs CombatLine, Character striker owner, noPartySound)
// SKIP B_0065#11: CombatSounds::OfLines not declared in our code (needs CombatLine, Character striker owner, noPartySound)
// SKIP B_0065#72: CombatSounds::OfLines not declared in our code (needs CombatLine, Character owner, noPartySound)
// SKIP B_0064#87: CompanionCarry::Run not declared in our code (needs Character with Companions slots, Companion Hits/MaxHits, monsterOf lookup)
// SKIP B_0064#102: CompanionCarry::Run not declared in our code (needs Character with Companions slots, Companion Hits/MaxHits, monsterOf lookup, CompanionName)
