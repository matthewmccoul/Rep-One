// Console front end for the Mordor rules core. It talks to the core only through include/mordor/game.h.
// Usage: mordor_console [--seed N] [--quick] [--script FILE] [--bot STEPS] [--data FILE]
//   --quick   create a ready-made party of four instead of the creation dialogue
//   --script  read commands from FILE instead of the keyboard (one per line; '#' starts a comment)
//   --bot     let a simple autopilot play STEPS commands (used as an integration test)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "mordor/game.h"
#include "mordor/features.h"

using namespace mordor;

namespace {

std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// Raise stats to the guild's minimums, then spend the remaining creation points.
Arr<i32, 6> autoStats(const Session& s, Race race, Guild guild) {
  Arr<i32, 6> st = {};
  const RaceDef& r = s.data.races[static_cast<size_t>(race)];
  const GuildDef& g = s.data.guilds[static_cast<size_t>(guild)];
  for (int i = 0; i < 6; ++i) st[i] = r.min[i];   // start from the race minimums; the pool pays for every point above
  for (int i = 0; i < 6; ++i) {
    while (st[i] < g.reqStats[i] && st[i] < r.max[i] && creationPointsLeft(s, race, st) > 0) ++st[i];
  }
  int pref[6] = {CON, DEX, STR, INT, WIS, CHA};
  if (guild >= Guild::Mage) { pref[0] = INT; pref[1] = WIS; pref[2] = CON; pref[3] = DEX; pref[4] = STR; pref[5] = CHA; }
  for (int guard = 0; guard < 200 && creationPointsLeft(s, race, st) > 0; ++guard) {
    int i = pref[guard % 6];
    if (st[i] < r.max[i]) ++st[i];
  }
  return st;
}

// Tries every race/alignment until the character can be created in the wanted guild.
bool addQuick(Session& s, const char* name, Race preferred, Align al, Guild guild) {
  for (int k = 0; k < 9; ++k) {
    Race race = static_cast<Race>((static_cast<int>(preferred) + k) % 9);
    for (int a = 0; a < 3; ++a) {
      NewCharacter nc;
      nc.name = name; nc.race = race; nc.alignment = static_cast<Align>((static_cast<int>(al) + a) % 3); nc.guild = guild; nc.stats = autoStats(s, race, guild);
      Str err;
      if (createCharacter(s, nc, err)) return true;
    }
  }
  std::printf("  could not create %s in guild %d\n", name, static_cast<int>(guild));
  return false;
}

void quickParty(Session& s) {
  addQuick(s, "Aldric", Race::Human, Align::Good, Guild::Warrior);
  addQuick(s, "Lyra", Race::Elf, Align::Good, Guild::Mage);
  addQuick(s, "Borin", Race::Osiri, Align::Neutral, Guild::Thief);
  addQuick(s, "Thessa", Race::Gnome, Align::Neutral, Guild::Healer);
}

std::string readLine(std::istream& in, bool& ok) {
  std::string l;
  ok = static_cast<bool>(std::getline(in, l));
  return l;
}

const char* kRaces[] = {"Human", "Elf", "Giant", "Gnome", "Dwarf", "Ogre", "Morloch", "Osiri", "Troll"};
const char* kGuilds[] = {"Nomad", "Warrior", "Paladin", "Ninja", "Villain", "Seeker", "Thief", "Scavenger", "Mage", "Sorcerer", "Wizard", "Healer"};

void creationDialogue(Session& s, std::istream& in) {
  std::printf("Create your party (1 to 4 characters). Enter a blank name to finish.\n");
  while (s.game.party.size < 4) {
    std::printf("Name (blank = done): ");
    bool ok; std::string name = readLine(in, ok);
    if (!ok || name.empty()) break;
    std::printf("Race: "); for (int i = 0; i < 9; ++i) std::printf("%d=%s ", i, kRaces[i]); std::printf("\n> ");
    int race = std::atoi(readLine(in, ok).c_str());
    std::printf("Alignment (0=Good 1=Neutral 2=Evil): "); int al = std::atoi(readLine(in, ok).c_str());
    std::printf("Guild: "); for (int i = 0; i < 12; ++i) std::printf("%d=%s ", i, kGuilds[i]); std::printf("\n> ");
    int gu = std::atoi(readLine(in, ok).c_str());
    NewCharacter nc;
    nc.name = name; nc.race = static_cast<Race>(race < 0 || race > 8 ? 0 : race); nc.alignment = static_cast<Align>(al < 0 || al > 2 ? 1 : al); nc.guild = static_cast<Guild>(gu < 0 || gu > 11 ? 0 : gu);
    nc.stats = autoStats(s, nc.race, nc.guild);
    Str err;
    if (createCharacter(s, nc, err)) std::printf("  %s joins the party.\n", name.c_str());
    else std::printf("  cannot create: %s\n", err.c_str());
  }
}

bool g_showEvents = false;
void flushEvents(Session& s) {
  if (g_showEvents) for (const Event& e : s.log.events) std::printf("  [event %s] %s\n", e.code.c_str(), e.text.c_str());
  s.log.clear();
}

// A simple autopilot: walks, turns when it hits a wall, fights, rests in town. Deterministic (own xorshift).
unsigned g_botRng = 12345u;
unsigned botNext() { g_botRng ^= g_botRng << 13; g_botRng ^= g_botRng >> 17; g_botRng ^= g_botRng << 5; return g_botRng; }

std::string botCommand(Session& s, int step, const std::string& lastReply) {
  switch (currentScreen(s)) {
    case Screen::Town: return (step % 3 == 0) ? "rest" : "descend";
    case Screen::Combat: return (botNext() % 6 == 0) ? "defend" : "fight";
    case Screen::Dungeon: {
      if (lastReply.find("Ouch") != std::string::npos) { unsigned r = botNext() % 3; return r == 0 ? "left" : (r == 1 ? "right" : "back"); }
      if (step % 17 == 0) return "open";
      if (step % 23 == 0) return "search";
      return "forward";
    }
    default: return "";
  }
}

bool run(Session& s, const std::string& cmd, std::string& reply) {
  switch (currentScreen(s)) {
    case Screen::Town: return townAction(s, cmd, reply);
    case Screen::Dungeon: return dungeonAction(s, cmd, reply);
    case Screen::Combat: return combatAction(s, cmd, reply);
    default: reply = "The game is over."; return false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  u64 seed = 1; bool quick = false; int botSteps = 0; std::string script, dataPath = MORDOR_DATA_FILE;
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "--seed") && i + 1 < argc) seed = static_cast<u64>(std::strtoull(argv[++i], nullptr, 10));
    else if (!std::strcmp(argv[i], "--quick")) quick = true;
    else if (!std::strcmp(argv[i], "--events")) g_showEvents = true;
    else if (!std::strcmp(argv[i], "--script") && i + 1 < argc) script = argv[++i];
    else if (!std::strcmp(argv[i], "--bot") && i + 1 < argc) botSteps = std::atoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--data") && i + 1 < argc) dataPath = argv[++i];
  }
  Session s; Str err;
  if (!sessionInit(s, slurp(dataPath), seed, err)) { std::printf("cannot load data: %s\n", err.c_str()); return 1; }
  std::printf("MORDOR: The Depths of Dejenol (rules core demo)  seed %llu\n", static_cast<unsigned long long>(seed));
  std::ifstream scriptIn;
  std::istream* in = &std::cin;
  if (!script.empty()) { scriptIn.open(script); in = &scriptIn; }
  if (quick || botSteps > 0) quickParty(s); else creationDialogue(s, *in);
  if (s.game.party.size == 0) { std::printf("No party. Bye.\n"); return 0; }
  std::printf("%s\n", describeParty(s).c_str());
  int step = 0; std::string lastReply; g_botRng = static_cast<unsigned>(seed * 2654435761u + 12345u) | 1u;
  for (;;) {
    if (currentScreen(s) == Screen::GameOver) { std::printf("\n*** The party has fallen. ***\n"); break; }
    std::string cmd;
    if (botSteps > 0) { if (step >= botSteps) break; cmd = botCommand(s, step++, lastReply); std::printf("\n[bot] > %s\n", cmd.c_str()); }
    else {
      std::printf("\n%s\n> ", describeSquare(s).c_str());
      bool ok; cmd = readLine(*in, ok);
      if (!ok) break;
      while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == ' ')) cmd.pop_back();
      if (cmd.empty() || cmd[0] == '#') continue;
    }
    std::string reply;
    if (cmd == "quit" || cmd == "q") break;
    if (cmd == "status" || cmd == "party") { std::printf("%s\n", describeParty(s).c_str()); continue; }
    if (cmd == "map") { std::printf("%s\n", renderAutomap(s, 7).c_str()); continue; }
    if (cmd.rfind("char ", 0) == 0) { std::printf("%s\n", describeCharacter(s, std::atoi(cmd.c_str() + 5) - 1).c_str()); continue; }
    if (cmd.rfind("cheat xp ", 0) == 0) {   // test aid: cheat xp <member 1-4> <amount>
      int m = 0; long long n = 0; std::sscanf(cmd.c_str() + 9, "%d %lld", &m, &n); --m;
      if (m >= 0 && m < s.game.party.size) { Character& c = s.game.party.members[static_cast<size_t>(m)]; c.guilds[static_cast<size_t>(c.activeGuild)].experience += n; std::printf("%s gains %lld xp (cheat)\n", c.name.c_str(), n); }
      continue;
    }
    if (cmd == "debug") { const GameState& g = s.game; std::printf("inCombat=%d underground=%d clockRunning=%d inRound=%d level=%d size=%d groups=%zu\n", g.inCombat, g.underground, g.combatClockRunning, g.inCombatRound, g.party.dungeonLevel, g.party.size, g.encounter.groups.size()); continue; }
    if (cmd == "fight?" || cmd == "foes") { std::printf("%s\n", describeFight(s).c_str()); continue; }
    if (cmd == "save") { std::ofstream f("mordor_save.txt"); f << saveSession(s); std::printf("saved to mordor_save.txt\n"); continue; }
    if (cmd == "load") { Str e; if (loadSession(s, slurp("mordor_save.txt"), e)) std::printf("loaded\n"); else std::printf("load failed: %s\n", e.c_str()); continue; }
    if (cmd == "help") { std::printf("town (members are numbered 1-4): rest guild levelup <m> shop buy <n> <m> sell <slot> <m> bank deposit|withdraw <g> morgue raise <name> pool descend\n"
                                      "dungeon: forward back left right stairs open search pool   combat: fight [group] defend cast <m> <spell> use <m> <slot>\n"
                                      "any: status char <m> map foes save load quit\n"); std::printf("%s", featureHelp(currentScreen(s)).c_str()); continue; }
    bool ok = run(s, cmd, reply); lastReply = reply;
    if (!reply.empty()) std::printf("%s\n", reply.c_str());
    if (!ok && reply.empty()) std::printf("(not possible here: %s; type help)\n", cmd.c_str());
    flushEvents(s);
  }
  std::printf("\n%s\n", describeParty(s).c_str());
  return 0;
}
