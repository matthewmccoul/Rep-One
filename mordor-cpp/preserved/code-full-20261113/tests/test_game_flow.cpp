// End-to-end tests of the game layer (game.h): create a party, descend, walk, fight, level up, save and load.
#include <fstream>
#include <sstream>
#include "harness.h"
#include "mordor/game.h"

using namespace mordor;

namespace {

Str readData() {
  std::ifstream f(MORDOR_DATA_FILE);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

Arr<i32, 6> allocate(const Session& s, Race race, Guild guild) {
  Arr<i32, 6> st = {};
  const RaceDef& r = s.data.races[static_cast<size_t>(race)];
  const GuildDef& g = s.data.guilds[static_cast<size_t>(guild)];
  for (int i = 0; i < 6; ++i) st[static_cast<size_t>(i)] = r.min[static_cast<size_t>(i)];
  for (int i = 0; i < 6; ++i)
    while (st[static_cast<size_t>(i)] < g.reqStats[static_cast<size_t>(i)] && st[static_cast<size_t>(i)] < r.max[static_cast<size_t>(i)] && creationPointsLeft(s, race, st) > 0) ++st[static_cast<size_t>(i)];
  for (int guard = 0; guard < 200 && creationPointsLeft(s, race, st) > 0; ++guard) {
    size_t i = static_cast<size_t>(guard % 6);
    if (st[i] < r.max[i]) ++st[i];
  }
  return st;
}

bool addMember(Session& s, const char* name, Race r0, Align a0, Guild guild) {
  for (int k = 0; k < 9; ++k) {
    Race race = static_cast<Race>((static_cast<int>(r0) + k) % 9);
    for (int a = 0; a < 3; ++a) {
      NewCharacter nc;
      nc.name = name; nc.race = race; nc.alignment = static_cast<Align>((static_cast<int>(a0) + a) % 3); nc.guild = guild; nc.stats = allocate(s, race, guild);
      Str err;
      if (createCharacter(s, nc, err)) return true;
    }
  }
  return false;
}

bool makeSession(Session& s, u64 seed) {
  Str err;
  if (!sessionInit(s, readData(), seed, err)) return false;
  addMember(s, "Aldric", Race::Human, Align::Good, Guild::Warrior);
  addMember(s, "Lyra", Race::Elf, Align::Good, Guild::Mage);
  addMember(s, "Borin", Race::Osiri, Align::Neutral, Guild::Thief);
  addMember(s, "Thessa", Race::Osiri, Align::Neutral, Guild::Healer);
  return true;
}

// Deterministic walker: forward; on a wall turn left, right or around.
void walk(Session& s, int steps, unsigned seed, int& fights) {
  unsigned r = seed | 1u;
  Str last;
  for (int i = 0; i < steps && currentScreen(s) != Screen::GameOver; ++i) {
    r ^= r << 13; r ^= r >> 17; r ^= r << 5;
    Str cmd, reply;
    if (currentScreen(s) == Screen::Combat) { cmd = "fight"; ++fights; combatAction(s, cmd, reply); }
    else if (currentScreen(s) == Screen::Dungeon) {
      cmd = last.find("Ouch") != Str::npos ? (r % 3 == 0 ? "left" : (r % 3 == 1 ? "right" : "back")) : "forward";
      dungeonAction(s, cmd, reply);
    } else break;
    last = reply;
    s.log.clear();
  }
}

}  // namespace

TEST(flow_new_party_has_four_members_in_town) {
  Session s;
  CHECK(makeSession(s, 1));
  CHECK_EQ(s.game.party.size, 4);
  CHECK(currentScreen(s) == Screen::Town);
  for (int i = 0; i < 4; ++i) {
    const Character& c = s.game.party.members[static_cast<size_t>(i)];
    CHECK_EQ(c.vitals.curHits, 15);
    CHECK_EQ(c.vitals.maxHits, 15);
    CHECK_EQ(c.wallet.goldOnHand > 0, true);
  }
}

TEST(flow_creation_rejects_stats_that_miss_the_guild_minimums) {
  Session s; Str err;
  CHECK(sessionInit(s, readData(), 1, err));
  NewCharacter nc; nc.name = "Weak"; nc.race = Race::Human; nc.guild = Guild::Warrior; nc.stats = defaultStats(s, Race::Human);
  CHECK(!createCharacter(s, nc, err));
  CHECK_EQ(s.game.party.size, 0);
}

TEST(flow_descend_puts_the_party_on_the_stairs_in_the_dungeon) {
  Session s;
  CHECK(makeSession(s, 2));
  Str reply;
  CHECK(townAction(s, "descend", reply));
  CHECK(currentScreen(s) == Screen::Dungeon);
  CHECK_EQ(s.game.party.dungeonLevel, 1);
  CHECK(s.game.party.x >= 1 && s.game.party.x <= 30);
  CHECK(s.game.party.y >= 1 && s.game.party.y <= 30);
}

TEST(flow_walking_keeps_the_party_on_the_map_and_alive_or_over) {
  for (unsigned seed = 1; seed <= 4; ++seed) {
    Session s;
    CHECK(makeSession(s, seed));
    Str reply;
    townAction(s, "descend", reply);
    int fights = 0;
    walk(s, 300, seed * 7919u, fights);
    const Party& p = s.game.party;
    CHECK(p.x >= 1 && p.x <= 30 && p.y >= 1 && p.y <= 30);
    for (int i = 0; i < p.size; ++i) {
      const Vitals& v = p.members[static_cast<size_t>(i)].vitals;
      CHECK(v.curHits >= 0 && v.curHits <= v.maxHits);
    }
  }
}

TEST(flow_fights_resolve_and_award_experience) {
  Session s;
  CHECK(makeSession(s, 3));
  Str reply;
  townAction(s, "descend", reply);
  int fights = 0;
  walk(s, 600, 99u, fights);
  CHECK(fights > 0);
  long long xp = 0;
  for (int i = 0; i < s.game.party.size; ++i) {
    const Character& c = s.game.party.members[static_cast<size_t>(i)];
    xp += c.guilds[static_cast<size_t>(c.activeGuild)].experience;
  }
  CHECK(xp > 0);
}

TEST(flow_level_up_in_town_raises_level_and_hits) {
  Session s;
  CHECK(makeSession(s, 4));
  Character& c = s.game.party.members[0];
  c.guilds[static_cast<size_t>(c.activeGuild)].experience += 100000;
  const i32 hitsBefore = c.vitals.maxHits;
  Str reply;
  CHECK(townAction(s, "levelup 1", reply));
  CHECK_EQ(s.game.party.members[0].guilds[static_cast<size_t>(s.game.party.members[0].activeGuild)].level, 2);
  CHECK(s.game.party.members[0].vitals.maxHits > hitsBefore);
}

TEST(flow_same_seed_same_commands_same_result) {
  Str a, b;
  for (int run = 0; run < 2; ++run) {
    Session s;
    CHECK(makeSession(s, 5));
    Str reply;
    townAction(s, "descend", reply);
    int fights = 0;
    walk(s, 250, 4242u, fights);
    (run == 0 ? a : b) = describeParty(s) + describeSquare(s);
  }
  CHECK(a == b);
}

TEST(flow_save_and_load_round_trip_keeps_the_party) {
  Session s;
  CHECK(makeSession(s, 6));
  Str reply;
  townAction(s, "descend", reply);
  int fights = 0;
  walk(s, 120, 31u, fights);
  Str saved = saveSession(s);
  CHECK(saved.size() > 100);
  Session t; Str err;
  CHECK(sessionInit(t, readData(), 6, err));
  CHECK(loadSession(t, saved, err));
  CHECK(describeParty(s) == describeParty(t));
  CHECK_EQ(s.game.party.x, t.game.party.x);
  CHECK_EQ(s.game.party.y, t.game.party.y);
  CHECK_EQ(s.game.party.dungeonLevel, t.game.party.dungeonLevel);
}
