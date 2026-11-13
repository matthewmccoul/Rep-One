// game_save.cpp: text save and load of the mutable session state (GAME LAYER).
//
// Format: one "key=value" line per field, ASCII. Fixed-size int arrays are a
// single comma list ("stats=15,12,10,14,8,13"). Text values escape '\\', '\n'
// and '\r'. Keys are prefixed by scope: "party.", "m<i>." (member), "m<i>.g<g>."
// (guild), "m<i>.p<s>" (pack slot), "f<f>." (floor), "gm<k>." (guildmaster).
//
// One traversal (visitGame) serves both directions. Saving copies the GameState
// and visits it in write mode; loading visits a fresh GameState in read mode.
// The two cannot drift apart because they share the same code path.
//
// Assumes Str and Vec support the std::string / std::vector operations used
// here: size, operator[], +=, +, substr, clear, push_back, resize.
//
// NOT SAVED:
//   RNG state, EventLog, Session::lastError (set on load only), Session::data,
//   Options (kept in settings.ini), Encounter and all combat transients
//   (encounter, castFlag, refusedOrder, party foes/groups/combatOrders/itemOrders,
//   character combat ratings, orders, resistances), companion records
//   (no GameState link except Party::aliveCompanions and carryingCompanion),
//   character bank, worn, knownSpellIds, activeEffectIds, explored, mods,
//   raisedBy, carryingCharacterName, guild abilityRating (f32), joinList,
//   morgueList, Item findMonsterId/lockLevel/location, area occupants, foes,
//   standingGroup and waiting, square chest/body/encounter/foe fields,
//   library, hall records, roster, saved parties, buffers, messages, guild log,
//   mod state, and the remaining GameState fields.
//   SIMPLIFIED: SquareState::automap is saved as the seen flag (automap != 0)
//   and loads back as 0 or 1. Party members beyond Party::size are not saved.

#include <cstddef>

#include "mordor/game.h"
#include "mordor/shared_helpers.h"
#include "mordor/parse.h"

namespace mordor {
namespace {

constexpr i64 kI64Max = 9223372036854775807LL;
constexpr i64 kAnyLo = -kI64Max;
constexpr i64 kAnyHi = kI64Max;
constexpr i32 kFormatVersion = 1;
constexpr i32 kAreaSlots = 201;          // Arr<Area, 201> in FloorState
constexpr i32 kMaxFloorCount = 256;
constexpr i32 kMaxSquareCount = 1000000;
constexpr i32 kMaxRowCount = 100000;
constexpr i32 kMaxListLen = 16;
constexpr i32 kItemFields = 14;
constexpr i32 kAreaFields = 7;
constexpr i32 kBitRows = 3;
const char* const kBitNames[kBitRows] = {"explored", "identified", "seen"};

// ---------------------------------------------------------------- primitives


bool strToI64(const Str& s, i64& out) {
  return parseDecimal(s, out, true, 0, -1);
}

bool inRange(i64 v, i64 lo, i64 hi) { return v >= lo && v <= hi; }

Str escapeText(const Str& s) {
  Str out;
  for (std::size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out += c;
    }
  }
  return out;
}

bool unescapeText(const Str& s, Str& out) {
  out = Str();
  for (std::size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c != '\\') {
      out += c;
      continue;
    }
    if (i + 1 >= s.size()) return false;
    char d = s[++i];
    if (d == '\\') {
      out += '\\';
    } else if (d == 'n') {
      out += '\n';
    } else if (d == 'r') {
      out += '\r';
    } else {
      return false;
    }
  }
  return true;
}

void splitLines(const Str& text, Vec<Str>& lines) {
  Str cur;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\n') {
      lines.push_back(cur);
      cur = Str();
    } else if (c != '\r') {
      cur += c;
    }
  }
  if (cur.size() > 0) lines.push_back(cur);
}

// ---------------------------------------------------------------- item and area helpers

// Item flag bits: guildIssued 1, aligned 2, chestTeleporter 4, destructible 8,
// equipped 16, lost 32, slotCursed 64.
i64 itemFlagBits(const Item& it) {
  i64 b = 0;
  if (it.guildIssued) b |= 1;
  if (it.aligned) b |= 2;
  if (it.chestTeleporter) b |= 4;
  if (it.destructible) b |= 8;
  if (it.equipped) b |= 16;
  if (it.lost) b |= 32;
  if (it.slotCursed) b |= 64;
  return b;
}

void applyItemFlags(Item& it, i64 b) {
  it.guildIssued = (b & 1) != 0;
  it.aligned = (b & 2) != 0;
  it.chestTeleporter = (b & 4) != 0;
  it.destructible = (b & 8) != 0;
  it.equipped = (b & 16) != 0;
  it.lost = (b & 32) != 0;
  it.slotCursed = (b & 64) != 0;
}

bool areaEmpty(const Area& a) {
  return a.stamp == 0 && a.emptiedAt == 0 && a.respawnT == 0 && a.lairId == 0 &&
         a.spawnMask == 0 && a.fight == 0;
}

bool getBit(const SquareState& sq, i32 which) {
  if (which == 0) return sq.explored;
  if (which == 1) return sq.identified;
  return sq.automap != 0;  // SIMPLIFIED: seen flag only
}

void setBit(SquareState& sq, i32 which, bool v) {
  if (which == 0) {
    sq.explored = v;
  } else if (which == 1) {
    sq.identified = v;
  } else {
    sq.automap = v ? 1 : 0;
  }
}

// ---------------------------------------------------------------- the traversal

// Io serves both directions. In write mode it appends lines to out. In read mode
// it consumes lines from lines in order. Once bad is set, every call is a no-op.
struct Io {
  bool loading = false;
  Str prefix;
  Str out;
  Vec<Str> lines;
  std::size_t pos = 0;
  bool bad = false;
  Str err;

  void fail(const Str& why) {
    if (!bad) {
      bad = true;
      err = why;
    }
  }

  void put(const Str& key, const Str& value) {
    out += key;
    out += '=';
    out += value;
    out += '\n';
  }

  // Reads the next line, which must be exactly "key=...".
  bool take(const Str& key, Str& value) {
    if (bad) return false;
    if (pos >= lines.size()) {
      fail(Str("unexpected end of save, expected ") + key);
      return false;
    }
    const Str& line = lines[pos];
    std::size_t n = key.size();
    bool match = line.size() > n && line[n] == '=';
    for (std::size_t i = 0; match && i < n; ++i) {
      if (line[i] != key[i]) match = false;
    }
    if (!match) {
      fail(Str("expected ") + key + " at line " + toStr(static_cast<i64>(pos + 1)));
      return false;
    }
    value = line.substr(n + 1);
    ++pos;
    return true;
  }

  template <typename T>
  void num(const Str& name, T& v, i64 lo = kAnyLo, i64 hi = kAnyHi) {
    Str key = prefix + name;
    if (loading) {
      Str t;
      i64 n = 0;
      if (!take(key, t)) return;
      if (!strToI64(t, n) || !inRange(n, lo, hi)) {
        fail(Str("bad number for ") + key);
        return;
      }
      v = static_cast<T>(n);
    } else {
      put(key, toStr(static_cast<i64>(v)));
    }
  }

  void flag(const Str& name, bool& v) {
    i64 t = v ? 1 : 0;
    num(name, t, 0, 1);
    if (loading) v = (t == 1);
  }

  void text(const Str& name, Str& v) {
    Str key = prefix + name;
    if (loading) {
      Str t;
      if (!take(key, t)) return;
      if (!unescapeText(t, v)) fail(Str("bad text for ") + key);
    } else {
      put(key, escapeText(v));
    }
  }

  // Comma list of exactly n integers.
  void ints(const Str& name, i64* v, i32 n) {
    Str key = prefix + name;
    if (!loading) {
      Str t;
      for (i32 k = 0; k < n; ++k) {
        if (k > 0) t += ',';
        t += toStr(v[k]);
      }
      put(key, t);
      return;
    }
    Str t;
    if (!take(key, t)) return;
    i32 idx = 0;
    Str field;
    for (std::size_t i = 0; i <= t.size(); ++i) {
      if (i == t.size() || t[i] == ',') {
        if (idx >= n || !strToI64(field, v[idx])) {
          fail(Str("bad list for ") + key);
          return;
        }
        ++idx;
        field = Str();
      } else {
        field += t[i];
      }
    }
    if (idx != n) fail(Str("bad list length for ") + key);
  }

  // Fixed int array (n <= kMaxListLen) stored as one comma list.
  void intArr(const Str& name, i32* a, i32 n) {
    i64 tmp[kMaxListLen] = {};
    for (i32 k = 0; k < n; ++k) tmp[k] = loading ? 0 : a[k];
    ints(name, tmp, n);
    if (loading && !bad) {
      for (i32 k = 0; k < n; ++k) a[k] = static_cast<i32>(tmp[k]);
    }
  }
};

// Scoped key prefix: base + index + "." while in scope.
struct Scope {
  Io& io;
  Str saved;
  Scope(Io& i, const Str& base, i32 n) : io(i), saved(i.prefix) {
    io.prefix = saved + base + toStr(static_cast<i64>(n)) + ".";
  }
  ~Scope() { io.prefix = saved; }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
};

void visitItem(Io& io, const Str& name, Item& it) {
  i64 f[kItemFields] = {};
  if (!io.loading) {
    f[0] = it.id;
    f[1] = it.ailments;
    f[2] = it.charges;
    f[3] = it.maxCharges;
    f[4] = it.level;
    f[5] = it.spellId;
    f[6] = it.spellIndex;
    f[7] = itemFlagBits(it);
    f[8] = static_cast<i64>(it.type);
    f[9] = static_cast<i64>(it.idLevel);
    f[10] = static_cast<i64>(it.alignment);
    f[11] = it.alignFlags;
    f[12] = static_cast<i64>(it.cursed);
    f[13] = static_cast<i64>(it.chestTrap);
  }
  io.ints(name, f, kItemFields);
  if (!io.loading || io.bad) return;
  if (!inRange(f[7], 0, 127) || !inRange(f[8], 0, 8) || !inRange(f[9], 0, 4) ||
      !inRange(f[10], 0, 3) || !inRange(f[11], 0, 255) || !inRange(f[12], 0, 2) ||
      !inRange(f[13], 0, 10)) {
    io.fail(Str("bad item in ") + name);
    return;
  }
  it.id = static_cast<i32>(f[0]);
  it.ailments = static_cast<i32>(f[1]);
  it.charges = static_cast<i32>(f[2]);
  it.maxCharges = static_cast<i32>(f[3]);
  it.level = static_cast<i32>(f[4]);
  it.spellId = static_cast<i32>(f[5]);
  it.spellIndex = static_cast<i32>(f[6]);
  it.type = static_cast<ItemType>(f[8]);
  it.idLevel = static_cast<IdLevel>(f[9]);
  it.alignment = static_cast<ItemAlign>(f[10]);
  it.alignFlags = static_cast<u8>(f[11]);
  it.cursed = static_cast<CurseKind>(f[12]);
  it.chestTrap = static_cast<ChestTrap>(f[13]);
  applyItemFlags(it, f[7]);
}

void visitGuild(Io& io, GuildMembership& m) {
  io.flag("joined", m.joined);
  io.num("level", m.level);
  io.num("experience", m.experience);
  io.num("att", m.att);
  io.num("def", m.def);
  io.num("guildGold", m.guildGold);
  io.flag("questDone", m.questDone);
  io.num("questKind", m.quest.kind, 0, 3);
  io.num("questState", m.quest.state, 0, 2);
  io.num("questTarget", m.quest.target);
  io.num("questLevel", m.quest.level);
  io.num("questCount", m.quest.count);
  io.flag("questPinned", m.quest.pinned);
}

void visitCharacter(Io& io, Character& c, Inventory& inv) {
  io.text("name", c.name);
  io.text("password", c.password);
  io.num("race", c.race, 0, 8);
  io.num("sex", c.sex, 0, 1);
  io.num("alignment", c.alignment, 0, 2);
  io.num("activeGuild", c.activeGuild, 0, kGuildCount - 1);
  io.num("level", c.level);
  io.num("experience", c.experience);
  io.num("xpRequired", c.xpRequired);
  io.num("ageDays", c.ageDays);
  io.num("deathState", c.deathState, 0, 8);
  io.num("deaths", c.deaths);
  io.num("kills", c.kills);
  io.num("curHits", c.vitals.curHits);
  io.num("maxHits", c.vitals.maxHits);
  io.num("curSpellPoints", c.vitals.curSpellPoints);
  io.num("maxSpellPoints", c.vitals.maxSpellPoints);
  io.num("goldOnHand", c.wallet.goldOnHand);
  io.num("goldInBank", c.wallet.goldInBank);
  io.intArr("stats", &c.abilities.stats[0], kStatCount);
  io.intArr("statMax", &c.abilities.statMax[0], kStatCount);
  io.num("poisonLevel", c.ailments.poisonLevel);
  io.num("diseaseLevel", c.ailments.diseaseLevel);
  io.num("paralysisLevel", c.ailments.paralysisLevel);
  io.num("paralyzedRounds", c.ailments.paralyzedRounds);
  io.num("protectLevel", c.ailments.protectLevel);
  io.num("ailmentSteps", c.ailments.ailmentSteps);
  io.flag("paralysed", c.ailments.paralysed);
  io.flag("rocked", c.ailments.rocked);
  io.flag("levitating", c.ailments.levitating);
  io.flag("invisible", c.ailments.invisible);
  io.flag("seeInvisible", c.ailments.seeInvisible);
  io.num("locX", c.location.x);
  io.num("locY", c.location.y);
  io.num("locDungeonLevel", c.location.dungeonLevel);
  io.num("locArea", c.location.area);
  io.num("locFacing", c.location.facing);
  io.flag("lostPosition", c.lost.position);
  io.flag("lostLevel", c.lost.level);
  io.flag("lostFacing", c.lost.facing);
  io.num("freeHands", inv.freeHands);
  io.num("carryIndex", inv.carryIndex);
  io.flag("hidden", inv.hidden);
  for (i32 s = 0; s < kPackSlots; ++s) {
    visitItem(io, Str("p") + toStr(static_cast<i64>(s)), inv.pack[s]);
  }
  for (i32 g = 0; g < kGuildCount; ++g) {
    Scope sc(io, "g", g);
    visitGuild(io, c.guilds[g]);
  }
}

void visitParty(Io& io, Party& p) {
  io.num("party.size", p.size, 0, kPartyMaxSize);
  io.num("party.dungeonLevel", p.dungeonLevel);
  io.num("party.x", p.x);
  io.num("party.y", p.y);
  io.num("party.facing", p.facing, 0, 3);
  io.flag("party.inDungeon", p.inDungeon);
  io.flag("party.lost", p.lost);
  io.flag("party.lostFacing", p.lostFacing);
  io.flag("party.lostLevel", p.lostLevel);
  io.flag("party.lostPosition", p.lostPosition);
  io.num("party.carryingSlot", p.carryingSlot, 0, 2);
  io.num("party.carryingCompanion", p.carryingCompanion);
  io.num("party.aliveCompanions", p.aliveCompanions);
  io.num("party.fightGold", p.fightGold);
  io.num("party.goldLeaderIndex", p.goldLeaderIndex);
  for (i32 i = 0; i < p.size && i < kPartyMaxSize; ++i) {
    Scope sc(io, "m", i);
    visitCharacter(io, p.members[i], p.inventories[i]);
  }
}

void visitFloor(Io& io, FloorState& f) {
  io.num("dungeonLevel", f.dungeonLevel);
  io.num("tally", f.tally);
  io.num("roomTally", f.roomTally);
  io.num("roomTotal", f.roomTotal);
  io.num("roomStamp", f.roomStamp);

  // Squares: three bit rows (explored, identified, seen), one char per square.
  i32 ns = static_cast<i32>(f.squares.size());
  io.num("squareCount", ns, 0, kMaxSquareCount);
  if (io.loading) {
    if (io.bad) return;
    f.squares.resize(static_cast<std::size_t>(ns));
  }
  std::size_t nsz = static_cast<std::size_t>(ns);
  Str bits[kBitRows];
  if (!io.loading) {
    for (std::size_t k = 0; k < nsz; ++k) {
      for (i32 w = 0; w < kBitRows; ++w) {
        bits[w] += (getBit(f.squares[k], w) ? '1' : '0');
      }
    }
  }
  for (i32 w = 0; w < kBitRows; ++w) io.text(kBitNames[w], bits[w]);
  if (io.loading && !io.bad) {
    for (i32 w = 0; w < kBitRows; ++w) {
      if (bits[w].size() != nsz) {
        io.fail("bad square row length");
        return;
      }
      for (std::size_t k = 0; k < nsz; ++k) {
        char c = bits[w][k];
        if (c != '0' && c != '1') {
          io.fail("bad square bit");
          return;
        }
        setBit(f.squares[k], w, c == '1');
      }
    }
  }

  // Area respawn data: one "area=" row per non-empty area, index first.
  if (io.bad) return;
  i32 na = 0;
  i32 cur = 0;
  if (!io.loading) {
    for (i32 a = 0; a < kAreaSlots; ++a) {
      if (!areaEmpty(f.areas[a])) ++na;
    }
  }
  io.num("areaRows", na, 0, kAreaSlots);
  if (io.loading) {
    if (io.bad) return;
    for (i32 a = 0; a < kAreaSlots; ++a) f.areas[a] = Area();
  }
  for (i32 k = 0; k < na; ++k) {
    i64 r[kAreaFields] = {};
    if (!io.loading) {
      while (cur < kAreaSlots && areaEmpty(f.areas[cur])) ++cur;
      const Area& a = f.areas[cur];
      r[0] = cur;
      r[1] = a.stamp;
      r[2] = a.emptiedAt;
      r[3] = a.respawnT;
      r[4] = a.lairId;
      r[5] = a.spawnMask;
      r[6] = a.fight;
      ++cur;
    }
    io.ints("area", r, kAreaFields);
    if (io.loading && !io.bad) {
      if (!inRange(r[0], 0, kAreaSlots - 1)) {
        io.fail("bad area index");
        return;
      }
      Area& a = f.areas[static_cast<i32>(r[0])];
      a.stamp = r[1];
      a.emptiedAt = static_cast<i32>(r[2]);
      a.respawnT = static_cast<i32>(r[3]);
      a.lairId = static_cast<i32>(r[4]);
      a.spawnMask = static_cast<i32>(r[5]);
      a.fight = static_cast<i32>(r[6]);
    }
  }
}

void visitFloors(Io& io, Vec<FloorState>& floors) {
  i32 nf = static_cast<i32>(floors.size());
  io.num("floorCount", nf, 0, kMaxFloorCount);
  if (io.loading) {
    if (io.bad) return;
    floors.resize(static_cast<std::size_t>(nf));
  }
  for (i32 f = 0; f < nf; ++f) {
    Scope sc(io, "f", f);
    visitFloor(io, floors[f]);
  }
}

void visitTown(Io& io, GameState& g) {
  io.num("townIncome", g.townIncome);
  io.intArr("storeRowCount", &g.storeRowCount[0], 3);
  io.num("storeUnalignedStock", g.storeUnalignedStock);
  i32 nr = static_cast<i32>(g.store.size());
  io.num("storeRows", nr, 0, kMaxRowCount);
  if (io.loading) {
    if (io.bad) return;
    g.store.resize(static_cast<std::size_t>(nr));
  }
  for (i32 k = 0; k < nr; ++k) io.intArr("store", &g.store[k][0], 4);
  for (i32 k = 0; k < kGuildCount; ++k) {
    Scope sc(io, "gm", k);
    GuildMaster& m = g.guildMasters[k];
    io.num("holder", m.holder);
    io.num("level", m.level);
    io.text("name", m.name);
  }
}

void visitGame(Io& io, GameState& g) {
  io.flag("inCity", g.inCity);
  io.flag("inDungeon", g.inDungeon);
  io.flag("inCombat", g.inCombat);
  io.flag("underground", g.underground);
  io.num("dungeonLevel", g.dungeonLevel);
  io.num("area", g.area);
  io.num("deepestReached", g.deepestReached);
  io.num("deepestStocked", g.deepestStocked);
  io.num("levelCount", g.levelCount);
  io.text("partyPassword", g.partyPassword);

  // Clocks.
  io.num("timeOfDay", g.timeOfDay);
  io.num("today", g.today);
  io.num("lastTickMs", g.lastTickMs);
  io.num("playElapsedMs", g.playElapsedMs);
  io.flag("playClockRunning", g.playClockRunning);
  io.num("playIntervalMs", g.playIntervalMs);
  io.flag("paused", g.paused);
  io.num("ailmentElapsedMs", g.ailmentElapsedMs);
  io.num("ailmentIntervalMs", g.ailmentIntervalMs);
  io.flag("inAilment", g.inAilment);
  io.num("meleeWaitMs", g.meleeWaitMs);
  io.flag("combatClockRunning", g.combatClockRunning);
  io.num("combatIntervalMs", g.combatIntervalMs);

  visitTown(io, g);
  visitFloors(io, g.floors);
  visitParty(io, g.party);
}

}  // namespace

// ---------------------------------------------------------------- public API

Str saveSession(const Session& s) {
  GameState g = s.game;  // copy: the traversal takes mutable references
  Io io;
  io.loading = false;
  i32 version = kFormatVersion;
  io.num("format", version);
  visitGame(io, g);
  i32 endMark = 1;
  io.num("end", endMark);
  return io.out;
}

bool loadSession(Session& s, const Str& text, Str& err) {
  Io io;
  io.loading = true;
  splitLines(text, io.lines);
  i32 version = 0;
  io.num("format", version, kFormatVersion, kFormatVersion);
  GameState g{};
  visitGame(io, g);
  i32 endMark = 0;
  io.num("end", endMark, 1, 1);
  if (!io.bad && io.pos != io.lines.size()) {
    io.fail("unexpected data after end marker");
  }
  if (io.bad) {
    err = io.err;
    s.lastError = io.err;
    return false;
  }
  s.game = g;  // data tables and rng are untouched
  s.lastError = Str();
  err = Str();
  return true;
}

}  // namespace mordor
