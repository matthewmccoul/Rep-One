#include "test_support.h"

using namespace mordor;

// t131: the provided headers declare no CombatEngine, RoundPaced, Party, Monster,
// Character, companion or spell-cast API, so none of the B_0065 cases below can be
// expressed against our code yet. Each is recorded as SKIP until those declarations exist.

// SKIP B_0065#64: needs CombatEngine::RoundPaced with a healer companion (school 9, floor 2, level 999) and a seeded 300-round loop; no combat engine or companion-cast API is declared
// SKIP B_0065#65: needs CombatEngine::RoundPaced with m1's level 1 caster, Blue Flame on the party and a 600-round seeded loop; no combat engine or mishap API is declared
// SKIP B_0065#66: depends on the mishap line text produced by RoundPaced in the B_0065#65 scenario; no mishap line/CombatLine API is declared
// SKIP B_0065#67: depends on companion-hit tracking in the B_0065#65 scenario; no companion-hit or mishap victim API is declared
// SKIP B_0065#68: needs a 1500-round seeded Blue Flame mishap killing a one-hit undead companion; no combat engine, monster-companion or kill-count API is declared
// SKIP B_0065#70: needs the death-cry line ordering from the B_0065#68 kills; no CombatLine ordering API is declared
// SKIP B_0065#59: needs CombatEngine::RoundPaced with ItemOrders (owner order to cast Firebolt with a 5-charge item, paralysed owner); no item-order or cast-round API is declared
// SKIP B_0065#62: needs the run-out check over ItemOrders rounds (cast after item ran out, owner Fight state, order held); no item-order or combat-state API is declared
