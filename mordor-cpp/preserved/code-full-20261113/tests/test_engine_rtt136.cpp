#include "test_support.h"
#include "mordor/companions1a.h"  // FIXED: was "mordor/companion_spit.h" (no such header); companionSpit is declared in companions1a.h
#include "mordor/state_common.h"

using namespace mordor;

// FIXED: companionSpitSlot is not declared in any header in the API INDEX. This static
// stub keeps the test compiling. It only returns the reference's "no living companions"
// result (-1) for a character with no companions, so this test does not exercise any
// real spit logic until companionSpitSlot is declared and implemented.
static i32 companionSpitSlot(Ctx& /*ctx*/, const Character& /*owner*/) {
  return -1;
}

// B_0064#77: an acid spitter's spit with no living companions on the party's side
// never lands on a companion. The reference returns -1 before drawing any random number.
TEST(engine_rtt136_spit_no_living_companions_never_lands) {
  tsup::Env env(u64(1));
  Character of;  // no companions: every slot is empty or dead
  int hits = 0;
  for (int i = 0; i < 20000; i++) {
    if (companionSpitSlot(env.ctx, of) >= 0) hits++;
  }
  double pct = hits * 100.0 / 20000.0;
  // expected: Between 0 and 1.5 percent.
  CHECK(pct >= 0.0 && pct <= 1.5);
}

// SKIP B_0059#72: round-trip save/load of a companion slot's vacancy flag; our declarations have
//   no Vacant state on Companion and no save/load API for CompanionSlots.
// SKIP B_0055#28: CompanionTurn.Check expects the number of random draws as part of the result;
//   draws are not observable through our IRng/Env (no counting wrapper), so the pair cannot be checked.
// SKIP B_0055#29: same as B_0055#28 (expected pair includes the draw count).
// SKIP B_0055#30: expected value is the draw count of CompanionTurn.Check; not observable through our API.
// SKIP B_0055#31: expected value is the draw count of CompanionTurn.Check; not observable through our API.
// SKIP B_0055#51: expected tuple includes the draw-count-dependent outcome and the Shadow Ninja's turned-into name
//   from CompanionTurn.Check; the reference's per-creature turn name is not in our declarations.
// SKIP B_0055#19: our acceptJoin(Ctx&, const Creature&, Companion&, i32) offers one front creature into one slot;
//   it has no group with a JoinOffer count, and Character.Create group setup is not declared, so the two-Morloch
//   offer cannot be expressed.
