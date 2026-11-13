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

