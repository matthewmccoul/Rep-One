// Batch t124: EncounterGenerator.Generate assertions (reference B_0066).
// NEED: encounter generator API (EncounterGenerator / Generate), Encounter (Foes[i].Hits, lead/group fields, Count, TreasureClass), Monster and guild-level table entries, Floor (AreaIdAt, HeaderAreaCount), and a null-floor overload. None of these appear in the declarations provided.
#include "test_support.h"

using namespace mordor;

// SKIP B_0066#74: needs Generate() and the encounter lead field; generator API not in the provided declarations (NEED above).
// SKIP B_0066#75: needs Generate() and the encounter lead field; generator API not in the provided declarations (NEED above).
// SKIP B_0066#76: needs Generate() with monster guild levels and an area-below-18 rule; generator API not in the provided declarations (NEED above).
// SKIP B_0066#77: needs Generate() with a fourth-index table monster and lead index; generator API not in the provided declarations (NEED above).
// SKIP B_0066#78: needs Generate() with area 18 gating and guild levels; generator API not in the provided declarations (NEED above).
// SKIP B_0066#79: needs Generate() with null floor and Encounter foe Hits; generator API not in the provided declarations (NEED above).
// SKIP B_0066#85: needs Generate() group-front and roll checks over Encounter groups; generator API not in the provided declarations (NEED above).
// SKIP B_0066#86: needs Generate() group slot hits and Monster Hits +/- (Int(Hits/4)+1) check; generator API not in the provided declarations (NEED above).
