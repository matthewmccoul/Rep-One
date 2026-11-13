#include "test_support.h"
// NEED-ENGINE: the round driver (RoundPaced equivalent returning text lines), party member
// state (orders, spell points, hits), anti-magic square flag, and the random seed plumbing.
// None of these are declared in the OUR DECLARATIONS list, so no case below is expressible.

using namespace mordor;

// SKIP B_0061#96: the round driver (CombatEngine::roundPaced) and the Healer/Heal ordered-cast setup are not declared in our API.
// SKIP B_0061#97: the round driver and the Healer hits/order setup are not declared in our API.
// SKIP B_0061#99: the round driver, the Warrior guild-level setup, and the refusal text output are not declared in our API.
// SKIP B_0061#100: the round driver and the member order-kind state are not declared in our API.
// SKIP B_0061#101: comparing monster turn lines and the next random value across two rounds needs the round driver and the seeded RNG round trip, which are not declared in our API.
// SKIP B_0061#102: the round driver, the Sorcerer setup, and the anti-magic square flag are not declared in our API.
// SKIP B_0061#103: the round driver and the member order-kind and spell-point state are not declared in our API.
// SKIP B_0061#104: the round driver, the anti-magic square setup, and the line comparison across rounds are not declared in our API.
