#include "test_support.h"
#include "mordor/city2a.h"

using namespace mordor;

// SKIP B_0057#99: Librarian.DescribeItem has no equivalent function in the headers (item page text).

// SKIP B_0057#102: needs a seeded restock run over 199 seeds and summing Library NSeen; no simulation/filing hook in headers.
// SKIP B_0057#103: needs the seeded restock run plus the filed Library records; no simulation hook in headers.
// SKIP B_0057#104: needs the seeded restock run's shelved items; only per-item shelvingAllowed exists, not a run.
// SKIP B_0057#105: needs a seeded restock simulation across seeds 1..199.
// SKIP B_0057#106: needs two identical seeded restock runs comparing shelves; simulation required.
// SKIP B_0058#17: LibrarySearch.Find has no equivalent function in the headers (UI list search).
// SKIP B_0058#18: LibrarySearch.Find has no equivalent function in the headers (UI list search).
