#include <cmath>

#include "test_support.h"
// FIXED: "mordor/rules.h" does not exist in the project headers, so the include is removed. complicationChance, fn13ED and fn12A1 are not declared in any header in the API INDEX or STATE HEADERS digest, so the tests that call them are disabled with #if 0 until a header declaring them exists. No implementation is invented here.

using namespace mordor;

#if 0 // FIXED: complicationChance, fn13ED and fn12A1 are undeclared; re-enable once a header declares them.

// Rounds to one decimal place, as the reference assertions do.
static double rtt050RoundOne(double v) {
  return std::round(v * 10.0) / 10.0;
}

// B_0061#142: 16-year-old Human, race lifespan 100 years (MaxAge in RaceData).
// Age is given in days as 16 * 365.
TEST(engine_rtt050_complication_chance_human_16) {
  double v = rtt050RoundOne(complicationChance(16 * 365, 100));
  CHECK(std::fabs(v - 32.0) < 1e-9);
}

// B_0061#143: 16-year-old Elf, race lifespan 400 years.
TEST(engine_rtt050_complication_chance_elf_16) {
  double v = rtt050RoundOne(complicationChance(16 * 365, 400));
  CHECK(std::fabs(v - 8.0) < 1e-9);
}

// B_0061#144: 16-year-old Dwarf, race lifespan 275 years. Raw value is 11.636..., rounds to 11.6.
TEST(engine_rtt050_complication_chance_dwarf_16) {
  double v = rtt050RoundOne(complicationChance(16 * 365, 275));
  CHECK(std::fabs(v - 11.6) < 1e-9);
}

// B_0061#4: Human Nomad (guild factor 8, race factor 3).
// Fn13ED at level 79 must equal Fn12A1 at level 80 less one.
TEST(engine_rtt050_fn13ed_level79_is_fn12a1_level80_minus_one) {
  i64 lhs = fn13ED(79, 8.0f, 3.0f);
  i64 rhs = fn12A1(80, 8.0f, 3.0f) - 1;
  CHECK_EQ(lhs, rhs);
}

#endif // FIXED: end of disabled block

// SKIP B_0061#5: needs GameData guild and race ExpFactor field names (not in the declarations
// shown) and the closed-form test uses an unspecified "race multiplier" for the race factor.
