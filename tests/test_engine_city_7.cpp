#include "test_support.h"
#include "mordor/city2a.h"

using namespace mordor;

// SKIP B_0057#99: Librarian.DescribeItem has no equivalent function in the headers (item page text).

TEST(engine_B_0057_100_file_restock_mostly_keeps_name_and_sets_grade) {
    tsup::Env env(1);
    LibraryRecord rec;
    rec.known = 3;            // Mostly
    rec.name = "Taker";
    libraryFileRestock(env.ctx, rec, 1000);
    CHECK(rec.name == Str("Taker"));
    CHECK_EQ(rec.known, 4);
}

TEST(engine_B_0057_101_file_restock_partial_grade0_clears_name_sets_grade) {
    tsup::Env env(1);
    LibraryRecord rec;
    rec.known = 0;            // grade 0
    rec.name = "Taker";
    libraryFileRestock(env.ctx, rec, 1000);
    CHECK(rec.name == Str(""));
    CHECK_EQ(rec.known, 4);
}

// SKIP B_0057#102: needs a seeded restock run over 199 seeds and summing Library NSeen; no simulation/filing hook in headers.
// SKIP B_0057#103: needs the seeded restock run plus the filed Library records; no simulation hook in headers.
// SKIP B_0057#104: needs the seeded restock run's shelved items; only per-item shelvingAllowed exists, not a run.
// SKIP B_0057#105: needs a seeded restock simulation across seeds 1..199.
// SKIP B_0057#106: needs two identical seeded restock runs comparing shelves; simulation required.
// SKIP B_0058#17: LibrarySearch.Find has no equivalent function in the headers (UI list search).
// SKIP B_0058#18: LibrarySearch.Find has no equivalent function in the headers (UI list search).
