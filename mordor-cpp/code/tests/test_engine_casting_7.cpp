#include "test_support.h"

// SKIP B_0061#100: needs CombatEngine.RoundPaced (round engine with order processing); no such function in the casting headers
// SKIP B_0061#101: needs CombatEngine.RoundPaced to compare monster turn lines and RNG state across rounds; no such function in the casting headers
// SKIP B_0061#102: needs CombatEngine.RoundPaced to produce round text ("Anti-Magic area!" line); no such function in the casting headers
// SKIP B_0061#103: needs CombatEngine.RoundPaced to apply a Firebolt order and report order kind and spell points; no such function in the casting headers
// SKIP B_0061#104: needs CombatEngine.RoundPaced to compare round lines in an anti-magic square; no such function in the casting headers
// SKIP B_0061#105: needs CombatEngine.RoundPaced to observe the RNG after a cast check; no such function in the casting headers
// SKIP B_0061#106: needs CombatEngine.RoundPaced and an anti-magic callback that receives a character name; no such function or callback in the casting headers
// SKIP B_0061#107: Rules.Fn1485Price has no equivalent in the casting headers (no rated-price function with an out parameter for this formula)
// SKIP B_0062#40: needs MdrLoader.LoadSpells reading MDATA2.MDR; no MDR loader in the provided headers
// SKIP B_0062#41: needs MdrLoader.LoadSpells reading MDATA2.MDR; no MDR loader in the provided headers

