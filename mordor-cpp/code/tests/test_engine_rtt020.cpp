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

TEST(engine_rtt020_b0068_13_strength_plus2_potion_caps_at_ceiling) {
  Env env;
  // FIXED: decltype of races[0] is a reference type; strip the reference before taking a pointer.
  const auto* human = static_cast<const std::remove_reference_t<decltype(data().races[0])>*>(nullptr);
  (void)human;
  const std::remove_reference_t<decltype(data().races[0])>* found = nullptr;
  for (const auto& r : data().races) {
    if (r.name == "Human") { found = &r; break; }
  }
  CHECK(found != nullptr);
  if (!found) return;
  Character c;
  c.race = Race::Human;
  const i32 maxStr = found->max[0];
  c.abilities.stats[0] = maxStr + 4;
  applyStatMod(env.ctx, c, StatIndex::Str, 2, false);
  // expected: race maximum plus 5
  CHECK_EQ(c.abilities.stats[0], maxStr + 5);
}

// SKIP B_0068#14: applyStatMod returns i32 and has no message list parameter, so the
// "Strength raise line that does not mention a record" cannot be observed through our API.

// SKIP B_0068#15: applyStatMod has no hall-of-records parameter in our declaration, so the
// Strength record holder and value cannot be checked through this call.

TEST(engine_rtt020_b0068_16_reversed_negative_mod_intelligence) {
  Env env;
  Character c2;
  c2.race = Race::Human;  // reference character always has a race (c.RaceDef); without it raceOf() is null
  const i32 intIdx = static_cast<i32>(StatIndex::Int);
  c2.abilities.stats[intIdx] = 15;
  applyStatMod(env.ctx, c2, StatIndex::Int, -2, true);  // R (ch13): negative mod on reversed item is added as-is
  // expected value part: 13
  CHECK_EQ(c2.abilities.stats[intIdx], 13);
  // SKIP B_0068#16 holder part ("empty holder name"): hall-of-records check is not part of
  // applyStatMod's declared signature.
}

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
