#include "test_support.h"
#include "mordor/types.h"
#include "mordor/math_util.h"

using namespace mordor;

// B_0065#33: no declaration for CombatEngine::RoundPaced or its chest-open line output.
// SKIP B_0065#33: CombatEngine::RoundPaced and chest-open line output are not declared in our API.

// B_0065#30: no declaration for CombatEngine::ChestRoundsOf.
// SKIP B_0065#30: CombatEngine::ChestRoundsOf (m1 chest count) is not declared in our API.

// B_0066#117: no declaration for EncounterGenerator::ClearedByJoining.
// SKIP B_0066#117: EncounterGenerator::ClearedByJoining is not declared in our API.

// B_0066#119: no declaration for EncounterGenerator::ClearedByJoining.
// SKIP B_0066#119: EncounterGenerator::ClearedByJoining is not declared in our API.

// B_0066#120: needs ClearedByJoining, CombatEngine gold/chest state, and the chest class; none are declared.
// SKIP B_0066#120: ClearedByJoining + CombatEngine chest and gold state are not declared in our API.

// B_0066#118: needs ClearedByJoining and the monGetGold halving (goldAfterClass) with a party gold total; none are declared.
// SKIP B_0066#118: ClearedByJoining and goldAfterClass (monGetGold halving) are not declared in our API.

// B_0066#114: needs EncounterGenerator::InRoom, Floor, DungeonState, and Encounter; none are declared.
// SKIP B_0066#114: EncounterGenerator::InRoom with Floor/DungeonState/Encounter is not declared in our API.

// B_0059#17: needs GiveCommand::GoldAnswer and GoldFromTag; none are declared.
// SKIP B_0059#17: GiveCommand::GoldAnswer and GiveCommand::GoldFromTag are not declared in our API.

