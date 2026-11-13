#include <cmath>
#include <string>

#include "test_support.h"
#include "mordor/misc.h"

using namespace mordor;

namespace {

bool hasText(const Str& s, const std::string& needle) {
    return std::string(s).find(needle) != std::string::npos;
}

bool strIs(const Str& s, const std::string& want) {
    return std::string(s) == want;
}

}  // namespace

// ---------------------------------------------------------------- Asmodeus (R1, R3, Asmodeus log line)

// ---------------------------------------------------------------- Teleporter trap (R6-R8, wall flags)

TEST(teleport_constants_match_srd) {
    CHECK_EQ(kTeleportMaxTries, 10000);
    CHECK_EQ(kTeleportAxisMin, 1);
    CHECK_EQ(kTeleportAxisMax, 30);
    CHECK_EQ(kWallBitRock, 19);
    CHECK_EQ(kWallBitTeleporter, 14);
    CHECK_EQ(kWallMaskRock, u32{1} << 19);
    CHECK_EQ(kWallMaskTeleporter, u32{1} << 14);
}

// ---------------------------------------------------------------- Hall of Records (R16-R17, check formula)

TEST(hall_seed_table_matches_srd_starting_records) {
    CHECK_EQ(kRecordKindCount, 12);
    CHECK(kHallSeeds[0].kind == RecordKind::Strength);
    CHECK_EQ(kHallSeeds[0].value, i64{14});
    CHECK(std::string(kHallSeeds[0].holder) == "Crashland");
    CHECK_EQ(kHallSeeds[1].value, i64{15});
    CHECK(std::string(kHallSeeds[1].holder) == "Mager");
    CHECK_EQ(kHallSeeds[2].value, i64{18});
    CHECK(std::string(kHallSeeds[2].holder) == "Theshal");
    CHECK_EQ(kHallSeeds[6].value, i64{1});
    CHECK(kHallSeeds[6].kind == RecordKind::Deadliest);
    CHECK_EQ(kHallSeeds[7].value, i64{1322451});
    CHECK_EQ(kHallSeeds[8].value, i64{5466453});
    CHECK(std::string(kHallSeeds[8].holder) == "Nimblefingers");
    CHECK_EQ(kHallSeeds[10].value, i64{6});
    CHECK(std::string(kHallSeeds[10].holder) == "Ge'nal");
    CHECK_EQ(kOriginalRecordDate, i64{0});
}

// ---------------------------------------------------------------- Monster power (R20)

// ---------------------------------------------------------------- Skill indexes (R21-R22)

// ---------------------------------------------------------------- Pluralisation (R25-R26)

// ---------------------------------------------------------------- Currency (R27-R28)

// ---------------------------------------------------------------- Extended precision (R29)

