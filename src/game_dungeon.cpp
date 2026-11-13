// game_dungeon.cpp: dungeon command layer (glue only; rules live in the rules modules).
// UNKNOWN: party.x / party.y are assumed 1-based, matching R19 (square = X + 30*(Y-1)).
// UNKNOWN: GameData::floors is assumed to be indexed by dungeonLevel-1.
// UNKNOWN: GameData::monsters is assumed to be a Vec with size().
// UNKNOWN: Str is assumed to be std::string-like (compare, substr, ==, assignment).
#include "mordor/game.h"
#include "mordor/features.h"
#include "mordor/monsters3a.h"
#include "mordor/monsters2a.h"
#include "mordor/clock.h"
#include "mordor/casting1.h"
#include "mordor/dungeon1a.h"
#include "mordor/dungeon1b.h"
#include "mordor/dungeon2a.h"
#include "mordor/dungeon3.h"
#include "mordor/loot1a.h"
#include "mordor/loot2.h"
#include "mordor/monsters1a.h"
#include "mordor/partyb.h"

namespace mordor {
namespace {

// R19: map width used for the flat square array.
constexpr i32 kMapWidth = 30;
// Length of the per-level area table (Arr<Area, 201> in FloorState).
constexpr i32 kAreaSlots = 201;
// One dungeon step or turn-with-step costs 1000 ms of game time.
constexpr i64 kMoveMs = 1000;
// SIMPLIFIED: the SRD ch.11 R7 encounter chance is not in the supplied context.
// The fixed percentage below stands in for it until the real rule is wired.
constexpr i32 kEncounterPercent = 10;

enum class DungeonCmd : i32 {
    None = 0,
    Forward,
    Back,
    TurnLeft,
    TurnRight,
    StairsUp,
    StairsDown,
    Open,
    Search,
    Pool,
    Map
};

DungeonCmd parseDungeonCmd(const Str& cmd) {
    if (cmd == "up" || cmd == "forward" || cmd == "w") return DungeonCmd::Forward;
    if (cmd == "down" || cmd == "back" || cmd == "s") return DungeonCmd::Back;
    if (cmd == "left" || cmd == "a") return DungeonCmd::TurnLeft;
    if (cmd == "right" || cmd == "d") return DungeonCmd::TurnRight;
    if (cmd == "stairs up" || cmd == "up stairs") return DungeonCmd::StairsUp;
    if (cmd == "stairs down" || cmd == "down stairs") return DungeonCmd::StairsDown;
    if (cmd == "open") return DungeonCmd::Open;
    if (cmd == "search") return DungeonCmd::Search;
    if (cmd == "pool") return DungeonCmd::Pool;
    if (cmd == "map") return DungeonCmd::Map;
    return DungeonCmd::None;
}

FloorState* floorAt(Session& s, i32 level) {
    for (FloorState& f : s.game.floors) {
        if (f.dungeonLevel == level) return &f;
    }
    return nullptr;
}

bool defIndexOk(const Session& s, i32 level) {
    return level >= 1 && static_cast<size_t>(level) <= s.data.floors.size();
}

// R19: 1-based (x, y) to the flat square array.
SquareState* squareAt(FloorState& floor, i32 x, i32 y) {
    if (x < 1 || y < 1 || x > kMapWidth) return nullptr;
    const i32 idx = (x - 1) + kMapWidth * (y - 1);
    const i32 n = static_cast<i32>(floor.squares.size());
    if (idx < 0 || idx >= n) return nullptr;
    return &floor.squares[static_cast<u32>(idx)];
}

void syncPosition(Session& s) {
    const Party& p = s.game.party;
    Location& l = s.game.partyPosition;
    l.x = p.x;
    l.y = p.y;
    l.dungeonLevel = p.dungeonLevel;
    l.facing = static_cast<i32>(p.facing);
    s.game.partyFacing = static_cast<i32>(p.facing);
    s.game.dungeonLevel = p.dungeonLevel;
    if (p.dungeonLevel > s.town.deepestReached) s.town.deepestReached = p.dungeonLevel;   // the deepest level reached (restock input)
}

// Encounter roll for the square just entered (SRD ch.11 R7, ch.15).
// Returns true if monsters were met.
// Static helpers for generateEncounterFor. Only truncation and nearest-even
// rounding are used on non-negative values, so no <cmath> include is needed.

// R28: hits roll on load. Sign is drawn first, then the swing.
static i32 rollLoadHits(Ctx& ctx, i32 baseHits) {
    const i32 sign = ctx.rng.below(2);
    const double swingRange = static_cast<double>(baseHits) * 0.5 / 2.0;
    const i32 swing = static_cast<i32>(ctx.rng.nextDouble() * swingRange) + 1;
    return (sign == 0) ? baseHits - swing : baseHits + swing;
}

// Round half to even for a non-negative value (SRD CInt).
static i32 roundHalfEvenNonNeg(double x) {
    const i32 whole = static_cast<i32>(x);
    const double frac = x - static_cast<double>(whole);
    if (frac > 0.5) return whole + 1;
    if (frac < 0.5) return whole;
    return (whole % 2 == 0) ? whole : whole + 1;
}

// Builds a Creature from its monster table row. ref is the table index used
// for name lookup (s.data.monsters[ref].name).
static Creature makeMonsterCreature(const MonsterDef& d, i32 monIndex,
                                    const Location& loc, i32 areaId) {
    Creature c{};
    c.ref = monIndex;
    c.id = d.id;
    c.monsterClass = d.type;   // UNKNOWN: which MonsterDef field is the class (type vs kind)
    c.att = d.att;
    c.levelFound = d.levelFound;
    c.guildLevel = d.guildLevel;
    c.floor = loc.dungeonLevel;
    c.homeArea = areaId;
    c.alive = true;
    c.hostile = true;
    c.unrolled = true;
    return c;
}

// File-scope include, needed at the top of src/game_dungeon.cpp:
// #include "mordor/monsters1a.h"

// R8-R20: encounter generation composed from the monster module API.
// UNKNOWN: API INDEX lists these names without return types or full signatures.
//   Assumed: encounterPrecheck -> EncounterPrecheck (pass value assumed to be enumerator 0),
//   rollLair -> LairRoll (struct; its index member is assumed to be `monsterIndex`: -1 no lair roll,
//   -2 lair roll passed with no monster of that Id), pickEncounterClass -> i32,
//   encounterTargetFloor -> i32, pickMonsterInClass -> MonsterPick (struct; its index member
//   is assumed to be `monsterIndex`, -1 none), spreadExhausted -> bool, widenSpread -> i32,
//   rollGroupCount/rollGroupSize -> i32, rollMonsterHits -> RolledHits (struct; its hit count
//   member is assumed to be `hits`), unrolledMemberHits -> i32, rollLeadEngaged -> bool.
// UNKNOWN: the member names of LairRoll and MonsterPick are not in the supplied index
//   (monsters3a.h header not shown); `monsterIndex` is a guess and must be verified.
// UNKNOWN: s.game.party is assumed to be the Party (GameState field name not shown).
// UNKNOWN: squareCode encoding is assumed to be the FloorDef field flags word (carries FLAG_ROCK and FLAG_STUD).
static const FloorDef* findFloorDefByLevel(const GameData& data, i32 dungeonLevel) {
    for (u32 i = 0; i < static_cast<u32>(data.floors.size()); ++i) {
        if (data.floors[i].levelNum == dungeonLevel) return &data.floors[i];
    }
    return nullptr;
}

bool generateEncounterFor(Session& s, Ctx& ctx, FloorState& floor,
                          SquareState& square, const Location& loc) {
    // Area slot gate. Area 0 and rock are refused by encounterPrecheck (R8).
    if (square.areaId < 0 || square.areaId >= kAreaSlots) return false;
    if (square.rockRecorded) return false;

    // Square coding from the floor definition. Party.x/y and Location x/y are 1-based.
    const FloorDef* fdef = findFloorDefByLevel(ctx.data, loc.dungeonLevel);
    if (fdef == nullptr || fdef->width <= 0) return false;
    const i32 squareIndex = (loc.y - 1) * fdef->width + (loc.x - 1);
    if (squareIndex < 0 || squareIndex >= static_cast<i32>(fdef->fields.size())) return false;
    const i32 squareCode = fdef->fields[static_cast<u32>(squareIndex)].flags;
    const bool stud = (static_cast<u32>(squareCode) & (1u << FLAG_STUD)) != 0;

    bool anyFoes = false;
    for (i32 f = 0; f < 4; ++f) {
        if (square.foes[f].count > 0) anyFoes = true;
    }

    Area& area = floor.areas[static_cast<u32>(square.areaId)];

    // R8: refuse rock, area 0, and squares with a stored encounter.
    // FIXED: encounterPrecheck returns EncounterPrecheck, not bool; compared against its pass value
    // (enumerator 0 assumed, since the name Ok does not exist in this enum).
    if (encounterPrecheck(squareCode, square.areaId, square.hasEncounter || anyFoes,
                          area.spawnMask) != EncounterPrecheck::Roll) {
        return false;
    }

    // Viewer is the leading member: top guild level, or level if none.
    if (s.game.party.size < 1) return false;
    const Character& leader = s.game.party.members[0];
    const i32 viewerLevel = leader.topGuildLevel > 0 ? leader.topGuildLevel : leader.level;
    // FIXED: rollLeadEngaged takes Align, so keep the viewer's alignment as Align.
    const Align viewerAlign = leader.alignment;

    // R12: target floor from the viewer level, +1 on a Stud square.
    const i32 targetFloor = encounterTargetFloor(viewerLevel, stud);

    // R9: lair roll (75% when the area has a LairId). Lead is the monster with that Id.
    // FIXED: rollLair returns LairRoll (a struct); its index is converted to the lead table index.
    // FIXED: member name `monsterIndex` is unverified (LairRoll definition not in the supplied index).
    const LairRoll lair = rollLair(ctx, area);
    if (lair.outcome == LairOutcome::Failed) return false;  // lair passed but no monster has that Id: the encounter fails (R9)
    i32 leadIdx = (lair.outcome == LairOutcome::Lead) ? lair.leadIndex : -1;
    i32 spread = 0;                   // UNKNOWN: initial spread not stated; 0 assumed.

    if (leadIdx < 0) {
        // R10/R11: class pick, then R13/R14: monster within class, widening spread.
        auto ranges = buildClassRanges(ctx);
        const i32 cls = pickEncounterClass(ctx, area, viewerLevel, square.areaId, ranges);
        if (cls < 0) return false;
        const auto& range = ranges[static_cast<u32>(cls)];

        i32 found = -1;
        // Bounded so that a function that never reports exhaustion cannot loop forever.
        for (i32 attempt = 0; attempt < 64; ++attempt) {
            // FIXED: pickMonsterInClass returns MonsterPick (a struct); its index is taken as the monster index.
            // FIXED: member name `monsterIndex` is unverified (MonsterPick definition not in the supplied index).
            found = static_cast<i32>(pickMonsterInClass(ctx, range, targetFloor, spread).monsterIndex);
            if (found >= 0) break;
            if (spreadExhausted(spread)) return false;  // R13: encounter restarts, here it fails
            spread = widenSpread(spread);
        }
        if (found < 0) return false;
        leadIdx = found;
    }

    if (leadIdx >= static_cast<i32>(ctx.data.monsters.size())) return false;
    const MonsterDef& monster = ctx.data.monsters[static_cast<u32>(leadIdx)];

    // R16: group count from the lead's NumGroups, clamped to 1..MaxGroups.
    i32 groups = rollGroupCount(ctx, monster.numGroups);
    if (groups < 1) groups = 1;
    if (groups > kMaxGroups) groups = kMaxGroups;

    // Nothing in the square is touched before this point.
    Encounter& enc = s.game.encounter;
    enc = Encounter{};
    enc.spread = spread;
    enc.targetFloor = targetFloor;
    i32 total = 0;

    for (i32 g = 0; g < groups; ++g) {
        // R17: group size from the lead's InGroup.
        i32 size = rollGroupSize(ctx, monster.inGroup);
        if (size < 1) size = 1;

        MonsterGroup& mg = enc.groups[static_cast<u32>(g)];

        // R28: the front's hits are rolled on load.
        Creature front = makeMonsterCreature(monster, leadIdx, loc, square.areaId);
        front.ref = leadIdx;
        // FIXED: rollMonsterHits returns RolledHits (a struct); its hit count is converted to the hit count.
        front.hits = static_cast<i32>(rollMonsterHits(ctx, monster.hits).hits);
        front.maxHits = front.hits;
        front.atNought = front.hits <= 0;
        front.unrolled = false;

        // R23: members behind the front are unrolled.
        Creature behind = makeMonsterCreature(monster, leadIdx, loc, square.areaId);
        behind.ref = leadIdx;
        behind.hits = unrolledMemberHits(monster.hits);
        behind.maxHits = behind.hits;
        behind.unrolled = true;

        mg.front = front;
        mg.count = size;
        mg.livingCount = size;

        for (i32 k = 0; k < size; ++k) {
            enc.monsters.push_back(k == 0 ? front : behind);
            ++total;
        }
    }

    // R18: group hostility from the lead's AlignmentValue and the viewer's alignment.
    const bool engaged = rollLeadEngaged(ctx, monster.alignmentValue, viewerAlign);
    for (i32 g = 0; g < groups; ++g) {
        enc.groups[static_cast<u32>(g)].state =
            engaged ? GroupState::Engaged : GroupState::Peaced;
    }

    square.hasEncounter = true;
    square.encounterKind = engaged ? EncounterKind::Hostile : EncounterKind::Peaced;
    // R24: a peaced group stays on the square and rolls for engagement each tick.
    square.encounterWaiting = !engaged;

    // Chest record from the encounter.
    // UNKNOWN: fight gold (ch.14) and MonsterGroupGold bands are not built here.
{
        // Fight gold (SRD ch.14 R4): per group base gold, low-gold floor, weight by group size, then by standing.
        Vec<MonsterGroupGold> gg;
        Currency fg = 0;
        i32 standing = 0;
        for (i32 gi = 0; gi < groups; ++gi) {
            MonsterGroupGold one;
            one.goldFactor = monster.goldFactor;
            one.levelFound = monster.levelFound;
            one.groupCount = enc.groups[static_cast<u32>(gi)].count;
            gg.push_back(one);
            Currency base = applyLowGroupGoldFloor(ctx, rollBaseGroupGold(ctx, one));
            fg += weightGroupGold(base, one.groupCount);
            standing += one.groupCount;
        }
        s.game.party.fightGold = scaleGoldByStanding(fg, standing);
        generateEncounterChest(ctx, square, leadIdx, gg, s.game.itemDropLevel, loc.dungeonLevel);
    }

    // R26: only an engaged group starts combat now.
    if (engaged) {
        s.game.inCombat = true;
        startFight(ctx, s.game);
    }

    ctx.say("dungeon.encounter",
            engaged ? "Monsters! You are attacked." : "Monsters are nearby.",
            {total, groups}, {});
    return true;
}

// Stairs: delta +1 descends, -1 climbs. Level 0 is the town.
// Finds the static floor definition whose levelNum matches the requested level.
// Helpers are defined here, before changeLevel, and dungeonAction (later in the
// same file) uses floorDefByLevel.
static const FloorDef* floorDefByLevel(const GameData& data, i32 level) {
    for (const FloorDef& f : data.floors) {
        if (f.levelNum == level) return &f;
    }
    return nullptr;
}

// Square index is (y-1)*width + (x-1) with 1-based Party.x/y.
static bool squareHasFlag(const FloorDef& def, i32 x, i32 y, FloorFlag flag) {
    if (def.width <= 0 || x < 1 || y < 1 || x > def.width) return false;
    const i32 idx = (y - 1) * def.width + (x - 1);
    if (idx < 0 || idx >= static_cast<i32>(def.fields.size())) return false;
    const u32 mask = 1u << static_cast<u32>(flag);
    return (static_cast<u32>(def.fields[static_cast<u32>(idx)].flags) & mask) != 0u;
}

// Returns the 1-based coordinates of the first square carrying the flag.
static bool findFlagSquare(const FloorDef& def, FloorFlag flag, i32& outX, i32& outY) {
    if (def.width <= 0) return false;
    const i32 count = static_cast<i32>(def.fields.size());
    const u32 mask = 1u << static_cast<u32>(flag);
    for (i32 idx = 0; idx < count; ++idx) {
        if ((static_cast<u32>(def.fields[static_cast<u32>(idx)].flags) & mask) != 0u) {
            outX = idx % def.width + 1;
            outY = idx / def.width + 1;
            return true;
        }
    }
    return false;
}

bool changeLevel(Session& s, Ctx& ctx, i32 delta, Str& reply) {
    Party& party = s.game.party;

    const FloorDef* curDef = floorDefByLevel(s.data, party.dungeonLevel);
    FloorState* cur = floorAt(s, party.dungeonLevel);
    if (curDef == nullptr || cur == nullptr) {
        reply = "There is no such level.";
        return true;
    }

    // The square's FloorDef flags decide whether stairs are here (R124).
    // delta is only the requested direction; it must agree with the square.
    const bool onUp = squareHasFlag(*curDef, party.x, party.y, FLAG_STAIRS_UP);
    const bool onDown = squareHasFlag(*curDef, party.x, party.y, FLAG_STAIRS_DOWN);
    if (!onUp && !onDown) {
        reply = "There are no stairs here.";
        return true;
    }
    if (delta < 0 && !onUp) {
        reply = "There are no stairs up here.";
        return true;
    }
    if (delta > 0 && !onDown) {
        reply = "There are no stairs down here.";
        return true;
    }

    const i32 target = (delta < 0) ? party.dungeonLevel - 1 : party.dungeonLevel + 1;

    if (target == 0) {
        // R124: stairs up from level 1 exit to town alive.
        climbToTownAlive(ctx, party);
        s.game.underground = false;
        party.onStairs = false;
        party.inDungeon = false;
        party.dungeonLevel = 0;
        s.game.inDungeon = false;
        s.game.inCity = true;
        s.game.dungeonLevel = 0;
        // R124: the leader's position is placed at the party's square.
        for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
            Location& loc = party.members[static_cast<u32>(i)].location;
            loc.x = party.x;
            loc.y = party.y;
            loc.dungeonLevel = 0;
            loc.area = -1;
        }
        advanceTime(s, kMoveMs);
        reply = "You climb the stairs and reach the town alive.";
        return true;
    }

    if (target > s.game.levelLimit) {
        reply = "The stairs go no deeper.";
        return true;
    }

    const FloorDef* destDef = floorDefByLevel(s.data, target);
    FloorState* dest = floorAt(s, target);
    if (destDef == nullptr || dest == nullptr) {
        // SIMPLIFIED: floor generation is not wired here; the level must exist in data.
        reply = "That level has not been reached yet.";
        return true;
    }

    // Climbing arrives on the level's stairs-down square; descending arrives
    // on its stairs-up square.
    // UNKNOWN: if several squares carry the flag, the first in field order is used.
    const FloorFlag arriveFlag = (delta < 0) ? FLAG_STAIRS_DOWN : FLAG_STAIRS_UP;
    i32 arriveX = 0;
    i32 arriveY = 0;
    if (!findFlagSquare(*destDef, arriveFlag, arriveX, arriveY)) {
        // UNKNOWN: the destination level has no matching staircase in its data.
        reply = "The stairs lead nowhere.";
        return true;
    }

    leaveArea(ctx, party, s.game.floors, s.data);
    party.dungeonLevel = target;
    party.x = arriveX;
    party.y = arriveY;
    party.onStairs = false;
    for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
        Location& loc = party.members[static_cast<u32>(i)].location;
        loc.x = arriveX;
        loc.y = arriveY;
        loc.dungeonLevel = target;
    }
    markExplored(ctx, party, *dest, arriveX, arriveY);
    syncPosition(s);
    advanceTime(s, kMoveMs);
    reply = (delta > 0) ? "You descend the stairs." : "You climb the stairs.";
    return true;
}

}  // namespace

// Plain-text helpers for the chest reply (no I/O, no locale).
static Str intStr(i64 v) {
    if (v == 0) return Str("0");
    if (v < 0) return Str("-") + intStr(-v);
    Str digits;
    while (v > 0) {
        digits.insert(digits.begin(), static_cast<char>('0' + (v % 10)));
        v /= 10;
    }
    return digits;
}

// Currency is in 1/10000 gold.
static Str goldText(i64 units) {
    const i64 whole = units / 10000;
    const i64 frac = units % 10000;
    Str out = intStr(whole);
    if (frac != 0) {
        out += ".";
        i64 d = 1000;
        while (d >= 1) {
            out += static_cast<char>('0' + ((frac / d) % 10));
            d /= 10;
        }
    }
    return out + " gold";
}

static const char* chestTrapName(ChestTrap t) {
    switch (t) {
        case ChestTrap::PoisonGas: return "poison gas";
        case ChestTrap::DiseaseSpores: return "disease spores";
        case ChestTrap::Explosion: return "explosion";
        case ChestTrap::Fire: return "fire";
        case ChestTrap::ItemDestruction: return "slime";
        case ChestTrap::Fate: return "fate";
        case ChestTrap::Trap7: return "teleporter";
        case ChestTrap::Blackout: return "blackout";
        case ChestTrap::Trap9: return "fear";
        case ChestTrap::Trap10: return "withering";
        default: return "unknown";
    }
}

// Items in every pack (used to work out how many items the trap destroyed).
static i32 chestPackCount(const Party& party) {
    i32 n = 0;
    for (i32 i = 0; i < party.size; ++i) {
        for (const Item& it : party.inventories[static_cast<size_t>(i)].pack) {
            if (it.id > 0) ++n;
        }
    }
    return n;
}

bool dungeonAction(Session& s, const Str& command0, Str& reply) {
    reply = Str();
    { bool fok = false; if (runFeatureCommand(s, Screen::Dungeon, command0, reply, fok)) return fok; }   // commands added by features
    // "stairs" alone means whichever stairs are on this square (up first).
    Str command = command0;
    if (command0 == "stairs") {
        const Party& pp = s.game.party;
        for (const FloorDef& d : s.data.floors) {
            if (d.levelNum != pp.dungeonLevel || pp.x < 1 || pp.y < 1) continue;
            const size_t q = static_cast<size_t>(pp.y - 1) * static_cast<size_t>(d.width) + static_cast<size_t>(pp.x - 1);
            if (q < d.fields.size()) command = (((static_cast<u32>(d.fields[q].flags) >> FLAG_STAIRS_UP) & 1u) != 0u) ? "stairs up" : "stairs down";
        }
    }
    reply = "";

    bool shift = false;
    Str cmd = command;
    if (cmd.compare(0, 6, "shift ") == 0) {
        shift = true;
        cmd = cmd.substr(6);
    }

    const DungeonCmd c = parseDungeonCmd(cmd);
    if (c == DungeonCmd::None) return false;

    if (!s.game.inDungeon) {
        reply = "You are not in the dungeon.";
        return true;
    }
    if (s.game.inCombat) {
        reply = "You cannot do that in combat.";
        return true;
    }

    Ctx ctx = s.ctx();
    Party& party = s.game.party;

    // Stairs are resolved from the square's FloorDef flags inside changeLevel (R124).
    // Handled before the floor lookup so the stairs path is not blocked by it.
    if (c == DungeonCmd::StairsUp) return changeLevel(s, ctx, -1, reply);
    if (c == DungeonCmd::StairsDown) return changeLevel(s, ctx, 1, reply);

    FloorState* floor = floorAt(s, party.dungeonLevel);
    const FloorDef* defPtr = floorDefByLevel(s.data, party.dungeonLevel);
    if (floor == nullptr || defPtr == nullptr || !defIndexOk(s, party.dungeonLevel)) {
        reply = "There is no such level.";
        return true;
    }

    switch (c) {
        case DungeonCmd::Forward:
        case DungeonCmd::Back:
        case DungeonCmd::TurnLeft:
        case DungeonCmd::TurnRight: {
            const FloorDef& def = *defPtr;
            const i32 px = party.x;
            const i32 py = party.y;
            bool moved = false;

            if (c == DungeonCmd::Forward) {
                // R5/R6: a refused forward step is a wall bump.
                moved = tryMoveForward(ctx, party, *floor, def);
                if (!moved) {
                    SquareState* here = squareAt(*floor, party.x, party.y);
                    if (here != nullptr) mapSquareOnAutomap(ctx, *here, false, true);
                    reply = "Ouch! A wall blocks the way.";
                    return true;
                }
            } else {
                // R2/R3: Down turns around; Left and Right turn. Shift adds a step.
                MoveKey key = MoveKey::Left;
                if (c == DungeonCmd::Back) key = MoveKey::Down;
                if (c == DungeonCmd::TurnRight) key = MoveKey::Right;
                applyMoveKey(ctx, party, *floor, def, key, shift);
                moved = (party.x != px || party.y != py);
                if (!moved) {
                    reply = "You turn.";
                    syncPosition(s);
                    return true;
                }
            }

            // Arrival: square effects (pits, chutes, teleporters, stairs, facing).
            Location loc;
            loc.x = party.x;
            loc.y = party.y;
            loc.dungeonLevel = party.dungeonLevel;
            loc.facing = static_cast<i32>(party.facing);
            applySquareEffects(ctx, party.members, party.size, *floor, loc);
            party.x = loc.x;
            party.y = loc.y;
            party.dungeonLevel = loc.dungeonLevel;
            party.facing = static_cast<Facing>(loc.facing & 3);

            // Effects may have moved the party to another level.
            floor = floorAt(s, party.dungeonLevel);
            defPtr = floorDefByLevel(s.data, party.dungeonLevel);
            if (floor == nullptr || defPtr == nullptr || !defIndexOk(s, party.dungeonLevel)) {
                syncPosition(s);
                reply = "The floor gives way beneath you.";
                advanceTime(s, kMoveMs);
                return true;
            }

            SquareState* here = squareAt(*floor, party.x, party.y);
            if (here == nullptr) {
                syncPosition(s);
                reply = "You are off the map.";
                advanceTime(s, kMoveMs);
                return true;
            }

            markExplored(ctx, party, *floor, party.x, party.y);
            mapSquareOnAutomap(ctx, *here, false, false);
            syncPosition(s);

            advanceTime(s, kMoveMs);

            const bool met = generateEncounterFor(s, ctx, *floor, *here, loc);
            if (met) {
                reply = "You step into the square and meet monsters.";
            } else {
                reply = "You move.";
            }
            return true;
        }

        case DungeonCmd::Open: {
            SquareState* here = squareAt(*floor, party.x, party.y);
            if (here == nullptr || !here->hasChest) {
                reply = "There is no chest here.";
                return true;
            }

            // Snapshot everything the reply needs before the chest is opened.
            const ChestTrap trapKind = here->chestTrap;
            const i32 openerIdx = party.opener;
            const bool openerOk = (openerIdx >= 0 && openerIdx < party.size);
            const i32 x0 = party.x;
            const i32 y0 = party.y;
            const i32 l0 = party.dungeonLevel;
            const i32 itemsBefore = static_cast<i32>(here->chestItems.size());
            const i32 packBefore = chestPackCount(party);
            Arr<i64, kPartyMaxSize> goldBefore = {};
            Arr<i64, kPartyMaxSize> hitsBefore = {};
            Arr<bool, kPartyMaxSize> deadBefore = {};
            for (i32 i = 0; i < party.size; ++i) {
                const size_t m = static_cast<size_t>(i);
                goldBefore[m] = party.members[m].wallet.goldOnHand;
                hitsBefore[m] = party.members[m].vitals.curHits;
                deadBefore[m] = party.members[m].isDead();
            }

            // openChest: trap first, then gold and items (see loot2.cpp for the codes).
            const i32 result = openChest(ctx, party, openerIdx, *here);
            if (result == 1) {
                reply = "You cannot open the chest.";
                return true;
            }
            if (result == 0) {
                reply = "You work at the chest's lock. It is not open yet.";
                return true;
            }

            const i32 itemsLeft = static_cast<i32>(here->chestItems.size());  // did not fit
            const i32 lost = (result == 2) ? itemsLeft : itemsBefore;
            const i32 placed = (result == 2) ? (itemsBefore - itemsLeft) : 0;
            i32 destroyed = packBefore + placed - chestPackCount(party);
            if (destroyed < 0) destroyed = 0;

            i64 hitsLost = 0;
            i32 newDeaths = 0;
            for (i32 i = 0; i < party.size; ++i) {
                const size_t m = static_cast<size_t>(i);
                const i64 hitsNow = party.members[m].vitals.curHits;
                if (hitsNow < hitsBefore[m]) hitsLost += hitsBefore[m] - hitsNow;
                if (!deadBefore[m] && party.members[m].isDead()) ++newDeaths;
            }
            const bool moved = (party.x != x0 || party.y != y0 || party.dungeonLevel != l0);

            Str text = "You open the chest.";
            if (trapKind != ChestTrap::NotTrapped) {
                text += " Trap! The ";
                text += chestTrapName(trapKind);
                text += " trap springs.";
                if (hitsLost > 0) text += " The party loses " + intStr(hitsLost) + " hit(s) in all.";
                if (newDeaths > 0) text += " " + intStr(newDeaths) + " member(s) die.";
                if (moved) text += " The trap moves the party elsewhere on the level.";
                if (destroyed > 0) text += " " + intStr(destroyed) + " item(s) destroyed.";
            }

            Currency share = 0;
            if (result == 2) {
                // Every living member gets the same share; report the first living member's gain.
                for (i32 i = 0; i < party.size; ++i) {
                    const size_t m = static_cast<size_t>(i);
                    if (!party.members[m].isDead()) {
                        share = party.members[m].wallet.goldOnHand - goldBefore[m];
                        break;
                    }
                }
                text += " Gold: each living member receives " + goldText(share) + ".";
                if (placed > 0) text += " " + intStr(placed) + " item(s) go into the party's packs.";
                if (itemsLeft > 0) text += " " + intStr(itemsLeft) + " item(s) did not fit and are lost.";
            } else {
                // result 3: no payout.
                if (openerOk && party.members[static_cast<size_t>(openerIdx)].isDead()) {
                    text += " The opener did not survive, so nothing is paid out and the contents are lost.";
                } else if (moved) {
                    text += " The party has been moved, so the chest's gold and items are left behind.";
                } else {
                    text += " No one is left standing to take the gold or items.";
                }
                if (lost > 0) text += " " + intStr(lost) + " item(s) lost.";
            }

            ctx.say("dungeon.chest", "The chest was opened.", {placed, lost, share}, {});
            reply = text;
            return true;
        }

        case DungeonCmd::Search: {
            SquareState* here = squareAt(*floor, party.x, party.y);
            if (here == nullptr) {
                reply = "There is nothing to search.";
                return true;
            }
            perceiveSquare(ctx, *here);
            reply = "You search the square.";
            return true;
        }

        case DungeonCmd::Pool: {
            const bool ok = poolGoldToSelected(ctx, party);
            reply = ok ? "Gold pooled." : "Gold could not be pooled.";
            return true;
        }

        case DungeonCmd::Map:
            // The front end draws the map with renderAutomap.
            reply = "Map shown by the front end.";
            return true;

        case DungeonCmd::StairsUp:
        case DungeonCmd::StairsDown:
            // Handled above before the floor lookup.
            return false;

        case DungeonCmd::None:
        default:
            return false;
    }
}

}  // namespace mordor
