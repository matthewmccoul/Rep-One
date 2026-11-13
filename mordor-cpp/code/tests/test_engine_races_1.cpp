#include "test_support.h"
#include "mordor/races.h"

// SKIP B_0062#17: Comparing the nine Races statistics/resistance/size table rows field by field against RaceData.All has no equivalent function in the headers. The table rows and RaceDef field names are not exposed by any function shown.
// SKIP B_0062#18: Counting races with zero field mismatches also requires the statistics table comparison, which has no equivalent function in the headers.
// SKIP B_0064#3: Counting entries in the race data table needs a size accessor on GameData.races (Vec<RaceDef>). The Vec<T> interface is not shown in the headers, so the count cannot be asserted without inventing an API.
// SKIP A_03#10: The character importer (RaceIndex, Alignment, sex byte, Age, Stats parsing) has no equivalent function in races.h or the other headers shown.
