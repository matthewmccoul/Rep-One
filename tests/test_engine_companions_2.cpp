#include "test_support.h"
#include "mordor/companions1b.h"

using namespace mordor;

// B_0055#28 is partly testable: the turn outcome is asserted below.
// The draw count (2) is not observable through the headers, so it is skipped.
TEST(engine_B_0055_28_morloch_evil_own_neutral_creature_holds) {
    tsup::Env env({0.5, 0.9});
    Companion c;
    c.alignment = CompanionAlignment::Evil;
    c.bindingLevel = 5;
    Vec<Creature> foes;
    // expected False: does not turn
    CHECK(companionTurnCheck(env.ctx, c, Align::Good, CharAlign::Neutral, false, foes, true)
          != TurnOutcome::Joins);
}
// SKIP B_0055#28: draw count (2) not observable; ScriptedRng exposes no draw counter in the headers.

// B_0055#29 is partly testable. The creature is the Good CharAlign, which covers
// the first MDATA5 creature with AlignmentValue 0. The turn outcome is asserted below.
TEST(engine_B_0055_29_good_creature_good_owner_holds) {
    tsup::Env env({0.1, 0.1});
    Companion c;
    c.alignment = CompanionAlignment::Good;
    c.bindingLevel = 5;
    Vec<Creature> foes;
    // expected False: does not turn
    CHECK(companionTurnCheck(env.ctx, c, Align::Good, CharAlign::Good, false, foes, true)
          != TurnOutcome::Joins);
}
// SKIP B_0055#29: draw count (2) not observable; ScriptedRng exposes no draw counter in the headers.

// SKIP B_0055#30: random draw count not observable through the headers.
// SKIP B_0055#31: random draw count (0) not observable through the headers.

TEST(engine_B_0055_32_morloch_neutral_turns) {
    tsup::Env env({0.9, 0.1});
    Companion c;
    c.alignment = CompanionAlignment::Neutral;
    c.bindingLevel = 5;
    Vec<Creature> foes;
    // expected true: turns (Neutral turns when second roll gives 10 < 15)
    CHECK(companionTurnCheck(env.ctx, c, Align::Good, CharAlign::Neutral, false, foes, true)
          == TurnOutcome::Joins);
}

TEST(engine_B_0055_33_good_companion_good_foe_not_refused) {
    tsup::Env env({0.9, 0.9});
    Companion c;
    c.alignment = CompanionAlignment::Good;
    Creature foe;
    foe.align = CharAlign::Good;  // group Alignment 0
    foe.alive = true;
    Vec<Creature> foes;
    foes.push_back(foe);
    // expected false: not refused
    CHECK(companionTurnCheck(env.ctx, c, Align::Good, CharAlign::Good, true, foes, true)
          != TurnOutcome::RefusedByFoe);
}

TEST(engine_B_0055_34_good_companion_evil_foe_refused) {
    tsup::Env env({0.9, 0.9});
    Companion c;
    c.alignment = CompanionAlignment::Good;
    Creature foe;
    foe.align = CharAlign::Evil;  // group Alignment 2
    foe.alive = true;
    Vec<Creature> foes;
    foes.push_back(foe);
    // expected True: refused. The refusal message is not asserted because
    // the headers do not expose a query API for EventLog.
    CHECK(companionTurnCheck(env.ctx, c, Align::Good, CharAlign::Good, true, foes, true)
          == TurnOutcome::RefusedByFoe);
}

TEST(engine_B_0055_35_morloch_neutral_good_companion_evil_foe_not_refused) {
    tsup::Env env({0.9, 0.9});
    Companion c;
    c.alignment = CompanionAlignment::Good;
    Creature foe;
    foe.align = CharAlign::Evil;
    foe.alive = true;
    Vec<Creature> foes;
    foes.push_back(foe);
    // expected false: not refused
    CHECK(companionTurnCheck(env.ctx, c, Align::Good, CharAlign::Neutral, true, foes, true)
          != TurnOutcome::RefusedByFoe);
}

// SKIP B_0055#44: Settle(monsters) has no equivalent in the headers. settleLegacyCompanionId returns no count.
// SKIP B_0055#51: needs sequential RNG state across two checks, plus name resolution ("Shadow Ninja") not available in the headers.
