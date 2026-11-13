#include <cmath>
#include "test_support.h"
#include "mordor/rng.h"

using namespace mordor;

namespace {

// Round half to even (SRD round_half_even), written locally so the test does
// not depend on the signature of math_util.h.
i64 roundHalfEvenRef(double x) {
  double f = std::floor(x);
  double d = x - f;
  i64 base = static_cast<i64>(f);
  if (d > 0.5) return base + 1;
  if (d < 0.5) return base;
  return (base % 2 == 0) ? base : base + 1;
}

// Raw chute formula from B_0066#57/#58: floor(Rnd * (level * 10.5)) * ln(raceSize + 2) + 5.
// The first Rnd of Rng(seed) is used, as ChuteDamage would draw it.
double chuteRaw(u64 seed, int level, int raceSize) {
  Rng r(seed);
  double rnd = r.nextDouble();
  return std::floor(rnd * (level * 10.5)) * std::log(raceSize + 2.0) + 5.0;
}

}  // namespace

// B_0066#58: over seeds 0..299 the level 7, size 2 raw chute formula must give
// truncation and round-half-even that disagree for at least one seed.
TEST(engine_rtt071_chute_raw_truncation_vs_round_even_disagree) {
  int differing = 0;
  for (u64 s = 0; s < 300; ++s) {
    double raw = chuteRaw(s, 7, 2);
    if (static_cast<i64>(raw) != roundHalfEvenRef(raw)) ++differing;
  }
  CHECK(differing >= 1);
}

// B_0066#57: for seeds 0..299 (level 7, race size 2) where truncation and
// round-half-even differ, ChuteDamage must return the rounded value for every
// such seed.
// SKIP engine_rtt071 B_0066#57: needs the chuteDamage(Ctx&, const Character&, level)
// path with a Character whose race has Size 2; the race name and Character
// construction for this are not in the declarations shown.

// SKIP B_0066#68: our pitDamage(i32 pitSize) takes no IRng and no dungeon level,
// so it cannot express PitDamage(rng, level 5, raceSize 2) over seeds 0..299.

// SKIP B_0058#12: asks about the C# DungeonWindow.axaml.cs UI source
// (ChestTeleport calling PruneTheDead); not a rule in the core port.

// SKIP B_0066#52, B_0066#53, B_0066#49, B_0066#50: all four exercise Chute.Land
// (on-screen/leader-floor message, levitation, hit deduction). Blocked on NEED
// below; no Land-equivalent declaration exists yet.

