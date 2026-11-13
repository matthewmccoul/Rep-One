#include "test_support.h"
#include "mordor/dungeon1b.h"

using namespace mordor;

namespace {

Character makeChar(const char* name, i32 dungeonLevel, i32 area, bool dead) {
    Character c;
    c.name = name;
    c.location.dungeonLevel = dungeonLevel;
    c.location.area = area;
    c.deathState = dead ? DeathState::Dead : DeathState::Alive;
    return c;
}

void placeLeader(Party& p, const Character& lead) {
    p.members[0] = lead;
    p.size = 1;
    p.dungeonLevel = lead.location.dungeonLevel;
}

// Leader at 26,6,6 facing West, with the given lost flags mirrored on the
// character and on the party.
void setReadout(Party& p, Character& lead, bool posLost, bool lvlLost, bool facLost) {
    lead.location.x = 26;
    lead.location.y = 6;
    lead.location.dungeonLevel = 6;
    lead.location.area = 7;
    lead.location.facing = 3;  // West
    lead.lost.position = posLost;
    lead.lost.level = lvlLost;
    lead.lost.facing = facLost;

    p.members[0] = lead;
    p.size = 1;
    p.x = 26;
    p.y = 6;
    p.dungeonLevel = 6;
    p.facing = Facing::W;
    p.lostPosition = posLost;
    p.lostLevel = lvlLost;
    p.lostFacing = facLost;
    p.lost = posLost;  // party-level lost text keys on position loss
}

// Roster for the Walk cases: Ghost, Far, Lead, Below, Hid.
// Lead is the leader on floor 3 area 7. Hid is hidden (visible = false);
// hiding does not affect roster inclusion.
Vec<Character> walkRoster(const Character& lead) {
    Vec<Character> roster;
    roster.push_back(makeChar("Ghost", 3, 7, true));
    roster.push_back(makeChar("Far", 3, 8, false));
    roster.push_back(lead);
    roster.push_back(makeChar("Below", 4, 7, false));
    Character hid = makeChar("Hid", 3, 7, false);
    hid.visible = false;
    roster.push_back(hid);
    return roster;
}

}  // namespace

// SKIP B_0063#128: needs a fixed-destination teleporter on a loaded floor
// with a populated FloorTeleporter and GameData; walkOntoTeleporter setup is
// not constructible from the headers alone.

TEST(engine_B_0066_1_walk_lists_roster_in_area) {
    Character lead = makeChar("Lead", 3, 7, false);
    Party party;
    placeLeader(party, lead);
    Vec<Character> roster = walkRoster(lead);

    // Roster order: Ghost, Far, Lead, Below, Hid
    CHECK(includeInRoster(party, roster[0], false));   // Ghost (dead, same area)
    CHECK(!includeInRoster(party, roster[1], false));  // Far (area 8)
    CHECK(includeInRoster(party, roster[2], false));   // Lead (leader)
    CHECK(!includeInRoster(party, roster[3], false));  // Below (floor 4)
    CHECK(includeInRoster(party, roster[4], false));   // Hid (hidden still listed)
}

TEST(engine_B_0066_2_count_living_and_dead_in_area) {
    Character lead = makeChar("Lead", 3, 7, false);
    Party party;
    placeLeader(party, lead);
    Vec<Character> roster = walkRoster(lead);

    RosterCounts c = countRoster(party, roster, false);
    CHECK_EQ(c.living, 2);
    CHECK_EQ(c.dead, 1);
}

TEST(engine_B_0066_3_dead_leader_counted_as_living) {
    Character lead = makeChar("Lead", 3, 7, true);
    Party party;
    placeLeader(party, lead);
    Vec<Character> roster;
    roster.push_back(lead);

    RosterCounts c = countRoster(party, roster, false);
    CHECK_EQ(c.living, 1);
    CHECK_EQ(c.dead, 0);
}

TEST(engine_B_0066_4_everywhere_lists_all_roster) {
    Character lead = makeChar("Lead", 3, 7, false);
    Party party;
    placeLeader(party, lead);
    Vec<Character> roster;
    roster.push_back(makeChar("Ghost", 3, 7, true));
    roster.push_back(makeChar("Far", 3, 8, false));
    roster.push_back(lead);
    roster.push_back(makeChar("Below", 4, 7, false));

    RosterCounts c = countRoster(party, roster, true);
    CHECK_EQ(c.living + c.dead, 4);
}

TEST(engine_B_0066_5_readout_alive_not_lost) {
    Character lead = makeChar("Lead", 6, 7, false);
    Party party;
    setReadout(party, lead, false, false, false);
    Vec<Character> roster;
    roster.push_back(lead);

    Str out = roomReadout(party, roster, false, 0);
    CHECK_EQ(out, Str("26,6,6 West [ 1 ]"));
}

TEST(engine_B_0066_6_readout_living_and_dead_counts) {
    Character lead = makeChar("Lead", 6, 7, false);
    Party party;
    setReadout(party, lead, false, false, false);
    Vec<Character> roster;
    roster.push_back(lead);
    roster.push_back(makeChar("Hid", 6, 7, false));
    roster.push_back(makeChar("Ghost", 6, 7, true));

    Str out = roomReadout(party, roster, false, 0);
    CHECK_EQ(out, Str("26,6,6 West [ 2+1 ]"));
}

TEST(engine_B_0066_7_readout_lost_on_level_only) {
    Character lead = makeChar("Lead", 6, 7, false);
    Party party;
    setReadout(party, lead, false, true, false);
    Vec<Character> roster;
    roster.push_back(lead);

    Str out = roomReadout(party, roster, false, 0);
    CHECK_EQ(out, Str("26,6,6 West [ 1 ]"));
}

TEST(engine_B_0066_8_readout_lost_on_level_and_position) {
    Character lead = makeChar("Lead", 6, 7, false);
    Party party;
    setReadout(party, lead, true, true, false);
    Vec<Character> roster;
    roster.push_back(lead);

    Str out = roomReadout(party, roster, false, 0);
    CHECK_EQ(out, Str("(Lost) West [ 1 ]"));
}

TEST(engine_B_0066_9_readout_lost_on_position_and_facing) {
    Character lead = makeChar("Lead", 6, 7, false);
    Party party;
    setReadout(party, lead, true, false, true);
    Vec<Character> roster;
    roster.push_back(lead);

    Str out = roomReadout(party, roster, false, 0);
    CHECK_EQ(out, Str("(Lost on 6) ? [ 1 ]"));
}
