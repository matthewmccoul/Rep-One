#include "test_support.h"
#include "mordor/city2a.h"
#include "mordor/city2b.h"
#include "mordor/state_common.h"

using namespace mordor;

namespace {

// Day serials: 2026-10-15 is day 20741 and 2026-10-18 is day 20744 counted
// from 1970-01-01. Convert to UTC milliseconds for libraryDaySerial.
i64 utcMillisForDay(i64 day) {
    return day * 86400000LL;
}

}  // namespace

// ---- B_0057#18: FileItem at IdLevel.Mostly on an empty record -> grade 3
TEST(engine_B_0057_18_item_mostly_grade) {
    tsup::Env env(1);
    LibraryRecord rec{};
    // Item id 1 stands in for the first non-zero-Id item; grade depends only on IdLevel.
    libraryFileItem(env.ctx, rec, Str{}, IdLevel::Mostly, 1);
    CHECK_EQ(rec.known, 3);
}

// ---- B_0057#19: times found, last-seen, location after FileItem (Mostly) -> (0, 0, "")
TEST(engine_B_0057_19_item_mostly_counts_and_place) {
    tsup::Env env(1);
    LibraryRecord rec{};
    libraryFileItem(env.ctx, rec, Str{}, IdLevel::Mostly, 1);
    CHECK_EQ(rec.nSeen, 0);
    CHECK_EQ(rec.lastSeen, static_cast<i64>(0));
    CHECK(rec.location == "");
}

// ---- B_0057#20: record stored under key 'i:<Id>' after a Partial find
// SKIP B_0057#20: the headers expose no keyed store lookup; LibraryState holds items by id in a Vec and
// libraryItemKey only returns a key string, so existence under 'i:<Id>' cannot be asserted.

// ---- B_0057#21: grade after a Partial find
// SKIP B_0057#21: libraryFileFind needs fromMonsterId = the lead's Id, but Character has no Id field,
// so the precondition cannot be set up from the headers.

// ---- B_0057#22: Library lists the item after a Partial find
// SKIP B_0057#22: no Store.KnowsItem function exists in the headers (libraryItemListed takes a grade, not an item).

// ---- B_0057#23: Library lists the item after a later Mostly filing
// SKIP B_0057#23: no Store.KnowsItem function exists in the headers (libraryItemListed takes a grade, not an item).

// ---- B_0057#95: restock filing grade -> 4
TEST(engine_B_0057_95_restock_grade) {
    tsup::Env env(1);
    LibraryRecord rec{};
    // libraryFileRestock takes no item; the Floor>0 / Cursed 0 selection happens in the store, not here.
    libraryFileRestock(env.ctx, rec, libraryDaySerial(utcMillisForDay(20744)));
    CHECK_EQ(rec.known, 4);
}

// ---- B_0057#96: restock record name -> empty
TEST(engine_B_0057_96_restock_name_empty) {
    tsup::Env env(1);
    LibraryRecord rec{};
    libraryFileRestock(env.ctx, rec, libraryDaySerial(utcMillisForDay(20744)));
    CHECK(rec.name == "");
}

// ---- B_0057#97: restock record monster Id and last-seen date -> (-2, serial of 2026-10-18)
TEST(engine_B_0057_97_restock_monid_and_last_seen) {
    tsup::Env env(1);
    LibraryRecord rec{};
    const i64 today = libraryDaySerial(utcMillisForDay(20744));
    libraryFileRestock(env.ctx, rec, today);
    CHECK_EQ(rec.monId, kLibraryRestockMonId);
    CHECK_EQ(rec.lastSeen, today);
}

// ---- B_0057#98: restock record location and times found -> ('?', 1)
TEST(engine_B_0057_98_restock_location_and_count) {
    tsup::Env env(1);
    LibraryRecord rec{};
    libraryFileRestock(env.ctx, rec, libraryDaySerial(utcMillisForDay(20744)));
    CHECK(rec.location == "?");
    CHECK_EQ(rec.nSeen, 1);
}
