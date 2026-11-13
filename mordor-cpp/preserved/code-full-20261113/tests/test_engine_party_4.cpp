#include "test_support.h"
#include "mordor/partya.h"
#include "mordor/partyb.h"

using namespace mordor;

// SKIP B_0059#31: needs message counting on env.log; EventLog API is not in the headers.
// SKIP B_0059#32: needs ordered message capture on env.log; EventLog API is not in the headers.
// SKIP B_0059#33: the give name list is a UI list; no headers function returns it (giveAvailable returns bool only).
// SKIP B_0059#83: asks about statement order inside the original MAINMOD2.BAS source; not testable through the API.
// SKIP B_0059#84: asks about statement order inside the original MAINMOD2.BAS source; not testable through the API.
// SKIP B_0059#89: asks about the original FRMGROUP.BAS UPDATE routine; a UI/source-level behaviour, no matching function.
// SKIP B_0067#17: Party.PlanShortfall is not declared in the headers.
// SKIP B_0067#18: Party.PlanShortfall is not declared in the headers.

TEST(engine_B_0059_82_inherit_lost_flags) {
    tsup::Env env(42);

    Character first{};
    first.lost.level = true;
    first.lost.position = false;
    first.lost.facing = true;

    Character newcomer{};
    newcomer.lost.level = false;
    newcomer.lost.position = true;
    newcomer.lost.facing = false;

    inheritLostFlags(env.ctx, first, newcomer);

    CHECK(newcomer.lost.level == true);
    CHECK(newcomer.lost.position == false);
    CHECK(newcomer.lost.facing == true);
}

TEST(engine_B_0059_85_dead_joiner_refused_in_combat) {
    tsup::Env env(42);

    Party party{};
    party.size = 1;
    party.members[0].vitals.curHits = 15;

    Character joiner{};
    joiner.name = "Ghost";  // distinct from the leader's blank name, so the joiner is not "already in party"
    joiner.deathState = DeathState::Dead;
    joiner.vitals.curHits = 0;

    PartyJoinOutcome outcome = joinParty(env.ctx, party, joiner, true);
    CHECK(outcome == PartyJoinOutcome::DeadInCombat);
}

TEST(engine_B_0059_85_living_joiner_runs_sub0bde) {
    tsup::Env env(42);

    Party party{};
    party.size = 1;
    party.members[0].vitals.curHits = 15;

    Character joiner{};
    joiner.vitals.curHits = 15;

    JoinCheck check = checkJoin(env.ctx, party, joiner, true, true, true);
    CHECK(check.runSub0BDE == true);
}
