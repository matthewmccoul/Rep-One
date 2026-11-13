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

