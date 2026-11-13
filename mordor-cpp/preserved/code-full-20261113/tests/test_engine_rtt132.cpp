#include "test_support.h"

// All cases in batch t132 exercise CombatEngine.RoundPaced: a multi-round
// combat driver with companions, owners, breath mishaps and friendly-fire
// captions. Our declarations include BreathResult, MishapRow and
// SpecialAttackKind, but no round driver, party/companion state, or caption
// event output for that loop. Until those are declared (see NEED above),
// none of these cases can be expressed against our API.

// SKIP B_0064#111: needs round driver + caption event output (NEED above); cannot count 'You were accidently hit for' against m0 hits
// SKIP B_0064#112: needs round driver + caption event output (NEED above); cannot count caption occurrences for non-m0 hits
// SKIP B_0064#110: needs round driver + party/owner state (NEED above); cannot count 'mN:' lines naming their owner
// SKIP B_0064#74: needs round driver with breath firing at monsters (NEED above); cannot count foe-hit drops with no mishap line
// SKIP B_0064#107: needs round driver + party of three with companions (NEED above); cannot count 'accidently hit "m0"' lines
// SKIP B_0064#108: needs round driver + party of three with companions (NEED above); cannot count 'accidently hit "m2"' lines
// SKIP B_0064#75: needs round driver + breath mishap caption output (NEED above); cannot count 'accidently breathed on' lines
// SKIP B_0064#109: needs round driver + companion owner links (NEED above); cannot count 'accidently hit' lines with 'belonging to'

