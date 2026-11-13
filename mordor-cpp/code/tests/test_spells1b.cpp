#include "test_support.h"
#include "mordor/spells1b.h"

using namespace mordor;

// Enum-to-int helper so CHECK_EQ never needs operator<< on scoped enums.
template <typename E>
static i32 ev(E e) {
    return static_cast<i32>(e);
}

// R60: OpensLockUpTo(effect) = effect - 29
TEST(opens_lock_up_to_maps_effects_30_to_32) {
    CHECK_EQ(kLockOpenBaseEffect, 29);
    CHECK_EQ(opensLockUpTo(30), 1);
    CHECK_EQ(opensLockUpTo(31), 2);
    CHECK_EQ(opensLockUpTo(32), 3);
}

// Module constants stated in the SRD and header.
TEST(module_constants_match_srd) {
    CHECK_EQ(kSanctuaryDays, 365);
    CHECK_EQ(kLocateLevelRange, 15);
    CHECK_EQ(kCityArea, 0);
    CHECK_EQ(kPortalPartyWipeSlotLimit, 2);
    CHECK_EQ(kFateUnusedDraw, 3);
}

// R64 / Table A: utility effect ids 11, 12, 13.
TEST(utility_effect_ids_match_table_a) {
    CHECK_EQ(ev(UtilityEffect::DetectRock), 11);
    CHECK_EQ(ev(UtilityEffect::DepthPerception), 12);
    CHECK_EQ(ev(UtilityEffect::FindDirection), 13);
}

// R63: zero stored point means never cast. Only x and y are checked.
TEST(default_sanctuary_point_is_never) {
    SanctuaryPoint none;
    CHECK(none.isNever());

    SanctuaryPoint levelOnly;
    levelOnly.dungeonLevel = 3;
    CHECK(levelOnly.isNever());

    SanctuaryPoint xSet;
    xSet.x = 1;
    CHECK(!xSet.isNever());
}

// R63: SanctuaryX = X + 1; SanctuaryY = Y + 1; SanctuaryLevel = DungeonLevel.
TEST(set_sanctuary_stores_one_based_square) {
    tsup::Env env({0.5, 0.9});
    Party party;
    party.x = 3;
    party.y = 4;
    party.dungeonLevel = 2;

    SanctuaryPoint sp = castSetSanctuary(env.ctx, party);
    CHECK_EQ(sp.x, 4);
    CHECK_EQ(sp.y, 5);
    CHECK_EQ(sp.dungeonLevel, 2);
    CHECK(!sp.isNever());
}

// R63 quirk: the +1 makes square (0,0) store as (1,1), so it is never "never".
TEST(set_sanctuary_at_origin_is_not_never) {
    tsup::Env env({0.5, 0.9});
    Party party;
    party.x = 0;
    party.y = 0;
    party.dungeonLevel = 0;

    SanctuaryPoint sp = castSetSanctuary(env.ctx, party);
    CHECK_EQ(sp.x, 1);
    CHECK_EQ(sp.y, 1);
    CHECK_EQ(sp.dungeonLevel, 0);
    CHECK(!sp.isNever());
}

// R63: a never-cast point returns NeverCast and does not age the caster.
TEST(sanctuary_return_with_never_cast_does_nothing) {
    tsup::Env env({0.5, 0.9});
    Character caster;
    caster.ageDays = 1000;

    SanctuaryOutcome out = castSanctuaryReturn(env.ctx, caster, SanctuaryPoint{});
    CHECK(out == SanctuaryOutcome::NeverCast);
    CHECK_EQ(caster.ageDays, 1000);
}

// R63: a return performed ages the caster by kSanctuaryDays (365).
TEST(sanctuary_return_ages_caster_365_days) {
    tsup::Env env({0.5, 0.9});
    Character caster;
    caster.ageDays = 1000;

    SanctuaryPoint stored;
    stored.x = 4;
    stored.y = 5;
    stored.dungeonLevel = 2;

    SanctuaryOutcome out = castSanctuaryReturn(env.ctx, caster, stored);
    CHECK(out == SanctuaryOutcome::Returned);
    CHECK_EQ(caster.ageDays, 1365);
}

// R62 / rerollFateVitals: CurHits = Int(rnd * MaxHits) + 1, then CurSpellPoints likewise.
// Draws 0.5 and 0.9. Hits: floor(0.5*15)+1 = 8. SP: floor(0.9*7)+1 = floor(6.3)+1 = 7.
TEST(reroll_fate_vitals_uses_scaled_draws) {
    tsup::Env env({0.5, 0.9});
    Character who;
    who.vitals.maxHits = 15;
    who.vitals.maxSpellPoints = 7;

    rerollFateVitals(env.ctx, who);
    CHECK_EQ(who.vitals.curHits, 8);
    CHECK_EQ(who.vitals.curSpellPoints, 7);
}

// R62 boundaries: draw 0.0 gives the minimum 1; draw 0.999 gives the maximum.
// 0.999*15 = 14.985, floor 14, +1 = 15.
TEST(reroll_fate_vitals_boundaries) {
    tsup::Env env({0.0, 0.999});
    Character who;
    who.vitals.maxHits = 15;
    who.vitals.maxSpellPoints = 15;

    rerollFateVitals(env.ctx, who);
    CHECK_EQ(who.vitals.curHits, 1);
    CHECK_EQ(who.vitals.curSpellPoints, 15);
}

// R62 steps 1-2 and quirk: one unused draw first (0.1), then hits (0.5 -> 8),
// then spell points (0.9 -> 7). Fate names the first member, not others.
// Without the unused draw, hits would come from 0.1 and be 2, so this checks the quirk.
TEST(apply_fate_draws_unused_first_and_names_first_member) {
    tsup::Env env({0.1, 0.5, 0.9});
    Party party;
    party.size = 2;
    party.members[0].vitals.maxHits = 15;
    party.members[0].vitals.maxSpellPoints = 7;
    party.members[1].vitals.maxHits = 15;
    party.members[1].vitals.maxSpellPoints = 7;

    applyFate(env.ctx, party);
    CHECK_EQ(party.members[0].vitals.curHits, 8);
    CHECK_EQ(party.members[0].vitals.curSpellPoints, 7);
    CHECK_EQ(party.members[1].vitals.curHits, 0);
    CHECK_EQ(party.members[1].vitals.curSpellPoints, 0);
}

// R58: Fetch lists only dead characters with cause "rock", on the leader's level, in area 0.
// Roster indices: 0 qualifies; 1 alive; 2 dead but not rock; 3 wrong area; 4 wrong level.
TEST(fetch_candidates_filters_rock_leader_level_area0) {
    Party party;
    party.dungeonLevel = 2;
    party.members[0].location.dungeonLevel = 2;

    Vec<Character> roster;

    Character r0;
    r0.deathState = DeathState::Rock;
    r0.location.dungeonLevel = 2;
    r0.location.area = 0;
    roster.push_back(r0);

    Character r1;
    r1.deathState = DeathState::Alive;
    r1.location.dungeonLevel = 2;
    r1.location.area = 0;
    roster.push_back(r1);

    Character r2;
    r2.deathState = DeathState::Dead;
    r2.location.dungeonLevel = 2;
    r2.location.area = 0;
    roster.push_back(r2);

    Character r3;
    r3.deathState = DeathState::Rock;
    r3.location.dungeonLevel = 2;
    r3.location.area = 1;
    roster.push_back(r3);

    Character r4;
    r4.deathState = DeathState::Rock;
    r4.location.dungeonLevel = 3;
    r4.location.area = 0;
    roster.push_back(r4);

    Vec<i32> res = fetchCandidates(roster, party);
    CHECK_EQ(static_cast<i32>(res.size()), 1);
    CHECK_EQ(res[0], 0);
}

// R57: outside LocateLevelRange (here a 29-level gap) gives TooFar; rosterIndex is kept.
TEST(classify_locate_too_far_keeps_roster_index) {
    Location from;
    from.x = 0;
    from.y = 0;
    from.dungeonLevel = 1;
    from.area = 1;

    Location to;
    to.x = 0;
    to.y = 0;
    to.dungeonLevel = 30;
    to.area = 1;

    LocateLine line = classifyLocate(from, to, 7);
    CHECK(line.kind == LocateKind::TooFar);
    CHECK_EQ(line.rosterIndex, 7);
}

// R57: a target in the city (area kCityArea) gives InCity.
TEST(classify_locate_in_city) {
    Location from;
    from.x = 0;
    from.y = 0;
    from.dungeonLevel = 2;
    from.area = 1;

    Location to;
    to.x = 0;
    to.y = 0;
    to.dungeonLevel = 0;
    to.area = kCityArea;

    LocateLine line = classifyLocate(from, to, 2);
    CHECK(line.kind == LocateKind::InCity);
    CHECK_EQ(line.rosterIndex, 2);
}

// R57: target in range and not in city gives Direction, with the relative direction
// matching relativeDirection() for the same squares.
TEST(classify_locate_in_range_gives_direction) {
    Location from;
    from.x = 0;
    from.y = 0;
    from.dungeonLevel = 2;
    from.area = 1;

    Location to;
    to.x = 3;
    to.y = 0;
    to.dungeonLevel = 10;
    to.area = 1;

    LocateLine line = classifyLocate(from, to, 1);
    CHECK(line.kind == LocateKind::Direction);
    CHECK(line.direction == relativeDirection(0, 0, 3, 0));
    CHECK(line.direction == RelativeDirection::East || line.direction == RelativeDirection::West);
}

// Relative directions are 8-point: cardinals are even, diagonals odd, and reversing
// a vector gives the opposite point (index differs by 4 mod 8).
TEST(relative_direction_opposites_and_axes) {
    RelativeDirection e = relativeDirection(0, 0, 3, 0);
    RelativeDirection w = relativeDirection(0, 0, -3, 0);
    CHECK_EQ((ev(e) + 4) % 8, ev(w));
    CHECK(e == RelativeDirection::East || e == RelativeDirection::West);

    RelativeDirection n = relativeDirection(0, 0, 0, 3);
    RelativeDirection s = relativeDirection(0, 0, 0, -3);
    CHECK_EQ((ev(n) + 4) % 8, ev(s));
    CHECK(n == RelativeDirection::North || n == RelativeDirection::South);

    RelativeDirection d1 = relativeDirection(0, 0, 3, 3);
    RelativeDirection d2 = relativeDirection(0, 0, -3, -3);
    CHECK_EQ(ev(d1) % 2, 1);
    CHECK_EQ((ev(d1) + 4) % 8, ev(d2));
}

// R61: candidates are living (Hits > 0) with BindingLevel != 0.
// Default bindingLevel is -10 and default hits is 0.
TEST(is_bind_candidate_requires_living_and_nonzero_level) {
    Companion living;
    living.hits = 5;
    living.bindingLevel = -10;
    CHECK(isBindCandidate(living));

    Companion dead;
    dead.hits = 0;
    dead.bindingLevel = -10;
    CHECK(!isBindCandidate(dead));

    Companion zeroLevel;
    zeroLevel.hits = 5;
    zeroLevel.bindingLevel = 0;
    CHECK(!isBindCandidate(zeroLevel));
}

// R61: bindCandidates returns the indices of qualifying companions in order.
TEST(bind_candidates_lists_qualifying_indices) {
    Vec<Companion> comps;

    Companion c0;
    c0.hits = 5;
    c0.bindingLevel = -10;
    comps.push_back(c0);

    Companion c1;
    c1.hits = 0;
    c1.bindingLevel = -10;
    comps.push_back(c1);

    Companion c2;
    c2.hits = 3;
    c2.bindingLevel = 0;
    comps.push_back(c2);

    Companion c3;
    c3.hits = 1;
    c3.bindingLevel = 20;
    comps.push_back(c3);

    Vec<i32> res = bindCandidates(comps);
    CHECK_EQ(static_cast<i32>(res.size()), 2);
    CHECK_EQ(res[0], 0);
    CHECK_EQ(res[1], 3);
}

// R61: with no candidates, Interact.NoBoundCompanions and nothing is spent.
TEST(cast_binding_without_candidates_spends_nothing) {
    tsup::Env env({0.5, 0.9});
    Character caster;
    caster.vitals.curSpellPoints = 20;
    caster.vitals.maxSpellPoints = 20;

    Vec<Companion> comps;
    Companion dead;
    dead.hits = 0;
    dead.bindingLevel = -10;
    comps.push_back(dead);

    BindTargetOutcome out = castBinding(env.ctx, caster, comps, 0, 40, 5);
    CHECK(out == BindTargetOutcome::NoBoundCompanions);
    CHECK_EQ(caster.vitals.curSpellPoints, 20);
}

// R61: with a candidate, the caster picks one, points are spent, and the bind is applied.
TEST(cast_binding_with_candidate_spends_points) {
    tsup::Env env({0.5, 0.9});
    Character caster;
    caster.vitals.curSpellPoints = 20;
    caster.vitals.maxSpellPoints = 20;

    Vec<Companion> comps;
    Companion live;
    live.hits = 5;
    live.bindingLevel = -10;
    comps.push_back(live);

    BindTargetOutcome out = castBinding(env.ctx, caster, comps, 0, 40, 5);
    CHECK(out == BindTargetOutcome::Bound);
    CHECK_EQ(caster.vitals.curSpellPoints, 15);
}

// R64: Depth Perception and Find Direction end the party's lost state.
TEST(clear_party_lost_ends_party_lost_state) {
    tsup::Env env({0.5, 0.9});
    Party party;
    party.lost = true;
    party.lostPosition = true;
    party.lostLevel = true;
    party.lostFacing = true;

    clearPartyLost(env.ctx, party);
    CHECK(!party.lost);
    CHECK(!party.lostPosition);
    CHECK(!party.lostLevel);
    CHECK(!party.lostFacing);
}
