// game_view.cpp: pure text rendering of the session state. No side effects.
#include "mordor/game.h"
#include "mordor/attributes.h"
#include "mordor/city1.h"
#include "mordor/dungeon1a.h"
#include "mordor/guilds1.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

constexpr i32 kViewMapWidth = 30;     // UNKNOWN: flat map width; SRD R19 addresses squares with a width of 30
constexpr i32 kViewMaxRadius = 20;
constexpr i32 kViewTownSections = 5;  // TownSection: Store, Confinement, Morgue, Guild, Bank


// Currency is i64 in 1/10000 gold.
Str viewGold(Currency c) {
  const bool neg = c < 0;
  const i64 a = neg ? -c : c;
  const Str frac = toStr(a % 10000);
  const Str pad(4 - frac.size(), '0');
  return Str(neg ? "-" : "") + toStr(a / 10000) + "." + pad + frac;
}

Str raceName(Race r) {
  switch (r) {
    case Race::Human: return "Human";
    case Race::Elf: return "Elf";
    case Race::Giant: return "Giant";
    case Race::Gnome: return "Gnome";
    case Race::Dwarf: return "Dwarf";
    case Race::Ogre: return "Ogre";
    case Race::Morloch: return "Morloch";
    case Race::Osiri: return "Osiri";
    case Race::Troll: return "Troll";
  }
  return "?";
}

// SIMPLIFIED: GameData race/guild name tables are not visible in the given headers,
// so names are local switches over the enums.
Str guildName(Guild g) {
  switch (g) {
    case Guild::Nomad: return "Nomad";
    case Guild::Warrior: return "Warrior";
    case Guild::Paladin: return "Paladin";
    case Guild::Ninja: return "Ninja";
    case Guild::Villain: return "Villain";
    case Guild::Seeker: return "Seeker";
    case Guild::Thief: return "Thief";
    case Guild::Scavenger: return "Scavenger";
    case Guild::Mage: return "Mage";
    case Guild::Sorcerer: return "Sorcerer";
    case Guild::Wizard: return "Wizard";
    case Guild::Healer: return "Healer";
  }
  return "?";
}

Str alignName(Align a) {
  switch (a) {
    case Align::Good: return "Good";
    case Align::Neutral: return "Neutral";
    case Align::Evil: return "Evil";
  }
  return "?";
}

Str deathName(DeathState d) {
  switch (d) {
    case DeathState::Alive: return "alive";
    case DeathState::Dead: return "dead";
    case DeathState::Stoned: return "stoned";
    case DeathState::Destroyed: return "destroyed";
    case DeathState::Immolated: return "immolated";
    case DeathState::Frozen: return "frozen";
    case DeathState::Annihilated: return "annihilated";
    case DeathState::Melted: return "melted";
    case DeathState::Rock: return "rock";
  }
  return "?";
}

Str groupStateName(GroupState g) {
  switch (g) {
    case GroupState::Hostile: return "hostile";
    case GroupState::Engaged: return "engaged";
    case GroupState::Peaced: return "peaceful";
  }
  return "?";
}

Str townSectionName(TownSection t) {
  switch (t) {
    case TownSection::Store: return "Store";
    case TownSection::Confinement: return "Confinement";
    case TownSection::Morgue: return "Morgue";
    case TownSection::Guild: return "Guild";
    case TownSection::Bank: return "Bank";
  }
  return "?";
}

Str facingName(Facing f) {
  switch (f) {
    case Facing::N: return "N";
    case Facing::E: return "E";
    case Facing::S: return "S";
    case Facing::W: return "W";
  }
  return "?";
}

Str edgeName(EdgeClass e) {
  switch (e) {
    case EdgeClass::Wall: return "wall";
    case EdgeClass::Door: return "door";
    case EdgeClass::SecretDoor: return "secret door";
    case EdgeClass::Open: return "open";
  }
  return "?";
}

// k is relative to the facing: 0 ahead, 1 right, 2 behind, 3 left.
Str relName(i32 k) {
  switch (k) {
    case 0: return "ahead";
    case 1: return "right";
    case 2: return "behind";
    default: return "left";
  }
}

Str statAbbrev(i32 k) {
  switch (k) {
    case 0: return "STR";
    case 1: return "INT";
    case 2: return "WIS";
    case 3: return "CON";
    case 4: return "CHA";
    default: return "DEX";
  }
}

// Grid step for a facing: N (0,+1), E (+1,0), S (0,-1), W (-1,0).
void viewDelta(Facing f, i32& dx, i32& dy) {
  dx = 0;
  dy = 0;
  switch (f) {
    case Facing::N: dy = 1; break;
    case Facing::E: dx = 1; break;
    case Facing::S: dy = -1; break;
    case Facing::W: dx = -1; break;
  }
}

Str viewStatus(const Character& c) {
  Str out;
  auto add = [&out](const Str& w) {
    if (!out.empty()) out += ", ";
    out += w;
  };
  if (c.deathState != DeathState::Alive) add(deathName(c.deathState));
  if (c.ailments.poisonLevel > 0) add("poisoned");
  if (c.ailments.diseaseLevel > 0) add("diseased");
  if (c.ailments.paralysed) add("paralysed");
  return out;
}

// Floor whose dungeonLevel matches the party's level; nullptr if none is stored.
const FloorState* viewFloor(const Session& s) {
  const i32 level = s.game.party.dungeonLevel;
  for (const FloorState& f : s.game.floors) {
    if (f.dungeonLevel == level) return &f;
  }
  return nullptr;
}

// UNKNOWN: squares are stored row-major, index = y * kViewMapWidth + x, 0-based.
const SquareState* viewSquare(const FloorState& f, i32 x, i32 y) {
  // Coordinates are 1-based like the dungeon module (SRD R19): square = x + 30 * (y - 1).
  if (x < 1 || y < 1 || x > kViewMapWidth) return nullptr;
  const i32 idx = (y - 1) * kViewMapWidth + (x - 1);
  if (idx >= static_cast<i32>(f.squares.size())) return nullptr;
  return &f.squares[static_cast<u32>(idx)];
}

}  // namespace

Str describeParty(const Session& s) {
  const Party& p = s.game.party;
  const i32 n = p.size < 0 ? 0 : (p.size > kPartyMaxSize ? kPartyMaxSize : p.size);
  if (n == 0) return "No party members.\n";
  Str out;
  for (i32 i = 0; i < n; ++i) {
    const Character& c = p.members[static_cast<u32>(i)];
    out += "[" + toStr(i + 1) + "] " + c.name  /* members are numbered from 1 in commands */ + ", " + raceName(c.race) + ", " +
           guildName(c.activeGuild) + " level " + toStr(c.activeLevel());
    out += ", HP " + toStr(c.vitals.curHits) + "/" + toStr(c.vitals.maxHits);
    out += ", SP " + toStr(c.vitals.curSpellPoints) + "/" + toStr(c.vitals.maxSpellPoints);
    const Str status = viewStatus(c);
    if (!status.empty()) out += ", " + status;
    out += ", xp " + toStr(static_cast<i32>(c.guilds[static_cast<u32>(c.activeGuild)].experience)) + ", gold " + viewGold(c.wallet.goldOnHand) + "\n";
  }
  return out;
}

Str describeCharacter(const Session& s, i32 member) {
  const Party& p = s.game.party;
  if (member < 0 || member >= p.size || member >= kPartyMaxSize) return "No such party member.\n";
  const Character& c = p.members[static_cast<u32>(member)];
  const Inventory& inv = p.inventories[static_cast<u32>(member)];

  Str out;
  out += c.name + "\n";
  // UNKNOWN: age years = (kAgeStartDays + ageDays) / 365; the SRD day-to-year rule is not in the given context.
  out += "Race " + raceName(c.race) + ", " + alignName(c.alignment) + ", age " +
         toStr((kAgeStartDays + c.ageDays) / 365) + " years\n";

  out += "Guilds:\n";
  for (i32 g = 0; g < kGuildCount; ++g) {
    const GuildMembership& m = c.guilds[static_cast<u32>(g)];
    if (!m.joined) continue;
    const bool active = (g == static_cast<i32>(c.activeGuild));
    out += active ? "  * " : "    ";
    out += guildName(static_cast<Guild>(g)) + " level " + toStr(m.level) + " (" +
           guildTitle(static_cast<Guild>(g), m.level) + ") experience " + toStr(m.experience) + "\n";
  }

  // Natural value, then value with mods from statWithMods.
  out += "Stats:";
  for (i32 k = 0; k < kStatCount; ++k) {
    const StatIndex st = static_cast<StatIndex>(k);
    out += " " + statAbbrev(k) + " " + toStr(c.abilities.stats[static_cast<u32>(k)]) +
           " (" + toStr(statWithMods(c, st)) + ")";
  }
  out += "\n";

  // Total attack/defence from the rules module. totalAD and combatAD take a const Ctx and never draw random numbers,
  // so a context over the session's tables with a throwaway generator and log is enough for a read-only view.
  {
    Rng viewRng(1);
    EventLog viewLog;
    const Ctx adCtx(s.data, viewRng, viewLog);
    const AdPair total = totalAD(adCtx, c, inv);
    out += "Attack " + toStr(total.att) + ", defence " + toStr(total.def) + " (combat A/D " + toStr(combatAD(adCtx, c, inv)) + ")\n";
  }

  out += "Resistances:";
  for (i32 k = 0; k < kResistKinds; ++k) {
    out += " " + toStr(c.resistances.resist[static_cast<u32>(k)]);
  }
  out += ", poison " + toStr(c.resistances.poisonResist) + "\n";

  out += "Pack:\n";
  for (i32 k = 0; k < kPackSlots; ++k) {
    const Item& it = inv.pack[static_cast<u32>(k)];
    // UNKNOWN: an item id of 0 is treated as an empty slot.
    if (it.id == 0) continue;
    Str itemName = "item " + toStr(it.id);
    for (const ItemDef& d : s.data.items) {
      if (d.id == it.id) { itemName = d.name; break; }
    }
    out += "  slot " + toStr(k + 1) + ": " + itemName + (it.equipped ? " (worn)" : "") + "\n";
  }
  out += "Worn items: " + toStr(static_cast<i64>(inv.worn.size())) + "\n";

  out += "Companions: alive " + toStr(p.aliveCompanions);
  if (p.carryingSlot == CarryingSlot::Companion && p.carryingCompanion >= 0) {
    out += ", carrying companion " + toStr(p.carryingCompanion);
  }
  // SIMPLIFIED: the full companion list is not reachable from the Session state given.
  out += "\n";
  return out;
}

Str describeSquare(const Session& s) {
  const Party& p = s.game.party;

  if (p.dungeonLevel == 0) {
    // Town menu: the TownSection order from city1.h.
    Str out = "Town. Places:\n";
    for (i32 k = 0; k < kViewTownSections; ++k) {
      out += "  " + toStr(k + 1) + ". " + townSectionName(static_cast<TownSection>(k)) + "\n";
    }
    return out;
  }

  Str out = "Level " + toStr(p.dungeonLevel) + ", x " + toStr(p.x) + ", y " + toStr(p.y) +
            ", facing " + facingName(p.facing) + "\n";
  const FloorState* f = viewFloor(s);
  if (f == nullptr) return out + "No map stored for this level.\n";
  if (viewSquare(*f, p.x, p.y) == nullptr) return out + "Square not on the map.\n";

  const i32 fi = static_cast<i32>(p.facing);
  out += "Edges:\n";
  for (i32 k = 0; k < 4; ++k) {
    const Facing ef = static_cast<Facing>((fi + k) % 4);
    const EdgeClass ec = classifyEdge(*f, p.x, p.y, ef);
    out += "  " + relName(k) + " (" + facingName(ef) + "): " + edgeName(ec) + "\n";
  }

  out += "Here:";
  const SquareState* sq = viewSquare(*f, p.x, p.y);
  bool any = false;
  u32 fl = 0;
  for (const FloorDef& d : s.data.floors) {
    if (d.levelNum != p.dungeonLevel || p.x < 1 || p.y < 1) continue;
    const size_t q = static_cast<size_t>(p.y - 1) * static_cast<size_t>(d.width) + static_cast<size_t>(p.x - 1);
    if (q < d.fields.size()) fl = static_cast<u32>(d.fields[q].flags);
  }
  auto has = [fl](int bit) { return ((fl >> bit) & 1u) != 0u; };
  if (has(FLAG_STAIRS_UP)) { out += " stairs up"; any = true; }
  if (has(FLAG_STAIRS_DOWN)) { out += " stairs down"; any = true; }
  if (has(FLAG_PIT)) { out += " pit"; any = true; }
  if (has(FLAG_CHUTE)) { out += " chute"; any = true; }
  if (has(FLAG_TELEPORTER)) { out += " teleporter"; any = true; }
  if (has(FLAG_WATER)) { out += " water"; any = true; }
  if (has(FLAG_QUICKSAND)) { out += " quicksand"; any = true; }
  if (has(FLAG_ANTIMAGIC)) { out += " antimagic field"; any = true; }
  if (has(FLAG_ROTATOR)) { out += " rotator"; any = true; }
  if (sq->hasChest) { out += " chest"; any = true; }
  if (!any) out += " nothing notable";
  out += "\n";
  return out;
}

Str renderAutomap(const Session& s, i32 radius) {
  const Party& p = s.game.party;
  const FloorState* f = viewFloor(s);
  if (f == nullptr) return "No map stored for this level.\n";

  const i32 r = radius < 0 ? 0 : (radius > kViewMaxRadius ? kViewMaxRadius : radius);
  const i32 side = 2 * r + 1;     // squares across
  const i32 cols = 2 * side + 1;  // character cells across: squares and the edges between them
  const i32 rows = cols;

  Str grid(static_cast<u32>(rows * (cols + 1)), ' ');
  for (i32 row = 0; row < rows; ++row) {
    grid[static_cast<u32>(row * (cols + 1) + cols)] = '\n';
  }
  auto put = [&grid, cols, rows](i32 col, i32 row, char ch) {
    if (col < 0 || row < 0 || col >= cols || row >= rows) return;
    grid[static_cast<u32>(row * (cols + 1) + col)] = ch;
  };
  auto get = [&grid, cols, rows](i32 col, i32 row) -> char {
    if (col < 0 || row < 0 || col >= cols || row >= rows) return ' ';
    return grid[static_cast<u32>(row * (cols + 1) + col)];
  };

  const i32 x0 = p.x - r;
  const i32 yTop = p.y + r;
  // Square (x,y) sits at cell (2*(x-x0)+1, 2*(yTop-y)+1); north is up (smaller row).
  auto cellCol = [x0](i32 x) { return 2 * (x - x0) + 1; };
  auto cellRow = [yTop](i32 y) { return 2 * (yTop - y) + 1; };

  for (i32 y = p.y - r; y <= p.y + r; ++y) {
    for (i32 x = x0; x <= p.x + r; ++x) {
      const SquareState* sq = viewSquare(*f, x, y);
      if (sq == nullptr) continue;
      const bool known = sq->explored;
      const i32 cc = cellCol(x);
      const i32 cr = cellRow(y);
      put(cc, cr, known ? '.' : ' ');
      if (!known) continue;  // edges of unknown squares are not shown
      for (i32 k = 0; k < 4; ++k) {
        const Facing ef = static_cast<Facing>(k);
        i32 dx = 0;
        i32 dy = 0;
        viewDelta(ef, dx, dy);
        const EdgeClass ec = classifyEdge(*f, x, y, ef);
        char ch = ' ';
        if (ec == EdgeClass::Wall || ec == EdgeClass::SecretDoor) ch = '#';
        else if (ec == EdgeClass::Door) ch = '+';
        if (ch != ' ') put(cc + dx, cr - dy, ch);  // open edges stay blank so they never overwrite walls
      }
    }
  }

  put(cellCol(p.x), cellRow(p.y), '@');
  // Facing arrow in the edge cell ahead of the party, only where it is blank.
  i32 fdx = 0;
  i32 fdy = 0;
  viewDelta(p.facing, fdx, fdy);
  const i32 ac = cellCol(p.x) + fdx;
  const i32 ar = cellRow(p.y) - fdy;
  if (get(ac, ar) == ' ') {
    char arrow = '^';
    switch (p.facing) {
      case Facing::N: arrow = '^'; break;
      case Facing::E: arrow = '>'; break;
      case Facing::S: arrow = 'v'; break;
      case Facing::W: arrow = '<'; break;
    }
    put(ac, ar, arrow);
  }

  Str out = "Level " + toStr(p.dungeonLevel) + " around x " + toStr(p.x) + ", y " +
            toStr(p.y) + ", facing " + facingName(p.facing) + "\n";
  out += grid;
  out += "@ party, . known open, # wall, + door, ^ > v < facing, blank unknown or open edge\n";
  return out;
}

Str describeFight(const Session& s) {
  const Encounter& e = s.game.encounter;
  const Party& p = s.game.party;
  Str out;
  for (i32 g = 0; g < kMaxGroups; ++g) {
    const MonsterGroup& mg = e.groups[static_cast<u32>(g)];
    if (mg.count <= 0 || (mg.front.ref < 0 && mg.livingCount <= 0)) continue;
    Str name = mg.knownAs.empty() ? mg.front.knownAs : mg.knownAs;
    if (name.empty() && mg.front.ref >= 0 && static_cast<size_t>(mg.front.ref) < s.data.monsters.size()) name = s.data.monsters[static_cast<size_t>(mg.front.ref)].name;
    if (name.empty()) name = "monster";
    out += "Group " + toStr(g) + ": " + toStr(mg.count) + " x " + name + " (" +
           groupStateName(mg.state) + ", " + toStr(mg.livingCount) + " living)";
    // UNKNOWN: aimedGroup is treated as a 0-based index into encounter.groups.
    for (i32 k = 0; k < p.size && k < kPartyMaxSize; ++k) {
      const CombatOrder& co = p.combatOrders[static_cast<u32>(k)];
      if (co.aimedGroup == g) {
        out += " <- aimed at by " + p.members[static_cast<u32>(k)].name;
      }
    }
    out += "\n";
  }
  if (out.empty()) return "No monsters in the encounter.\n";
  return out;
}

}  // namespace mordor
