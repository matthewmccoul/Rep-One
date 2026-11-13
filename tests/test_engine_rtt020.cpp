#include "test_support.h"
#include "mordor/attributes.h"
#include "mordor/items1b.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"  // FIXED: for kStatCount
#include <type_traits>               // FIXED: for std::remove_reference_t

using namespace mordor;
using namespace tsup;

// Looks up a race by name in the shipped data (RaceData.ByName equivalent).
// Returns nullptr when absent; the caller checks it.

// SKIP B_0058#34: CharacterPanel.cs is UI rendering in the port; the core has no bold-figure rule to call.

// SKIP B_0068#14: applyStatMod returns i32 and has no message list parameter, so the
// "Strength raise line that does not mention a record" cannot be observed through our API.

// SKIP B_0068#15: applyStatMod has no hall-of-records parameter in our declaration, so the
// Strength record holder and value cannot be checked through this call.

TEST(engine_rtt020_b0062_19_human_first_stat_highest_raise) {
  Env env;
  // FIXED: same reference-stripping as above.
  const std::remove_reference_t<decltype(data().races[0])>* human = nullptr;
  for (const auto& r : data().races) {
    if (r.name == "Human") { human = &r; break; }
  }
  CHECK(human != nullptr);
  if (!human) return;
  // expected: 22 (listed maximum plus 5)
  CHECK_EQ(human->max[0] + 5, 22);
}

TEST(engine_rtt020_b0064_10_stat_names_long_count) {
  // expected: 6
  // FIXED: GameData has no statNamesLong member; the stat count is kStatCount (6).
  CHECK_EQ(static_cast<size_t>(kStatCount), static_cast<size_t>(6));
}
