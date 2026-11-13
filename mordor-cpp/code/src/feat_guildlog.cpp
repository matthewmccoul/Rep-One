#include "mordor/features.h"

#include <cstddef>

#include "mordor/access.h"
#include "mordor/state_common.h"
#include "mordor/shared_helpers.h"
#include "mordor/parse.h"

namespace mordor {
namespace {

// Storage in s.featureState (the core writes no files):
//   guildlog.count     decimal count of entries
//   guildlog.e<i>      "<guild>|<day>|<text>", i = 0 is the oldest
//   guildlog.today     current day serial, set by the caller. NEED: game day counter.
//   guildlog.seeded    "1" once the new-game guildmaster appointments have been written
//   guildlog.text      GUILDLOG.TXT equivalent: lines removed by a clean, oldest first

struct GuildlogRow {
  i32 guild = 0;
  i64 day = 0;
  Str text;
};

}  // namespace
namespace {

// Parses a decimal integer without exceptions. ok is false on any non-digit.
i64 glParseInt(const Str& t, bool& ok) {
  i64 v = 0;
  ok = parseDecimal(t, v, true, 0, -1);
  return ok ? v : 0;
}

Str glTrim(const Str& t) {
  const size_t b = t.find_first_not_of(" \t");
  if (b == Str::npos) return Str();
  const size_t e = t.find_last_not_of(" \t");
  return t.substr(b, e - b + 1);
}

// UNKNOWN: day counter not in GameState; reads 0 until the caller sets guildlog.today.
i64 glToday(const Session& s) {
  bool ok = false;
  const i64 d = glParseInt(featureStateOr(s, "guildlog.today", Str()), ok);
  return ok ? d : 0;
}

// Number of guild halls that exist, capped at the state constant.
i32 glGuildCount(const Session& s) {
  const size_t n = s.data.guilds.size();
  const size_t cap = static_cast<size_t>(kGuildCount);
  return static_cast<i32>(n < cap ? n : cap);
}

Str glGuildName(const Session& s, i32 guild) {
  if (guild < 0 || guild >= glGuildCount(s)) return std::to_string(guild);
  return glTrim(s.data.guilds[static_cast<size_t>(guild)].name);
}

// The party leader (member 1) signs the clean record. Empty when no party.
Str glLeaderName(Session& s) {
  Character* c = accMember(s, 1);
  return c != nullptr ? glTrim(c->name) : Str();
}

Str glEntryKey(i64 index) { return Str("guildlog.e") + std::to_string(index); }

Str glPack(const GuildlogRow& r) {
  return std::to_string(r.guild) + "|" + std::to_string(r.day) + "|" + r.text;
}

bool glUnpack(const Str& raw, GuildlogRow& r) {
  const size_t p1 = raw.find('|');
  if (p1 == Str::npos) return false;
  const size_t p2 = raw.find('|', p1 + 1);
  if (p2 == Str::npos) return false;
  bool okG = false;
  bool okD = false;
  const i64 g = glParseInt(raw.substr(0, p1), okG);
  const i64 d = glParseInt(raw.substr(p1 + 1, p2 - p1 - 1), okD);
  if (!okG || !okD) return false;
  r.guild = static_cast<i32>(g);
  r.day = d;
  r.text = raw.substr(p2 + 1);
  return true;
}

// Oldest first. A damaged record is skipped.
Vec<GuildlogRow> glLoadAll(const Session& s) {
  Vec<GuildlogRow> rows;
  bool ok = false;
  const i64 n = glParseInt(featureStateOr(s, "guildlog.count", Str()), ok);
  if (!ok || n <= 0) return rows;
  for (i64 i = 0; i < n; ++i) {
    GuildlogRow r;
    if (glUnpack(featureStateOr(s, glEntryKey(i), Str()), r)) rows.push_back(r);
  }
  return rows;
}

// Replaces the whole log and drops any stale keys past the new end.
void glStoreAll(Session& s, const Vec<GuildlogRow>& rows) {
  bool ok = false;
  i64 old = glParseInt(featureStateOr(s, "guildlog.count", Str()), ok);
  if (!ok || old < 0) old = 0;
  const i64 n = static_cast<i64>(rows.size());
  for (i64 i = 0; i < n; ++i) s.featureState[glEntryKey(i)] = glPack(rows[static_cast<size_t>(i)]);
  for (i64 i = n; i < old; ++i) s.featureState.erase(glEntryKey(i));
  s.featureState["guildlog.count"] = std::to_string(n);
}

// R8 / R44 labels, relative to today. The gap is absolute, so a clock set backwards still reads "ago".
Str glPrefix(i64 day, i64 today) {
  const i64 gap = day > today ? day - today : today - day;
  if (gap == 0) return "TODAY: ";
  if (gap == 1) return "1 DAY AGO: ";
  return std::to_string(gap) + " DAYS AGO: ";
}


bool glParseGuild(const Session& s, const Str& arg, i32& guild) {
  const Str want = accLower(glTrim(arg));
  if (want.empty()) return false;
  const i32 count = glGuildCount(s);
  bool ok = false;
  const i64 g = glParseInt(want, ok);
  if (ok) {
    if (g < 0 || g >= static_cast<i64>(count)) return false;
    guild = static_cast<i32>(g);
    return true;
  }
  for (i32 i = 0; i < count; ++i) {
    if (accLower(glGuildName(s, i)) == want) {
      guild = i;
      return true;
    }
  }
  return false;
}

// GuildLog.NeedsCleaning: over fifty per guild across the whole log.
bool glNeedsCleaning(const Session& s) {
  const i64 total = static_cast<i64>(glLoadAll(s).size());
  i64 guilds = static_cast<i64>(glGuildCount(s));
  if (guilds < 1) guilds = 1;
  return total > static_cast<i64>(kGuildLogKeepOnClean) * guilds;
}

// The clean record, as the GUILDLOG.TXT writer lays it out (CHECK the exact layout against the file).
// UNKNOWN: the original dates each line with a calendar date; the day serial is stored here instead.
void glArchive(Session& s, const Vec<GuildlogRow>& removed, i64 today) {
  Str text = featureStateOr(s, "guildlog.text", Str());
  text += "BEGIN GUILD LOG CLEAN (day " + std::to_string(today) + ")\n";
  text += "Done By: \"" + glLeaderName(s) + "\"\n";
  for (const GuildlogRow& r : removed) {
    text += "GUILD: " + glGuildName(s, r.guild) + "\n";
    text += " DATE: " + std::to_string(r.day) + "\n";
    text += "  LOG: " + r.text + "\n";
  }
  s.featureState["guildlog.text"] = text;
}

// sub316B: keep the newest kGuildLogKeepOnClean entries across the whole log.
// R65: discarded lines are archived to guildlog.text before the log is cut.
// UNKNOWN: exact trigger point; here cleaning runs only when the player asks.
bool glClean(Session& s, Str& reply) {
  if (!glNeedsCleaning(s)) {
    reply = "The guild log is not over the cleaning threshold.";
    return true;
  }
  const Vec<GuildlogRow> rows = glLoadAll(s);
  const i64 keep = static_cast<i64>(kGuildLogKeepOnClean);
  const i64 total = static_cast<i64>(rows.size());
  if (total <= keep) {
    reply = "Nothing to clean.";
    return true;
  }
  const i64 cut = total - keep;
  const Vec<GuildlogRow> removed(rows.begin(), rows.begin() + static_cast<std::ptrdiff_t>(cut));
  const Vec<GuildlogRow> kept(rows.begin() + static_cast<std::ptrdiff_t>(cut), rows.end());
  glArchive(s, removed, glToday(s));
  glStoreAll(s, kept);
  reply = "Guild log cleaned: " + std::to_string(cut) + " entries removed, " + std::to_string(keep) + " kept.";
  return true;
}

// guildlog <guild> | guildlog clean
bool cmdGuildlog(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "Usage: guildlog <guild number 0-11> | guildlog clean";
    return false;
  }
  if (accLower(w[1]) == "clean") return glClean(s, reply);

  i32 guild = 0;
  if (!glParseGuild(s, joinFrom(w, 1), guild)) {
    reply = "No such guild.";
    return false;
  }

  const Vec<GuildlogRow> rows = glLoadAll(s);
  const i64 today = glToday(s);
  Str out;
  bool first = true;
  // Newest first, as GuildLog.For reverses the list.
  for (size_t i = rows.size(); i-- > 0;) {
    if (rows[i].guild != guild) continue;
    if (!first) out += "\n";
    out += glPrefix(rows[i].day, today) + rows[i].text;
    first = false;
  }
  reply = out.empty() ? Str("No Entries") : out;
  return true;
}

}  // namespace

void register_guildlog() {
  registerFeatureCommand({"guildlog", Screen::Town, &cmdGuildlog,
                          "guildlog <guild 0-11>  - show a guild's log;  guildlog clean  - keep the newest 50"});
}

}  // namespace mordor
