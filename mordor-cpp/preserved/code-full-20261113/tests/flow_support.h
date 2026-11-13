// Shared helpers for game-layer flow tests: load the data, build a four-member party, make a session.
#pragma once
#include <fstream>
#include <sstream>
#include "harness.h"
#include "mordor/game.h"

namespace flowsup {
using namespace mordor;

inline Str readData() {
  std::ifstream f(MORDOR_DATA_FILE);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

inline Arr<i32, 6> allocate(const Session& s, Race race, Guild guild) {
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

inline bool addMember(Session& s, const char* name, Race r0, Align a0, Guild guild) {
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

inline bool makeSession(Session& s, u64 seed) {
  Str err;
  if (!sessionInit(s, readData(), seed, err)) return false;
  addMember(s, "Aldric", Race::Human, Align::Good, Guild::Warrior);
  addMember(s, "Lyra", Race::Elf, Align::Good, Guild::Mage);
  addMember(s, "Borin", Race::Osiri, Align::Neutral, Guild::Thief);
  addMember(s, "Thessa", Race::Osiri, Align::Neutral, Guild::Healer);
  return true;
}

}  // namespace flowsup
