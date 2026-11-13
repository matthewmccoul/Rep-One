#include <algorithm>
#include <cctype>
#include <cstddef>

#include "mordor/access.h"
// FIXED: removed #include "mordor/feat_roster.h"; no such header exists in the project.
#include "mordor/features.h"
#include "mordor/game.h"
#include "mordor/state_common.h"

namespace mordor {
namespace {

// Left-out members keep their full records in the party record arrays, just past the party:
// slot (party.size + i) holds the record named on line i of featureState "roster.left".
// Mirrors Roster.Records/Removed in Roster.cs: a name stays on record after it leaves the party.
const char* kRosterLeftKey = "roster.left";

// Name match as in Roster.cs Same(): trailing spaces ignored, case-insensitive.
bool sameName(const Str& a, const Str& b) {
  size_t ea = a.size();
  while (ea > 0 && a[ea - 1] == ' ') --ea;
  size_t eb = b.size();
  while (eb > 0 && b[eb - 1] == ' ') --eb;
  if (ea != eb) return false;
  for (size_t i = 0; i < ea; ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  }
  return true;
}

Vec<Str> rosterLeftBehind(const Session& s) {
  Vec<Str> out;
  auto it = s.featureState.find(Str(kRosterLeftKey));
  if (it == s.featureState.end()) return out;
  Str cur;
  for (char c : it->second) {
    if (c == '\n') {
      if (!cur.empty()) out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}

void rosterWriteLeft(Session& s, const Vec<Str>& names) {
  Str text;
  for (const Str& n : names) {
    text += n;
    text.push_back('\n');
  }
  s.featureState[Str(kRosterLeftKey)] = text;
}

// Returns index into the left-behind list, or -1.
i32 rosterFindLeft(const Vec<Str>& left, const Str& name) {
  for (size_t i = 0; i < left.size(); ++i) {
    if (sameName(left[i], name)) return static_cast<i32>(i);
  }
  return -1;
}

// Slots the parallel record arrays hold. Party plus left-out records must fit in this.
size_t rosterCapacity(const Session& s) {
  size_t cap = s.game.party.members.size();
  cap = std::min(cap, static_cast<size_t>(s.game.party.inventories.size()));
  cap = std::min(cap, static_cast<size_t>(s.game.memberCompanions.size()));
  return cap;
}

inline std::ptrdiff_t rosterOffset(size_t v) { return static_cast<std::ptrdiff_t>(v); }

template <typename Container>
void rosterRotate(Container& c, size_t first, size_t mid, size_t last) {
  std::rotate(c.begin() + rosterOffset(first), c.begin() + rosterOffset(mid),
              c.begin() + rosterOffset(last));
}

// Rotates the same range of all three parallel arrays, so a member's inventory and
// companions travel with the member's record.
void rosterRotateRecords(Session& s, size_t first, size_t mid, size_t last) {
  rosterRotate(s.game.party.members, first, mid, last);
  rosterRotate(s.game.party.inventories, first, mid, last);
  rosterRotate(s.game.memberCompanions, first, mid, last);
}

void rosterSay(Session& s, const char* code, const Str& text, const Str& name) {
  Ctx ctx = s.ctx();
  ctx.say(code, text, {}, {name});
}

// SRD/reference: sub159D (MULTIMOD.BAS:739-751). Refuses the leader (position one) and the last member.
// The party is kept in order; the removed record goes to the end of the roster (rotation to the end).
bool rosterUnloadMember(Session& s, i32 member1, Str& reply) {
  const i32 n = accPartySize(s);
  if (member1 < 1 || member1 > n) {
    reply = "No party member in that place.";
    return false;
  }
  if (member1 == 1) {
    reply = "Leader cannot leave Party!";
    return false;
  }
  if (n <= 1) {
    // UNKNOWN: wording of the last-member refusal; the rule is taken from sub159D.
    reply = "The party cannot be left empty.";
    return false;
  }
  Vec<Str> left = rosterLeftBehind(s);
  const size_t sn = static_cast<size_t>(n);
  const size_t total = sn + left.size();
  if (total > rosterCapacity(s)) {
    reply = "The roster has no room for that record.";
    return false;
  }
  const size_t r = static_cast<size_t>(member1 - 1);
  const Str name = s.game.party.members[r].name;
  // Rotating [r, total) left by one sends the removed record to the end of the roster and
  // shifts the party and the already left-out records down one slot.
  rosterRotateRecords(s, r, r + 1, total);
  s.game.party.size = static_cast<decltype(s.game.party.size)>(n - 1);
  left.push_back(name);
  rosterWriteLeft(s, left);
  reply = name + " leaves the party.";
  rosterSay(s, "roster.unload", reply, name);
  return true;
}

// Brings the named left-out member back to the end of the party (max kPartyMaxSize).
bool rosterLoadMember(Session& s, const Str& name, Str& reply) {
  Vec<Str> left = rosterLeftBehind(s);
  const i32 j = rosterFindLeft(left, name);
  if (j < 0) {
    reply = "No one by that name is out of the party.";
    return false;
  }
  const i32 n = accPartySize(s);
  if (n >= kPartyMaxSize) {
    reply = "The party is full.";
    return false;
  }
  const size_t sn = static_cast<size_t>(n);
  const size_t from = sn + static_cast<size_t>(j);
  if (from >= rosterCapacity(s)) {
    reply = "The roster has no room for that record.";
    return false;
  }
  // Rotating [n, from+1) right by one brings the record at slot `from` to slot n.
  rosterRotateRecords(s, sn, from, from + 1);
  s.game.party.size = static_cast<decltype(s.game.party.size)>(n + 1);
  const Str who = s.game.party.members[sn].name;
  left.erase(left.begin() + static_cast<std::ptrdiff_t>(j));
  rosterWriteLeft(s, left);
  reply = who + " rejoins the party.";
  rosterSay(s, "roster.load", reply, who);
  return true;
}

bool cmdRoster(Session& s, const Vec<Str>& w, Str& reply) {
  (void)w;
  reply = describeParty(s);
  Vec<Str> left = rosterLeftBehind(s);
  for (const Str& n : left) {
    reply += "\nNot in party: " + n;
  }
  return true;
}

bool cmdUnload(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "unload <member>";
    return true;
  }
  i64 v = 0;
  i32 member = 0;
  if (accParseInt(w[1], v)) {
    member = (v >= 1 && v <= kPartyMaxSize) ? static_cast<i32>(v) : 0;
  } else {
    member = accMemberByName(s, w[1]);
  }
  if (member == 0) {
    reply = "No party member by that name or number.";
    return true;
  }
  rosterUnloadMember(s, member, reply);
  return true;
}

bool cmdLoad(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "load <name>";
    return true;
  }
  rosterLoadMember(s, w[1], reply);
  return true;
}

}  // namespace

void register_roster() {
  registerFeatureCommand({"roster", Screen::Town, &cmdRoster, "roster  - list the party and those left out of it"});
  registerFeatureCommand({"unload", Screen::Town, &cmdUnload, "unload <member>  - take a member out of the party (record kept)"});
  registerFeatureCommand({"load", Screen::Town, &cmdLoad, "load <name>  - bring a left-out member back into the party (max 4)"});
}

}  // namespace mordor

// Entry points used by the feature test (global scope, as the test links them).
bool rosterUnload(mordor::Session& s, int member, mordor::Str& reply) {
  return mordor::rosterUnloadMember(s, static_cast<mordor::i32>(member), reply);
}

bool rosterLoad(mordor::Session& s, const mordor::Str& name, mordor::Str& reply) {
  return mordor::rosterLoadMember(s, name, reply);
}
