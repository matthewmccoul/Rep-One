#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/rng.h"

using namespace mordor;

// No combat-round functions (PlayerSwings / MonsterAttacks / Dies / SetDead) are declared
// in the headers provided, and no Character, Monster or group-state structs are declared
// with the fields these cases read. Every case is therefore skipped until those declarations exist.

// SKIP B_0056#36: no declaration for MonsterAttacks (round attack) or the lone-dead-character party state; cannot count non-empty output lines.
// SKIP B_0066#137: no declaration for MonsterAttacks or PartyWiped state; cannot check that the hit caption is yielded after the party is wiped.
// SKIP B_0066#139: no declaration for Character carry/paralysis state (CarryingCharacter, paralysed) or the kill path that sets them.
// SKIP B_0056#35: no declaration for MonsterAttacks, Monster groups, or a ScriptedRng draw counter; cannot count random draws in the monsters' turn.
// SKIP B_0068#4: no declaration for MonsterAttacks carrier handling or the poison/disease/paralysis/spell-resistance fields on a picked-up body.
// SKIP B_0068#3: no declaration for MonsterAttacks, seeded-generator death flow, or the character ailment/carry fields.
// SKIP B_0065#112: no declaration for PlayerSwings or per-monster kill counters (m0 kills).
// SKIP B_0065#113: no declaration for PlayerSwings or per-monster kill counters (m1 kills).

