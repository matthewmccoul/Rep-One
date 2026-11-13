#include "test_support.h"

using namespace mordor;

// t111 batch: none of the four cases can be checked against the declared API yet.
// Cases B_0064#117/#118/#119 need a round-paced combat simulation (CombatEngine::roundPaced,
// monsters, hit sounds), which is not declared. B_0065#74 needs a party with two paralysed
// characters, the cure-paralysis spell cast through SpellThrown::apply, and paralysis
// round counters, which are not declared.

// SKIP B_0064#117: CombatEngine round-paced simulation and stoning state are not declared in our API
// SKIP B_0064#118: HitSounds / round-paced combat log is not declared in our API
// SKIP B_0064#119: fight.Monsters[0].Hits after RoundPaced needs the combat engine, not declared in our API
// SKIP B_0065#74: SpellThrown::apply, party paralysis fields (ParalyzedRounds, ParalysisLevel) and Unparalyse are not declared in our API
