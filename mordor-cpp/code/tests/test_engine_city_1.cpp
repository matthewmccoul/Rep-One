#include "test_support.h"
#include "mordor/city1.h"

// Confinement cases: pen counts are passed as {Good, Neutral, Evil, unused}.
// Result indices are 0-based Good/Neutral/Evil, matching the reference's
// AlignmentChoice output.
// Store cases: counts are indexed by ItemAlign (0 Unaligned, 1 Good, 2 Neutral, 3 Evil).

// SKIP B_0055#9: needs MdrLoader.LoadMonsters over MDATA5.MDR to pick a creature; no loader in headers.
// SKIP B_0055#10: needs Confinement.Add to build pen state and PriceToBuy per alignment; the headers only expose PenBook.byRef and penBuyPrice(base, count) with no monster base value available.
