#include <string>

#include "mordor/access.h"
#include "mordor/features.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// One Look page row (LookAround.Walk).
struct LookRow {
  Str name;
  Str guild;
  i32 level = 0;
  i32 hits = 0;
  bool countedDead = false;
};

// The party walk: LookAround.Walk restricted to the party, which is all the state exposes.
// Member 1 is the leader and is always counted living. A dead non-leader is counted apart.
struct RoomWalk {
  Vec<LookRow> here;
  i32 living = 0;
  i32 dead = 0;
};


// Pads or cuts a field to a fixed column width (Look page columns 1, 9, 15, 19).
Str padTo(const Str& text, size_t width) {
  Str o = text.substr(0, width);
  while (o.size() < width) o += ' ';
  return o;
}

bool isDead(const Character& c) { return c.deathState != DeathState::Alive; }

// UNKNOWN: GuildDef display field assumed to be `name`.
Str guildNameOf(const Session& s, const Character& c) {
  const i32 g = static_cast<i32>(c.activeGuild);
  if (g < 0 || g >= static_cast<i32>(s.data.guilds.size())) return Str();
  return s.data.guilds[static_cast<size_t>(g)].name;
}

// UNKNOWN: facing values assumed 0=N, 1=E, 2=S, 3=W; anything else shows '?'.
Str facingLetter(i32 f) {
  static const char kLetters[] = "NESW";
  if (f < 0 || f > 3) return Str("?");
  return Str(1, kLetters[f]);
}

RoomWalk walkRoom(Session& s) {
  RoomWalk w;
  const i32 n = accPartySize(s);
  for (i32 m = 1; m <= n; ++m) {
    Character* c = accMember(s, m);
    if (c == nullptr) continue;
    const bool isLeader = (m == 1);
    const bool dead = isDead(*c);
    LookRow row;
    row.name = c->name;
    row.guild = guildNameOf(s, *c);
    row.level = c->level;
    row.hits = c->vitals.curHits;
    row.countedDead = dead && !isLeader;
    if (row.countedDead) {
      ++w.dead;
    } else {
      ++w.living;
    }
    w.here.push_back(row);
  }
  return w;
}

// Readout "x,y,level facing [ n ]", or "[ n+d ]" when anyone dead is here (LookAround.Readout).
Str readoutLine(const Session& s, i32 living, i32 dead) {
  Str line = toStr(s.game.party.x) + "," + toStr(s.game.party.y) + "," +
             toStr(s.game.party.dungeonLevel) + " " +
             facingLetter(static_cast<i32>(s.game.party.facing)) + " [ " + toStr(living);
  if (dead > 0) line += "+" + toStr(dead);
  line += " ]";
  return line;
}

}  // namespace

// Look page text, read-only. Returns the readout line, the square line, the room lines and
// the Look page rows. Exported so the smoke test can call it without the command registry.
Str lookText(Session& s) {
  const RoomWalk w = walkRoom(s);
  Str out = readoutLine(s, w.living, w.dead) + "\n";

  // Square line. Mirrors SquareInfo.Of: describeSquare applies the stairs/chute precedence.
  // UNKNOWN: does describeSquare return the water/quicksand clock lines? Those belong to the
  // ailment clock, not to look, so they should not appear here.
  const Str square = describeSquare(s);
  if (!square.empty()) out += square + "\n";

  // Who is in the room (RoomRoster.Lines), bodies first, then strangers.
  // NEED: carryingCount. Until it exists no member is carrying a body, so carrying = 0.
  const i32 carrying = 0;
  const i32 bodies = w.dead - carrying;
  if (bodies == 1) {
    out += "There is a dead body here!\n";
  } else if (bodies > 1) {
    out += "There are dead bodies here!\n";
  }
  // NEED: rosterEntries. Characters left in the dungeon outside the party are not visible,
  // so strangers is the party-only remainder, which is never positive.
  const i32 strangers = w.living - accPartySize(s);
  if (strangers == 1) {
    out += "Somebody else is in the room!\n";
  } else if (strangers > 1) {
    out += "Other characters are in the room!\n";
  }

  // Look page rows: Name at column 1, Guild at 9, Level at 15, Hits at 19.
  // UNKNOWN: bold headings cannot be expressed in plain text; the heading line is plain.
  if (!w.here.empty()) {
    out += padTo("Name", 8) + padTo("Guild", 6) + padTo("Level", 4) + "Hits\n";
    for (const LookRow& r : w.here) {
      out += padTo(r.name, 8) + padTo(r.guild, 6) + padTo(toStr(r.level), 4) + toStr(r.hits) + "\n";
    }
  }
  return out;
}

namespace {

// UNKNOWN: assumes Vec<Str> words include the verb at index 0.
bool cmdLook(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() > 1) {
    reply = "Usage: look";
    return true;
  }
  reply = lookText(s);
  return true;
}

}  // namespace

void register_look() {
  registerFeatureCommand({"look", Screen::Dungeon, &cmdLook, "look  - describe the square you stand on"});
}

}  // namespace mordor
