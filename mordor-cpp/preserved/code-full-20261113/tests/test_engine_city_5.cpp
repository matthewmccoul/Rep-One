#include "test_support.h"
#include "mordor/city2a.h"

using namespace mordor;

// 2026-10-15T00:00:00Z in UTC millis (20741 days after 1970-01-01).
namespace {
constexpr i64 kOct15Millis = 1792022400000LL;

// First filing: item found by a monster (Id 10) on level 4, not lost, fully identified.
void fileFirst(Ctx& ctx, LibraryRecord& rec, i64 day) {
    libraryFileFind(ctx, rec, Str("Tester"), IdLevel::All, 42, 10, day, false, false, Str("4"));
}

// Second filing: partially identified, monster Id -2, level 6, not lost.
void fileSecond(Ctx& ctx, LibraryRecord& rec, i64 day) {
    // Reference: SelfCheckProbe.LibraryFind.cs:41 passes lostLevel = true (the seventh argument).
    libraryFileFind(ctx, rec, Str("Tester"), IdLevel::Partial, 42, -2, day, true, false, Str("6"));
}
}  // namespace

TEST(engine_B_0057_8_date_is_serial_of_2026_10_15) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    CHECK_EQ(rec.lastSeen, libraryDaySerial(kOct15Millis));
}

TEST(engine_B_0057_9_location_is_level_4) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    CHECK(rec.location == Str("4"));
}

TEST(engine_B_0057_10_found_once) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    CHECK_EQ(rec.nSeen, 1);
}

// SKIP B_0057#11: Librarian.DescribeItem page text is not exposed by any function in the headers (needs UI/text rendering).

TEST(engine_B_0057_12_location_after_second_filing) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    fileSecond(env.ctx, rec, day + 5);
    CHECK(rec.location == Str("?"));
}

TEST(engine_B_0057_13_monster_id_after_second_filing) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    fileSecond(env.ctx, rec, day + 5);
    CHECK_EQ(rec.monId, -2);
}

TEST(engine_B_0057_14_date_is_first_plus_5) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    fileSecond(env.ctx, rec, day + 5);
    CHECK_EQ(rec.lastSeen, day + 5);
}

TEST(engine_B_0057_15_found_twice) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    fileSecond(env.ctx, rec, day + 5);
    CHECK_EQ(rec.nSeen, 2);
}

TEST(engine_B_0057_16_grade_kept_at_4_after_partial_filing) {
    tsup::Env env(7);
    LibraryRecord rec{};
    const i64 day = libraryDaySerial(kOct15Millis);
    fileFirst(env.ctx, rec, day);
    fileSecond(env.ctx, rec, day + 5);
    CHECK_EQ(rec.known, 4);
}

// SKIP B_0057#17: Librarian.DescribeItem page text is not exposed by any function in the headers (needs UI/text rendering).
