// dungeon1a.cpp: SRD c15p1 (Chapter 15 Dungeon Exploration, part 1).
// Movement, walls and turning, arrival and automap, lost state, square effects,
// underfoot and perceive-ahead perception, encounter helpers, pits and chutes (pit part).
//
// ASSUMED NAMES. These are not visible in the inputs given to this ant. Verify them against
// game_data.h and the state headers when the module is integrated:
//   FloorDef::fields (Vec<FloorField>), FloorField::flags, RaceDef::size, and the FloorFlag
//   enumerators FLAG_WALL_EAST, FLAG_WALL_NORTH, FLAG_DOOR_EAST, FLAG_DOOR_NORTH,
//   FLAG_SECRET_DOOR_EAST, FLAG_SECRET_DOOR_NORTH, FLAG_ROCK, FLAG_FOG, FLAG_STUD,
//   FLAG_EXTINGUISHER, FLAG_ROTATOR, FLAG_ANTIMAGIC, FLAG_FACE_N, FLAG_FACE_E, FLAG_FACE_S,
//   FLAG_FACE_W, FLAG_TELEPORTER, FLAG_QUICKSAND, FLAG_PIT, FLAG_CHUTE, FLAG_STAIRS_UP,
//   FLAG_STAIRS_DOWN, FLAG_WATER. Each is assumed to be a bit mask.
//
// Two storage roles. FloorDef::fields holds the static cell flags (the truth). FloorState
// SquareState::bits holds the live, automap-side copy of wall, door and feature bits. Edge
// classification and perception read the live copy. Rock and movement checks read the static flags.
//
// Secret edges use one marker. A SECRET bit set in SquareState::bits means "unrecorded secret
// edge". The roll on entry replaces it with DOOR (found) or WALL (not found), so it is never rolled
// again.

#include "mordor/dungeon1a.h"

#include <cmath>
#include <cstddef>
#include <string>

namespace mordor {
namespace {

// ---- cell flag masks (assumed FloorFlag names, see header comment) ----
constexpr u32 kBitWallEast = 1u << static_cast<u32>(FLAG_WALL_EAST);
constexpr u32 kBitWallNorth = 1u << static_cast<u32>(FLAG_WALL_NORTH);
constexpr u32 kBitDoorEast = 1u << static_cast<u32>(FLAG_DOOR_EAST);
constexpr u32 kBitDoorNorth = 1u << static_cast<u32>(FLAG_DOOR_NORTH);
constexpr u32 kBitSecretEast = 1u << static_cast<u32>(FLAG_SECRET_DOOR_EAST);
constexpr u32 kBitSecretNorth = 1u << static_cast<u32>(FLAG_SECRET_DOOR_NORTH);
constexpr u32 kBitRock = 1u << static_cast<u32>(FLAG_ROCK);
constexpr u32 kBitFog = 1u << static_cast<u32>(FLAG_FOG);
constexpr u32 kBitStud = 1u << static_cast<u32>(FLAG_STUD);
constexpr u32 kBitExtinguisher = 1u << static_cast<u32>(FLAG_EXTINGUISHER);
constexpr u32 kBitRotator = 1u << static_cast<u32>(FLAG_ROTATOR);
constexpr u32 kBitAntiMagic = 1u << static_cast<u32>(FLAG_ANTIMAGIC);
// FIXED: FLAG_FACE_WEST is confirmed by the compiler's suggestion; the N, E and S names follow
// the same pattern (FLAG_FACE_NORTH, FLAG_FACE_EAST, FLAG_FACE_SOUTH) and are not verified.
constexpr u32 kBitFaceN = 1u << static_cast<u32>(FLAG_FACE_NORTH);
constexpr u32 kBitFaceE = 1u << static_cast<u32>(FLAG_FACE_EAST);
constexpr u32 kBitFaceS = 1u << static_cast<u32>(FLAG_FACE_SOUTH);
constexpr u32 kBitFaceW = 1u << static_cast<u32>(FLAG_FACE_WEST);
constexpr u32 kBitTeleporter = 1u << static_cast<u32>(FLAG_TELEPORTER);
constexpr u32 kBitQuicksand = 1u << static_cast<u32>(FLAG_QUICKSAND);
constexpr u32 kBitPit = 1u << static_cast<u32>(FLAG_PIT);
constexpr u32 kBitChute = 1u << static_cast<u32>(FLAG_CHUTE);
constexpr u32 kBitStairsUp = 1u << static_cast<u32>(FLAG_STAIRS_UP);
constexpr u32 kBitStairsDown = 1u << static_cast<u32>(FLAG_STAIRS_DOWN);
constexpr u32 kBitWater = 1u << static_cast<u32>(FLAG_WATER);

// R13 six edge bits written on entry (secret bits act as the unrecorded marker, see above)
constexpr u32 kEdgeBitsEntry = kBitWallEast | kBitWallNorth | kBitDoorEast | kBitDoorNorth |
                               kBitSecretEast | kBitSecretNorth;
// R13 seven features recorded on entry
constexpr u32 kFeatureBitsEntry = kBitPit | kBitStairsUp | kBitStairsDown | kBitWater |
                                  kBitQuicksand | kBitFog | kBitChute;
// R35 noticeable types
constexpr u32 kNoticeBits = kBitFaceN | kBitFaceE | kBitFaceS | kBitFaceW | kBitExtinguisher |
                            kBitRotator | kBitAntiMagic | kBitStud;
constexpr u32 kFaceBits = kBitFaceN | kBitFaceE | kBitFaceS | kBitFaceW;

// ---- small helpers ----
i32 mapHeight(const FloorDef& def) {
    // Height = total fields / this floor's own width (not the global kMapWidth).
    // Guard against a zero width so the function stays total.
    if (def.width <= 0) {
        return 0;
    }
    return static_cast<i32>(def.fields.size()) / def.width;
}

i32 stateHeight(const FloorState& floor) {
    return static_cast<i32>(floor.squares.size()) / kMapWidth;
}

bool inMap(i32 x, i32 y, i32 height) {
    return x >= 1 && x <= kMapWidth && y >= 1 && y <= height;
}

bool cellInState(const FloorState& floor, i32 x, i32 y) {
    return inMap(x, y, stateHeight(floor));
}

i32 cellOf(i32 x, i32 y) {
    return mapSquareIndex(x, y) - 1;
}

SquareState& sqAt(FloorState& floor, i32 x, i32 y) {
    return floor.squares[static_cast<std::size_t>(cellOf(x, y))];
}

const SquareState& sqAt(const FloorState& floor, i32 x, i32 y) {
    return floor.squares[static_cast<std::size_t>(cellOf(x, y))];
}

// Edge storage: the edge's cell, and its wall, door and secret bits (R1, R17, R18).
// N edge is (x,y) North bit. E edge is (x,y) East bit. S edge is (x,y-1) North bit. W edge is (x-1,y) East bit.
struct EdgeRef {
    i32 cx;
    i32 cy;
    u32 wall;
    u32 door;
    u32 secret;
};

EdgeRef edgeRefOf(i32 x, i32 y, Facing f) {
    switch (f) {
        case Facing::N: return {x, y, kBitWallNorth, kBitDoorNorth, kBitSecretNorth};
        case Facing::E: return {x, y, kBitWallEast, kBitDoorEast, kBitSecretEast};
        case Facing::S: return {x, y - 1, kBitWallNorth, kBitDoorNorth, kBitSecretNorth};
        case Facing::W: return {x - 1, y, kBitWallEast, kBitDoorEast, kBitSecretEast};
    }
    return {x, y, 0, 0, 0};
}

// Leader's ability-5 value for the entry secret roll (R16).
// UNKNOWN: BestAbilityValue(c, 5, gear) is not available in this module. Approximated as stat + statMod.
i32 senseAbilityValue(const Party& party) {
    const Character& leader = party.leader();
    return leader.abilities.stats[kDungeonSenseAbility] +
           leader.abilities.statMods[kDungeonSenseAbility];
}

// R29/R33 uses the race size (RaceData.Size, R53 raceSize).
// UNKNOWN: RaceDef::size field name assumed.
i32 raceSizeOf(const Ctx& ctx, const Character& c) {
    i32 idx = static_cast<i32>(c.race);
    if (idx < 0 || idx >= static_cast<i32>(ctx.data.races.size())) return 0;
    return ctx.data.races[static_cast<std::size_t>(idx)].size;
}

}  // namespace

// ============================================================
// Facing and edge classification (R1, R2, R5, R7)
// ============================================================

Facing turnRightOf(Facing f) {
    return static_cast<Facing>((static_cast<i32>(f) + 1) & 3);  // R1 TurnRight (f+1)&3
}

Facing turnLeftOf(Facing f) {
    return static_cast<Facing>((static_cast<i32>(f) + 3) & 3);  // R1 TurnLeft (f+3)&3
}

Facing turnAroundOf(Facing f) {
    return static_cast<Facing>((static_cast<i32>(f) + 2) & 3);  // R2 Opposite (f+2)&3
}

void facingDelta(Facing f, i32& dx, i32& dy) {
    switch (f) {  // R1 Delta: N(0,+1) E(+1,0) S(0,-1) W(-1,0)
        case Facing::N: dx = 0; dy = 1; break;
        case Facing::E: dx = 1; dy = 0; break;
        case Facing::S: dx = 0; dy = -1; break;
        case Facing::W: dx = -1; dy = 0; break;
    }
}

void applyMoveKey(Ctx& ctx, Party& party, FloorState& floor, const FloorDef& def, MoveKey key, bool shift) {
    // R3: paralysis stops both the turn and the step.
    if (party.members[0].ailments.paralysed) return;
    switch (key) {
        case MoveKey::Up:  // R2 Up steps forward
            tryMoveForward(ctx, party, floor, def);
            return;
        case MoveKey::Down:  // R2 Down turns around
            party.facing = turnAroundOf(party.facing);
            break;
        case MoveKey::Left:  // R2 Left turns left
            party.facing = turnLeftOf(party.facing);
            break;
        case MoveKey::Right:  // R2 Right turns right
            party.facing = turnRightOf(party.facing);
            break;
    }
    // R3: shift with a turn is followed by one forward step (fn1521, False).
    if (shift) tryMoveForward(ctx, party, floor, def);
    // UNKNOWN: AfterMove square effects (R33) are run by the caller through runSquareEffects,
    // because they need the leader's ability-5 chance.
}

bool tryMoveForward(Ctx& ctx, Party& party, FloorState& floor, const FloorDef& def) {
    i32 x = party.x;
    i32 y = party.y;
    Facing f = party.facing;
    i32 dx = 0;
    i32 dy = 0;
    facingDelta(f, dx, dy);
    i32 tx = x + dx;
    i32 ty = y + dy;

    // R7: a step off the map is clamped with OUCH and records nothing.
    if (stepLeavesMap(def, x, y, f)) {
        reportWallHit(ctx);
        return false;
    }
    // R5 / R1: the target edge is classified. A Wall refuses the move.
    EdgeClass edge = classifyEdge(floor, x, y, f);
    if (edge == EdgeClass::Wall) {
        // R8 / R18: a wall bump in fog records the wall on the bumped edge.
        if (squareIsFogged(floor, x, y)) recordFogWallBump(ctx, floor, x, y, f);
        reportWallHit(ctx);  // R6
        return false;
    }
    // R5: target rock refuses the move.
    if (!canMoveFacing(floor, def, x, y, f)) {
        reportWallHit(ctx);  // R6
        return false;
    }
    // R20: lost handling on arrival. Runs before the new square is marked, so the mapped check
    // sees the pre-step automap state.
    arrivalLostCheck(ctx, party, floor, tx, ty);
    // R17: passing through a door records the door bit and clears the wall bit.
    if (edge == EdgeClass::Door || edge == EdgeClass::SecretDoor) {
        recordDoorPassage(ctx, floor, x, y, f);
    }
    // R10: placement sets position with no rock check. Marks explored if not rock (R15 gated).
    setPartyPosition(ctx, party, floor, tx, ty);
    // R11-R15: entry recording. A fresh guard is used because Party has no guard field.
    MapRecordGuard guard{};
    recordArrival(ctx, party, floor, def, guard, tx, ty);
    // R16: secret doors roll on entry.
    rollSecretDoorsOnEntry(ctx, floor, tx, ty, senseAbilityValue(party));
    // R4: the party shares the leader's lost flags.
    shareLeaderLostFlags(ctx, party);
    return true;
}

void reportWallHit(Ctx& ctx) {
    ctx.say("move.ouch", "", {}, {Str("OUCH")});  // R6: plays OUCH with no text
}

void shareLeaderLostFlags(Ctx& ctx, Party& party) {
    (void)ctx;
    // R4: the party shares the leader's lost flags.
    for (i32 i = 1; i < party.size; ++i) {
        party.members[i].lost = party.members[0].lost;
    }
    // Party-level mirror flags follow the leader.
    const LostState& L = party.members[0].lost;
    party.lostLevel = L.level;
    party.lostPosition = L.position;
    party.lostFacing = L.facing;
    party.lost = party.members[0].isLost();
}

bool squareIsRock(const FloorState& floor, i32 x, i32 y) {
    // R1/R10 rock flag.
    // UNKNOWN: the rock flag is read from the live SquareState::bits copy here. canMoveFacing
    // reads the static FloorDef flags instead.
    return cellInState(floor, x, y) && (sqAt(floor, x, y).bits & kBitRock) != 0;
}

bool squareIsFogged(const FloorState& floor, i32 x, i32 y) {
    // R11 fog read from the square.
    return cellInState(floor, x, y) && (sqAt(floor, x, y).bits & kBitFog) != 0;
}

EdgeClass classifyEdge(const FloorState& floor, i32 x, i32 y, Facing f) {
    // R1 Classify: wall -> Wall; else door -> Door; else secret -> SecretDoor; else Open.
    EdgeRef e = edgeRefOf(x, y, f);
    if (!cellInState(floor, e.cx, e.cy)) return EdgeClass::Wall;  // R7: off the map counts as a wall
    u32 b = sqAt(floor, e.cx, e.cy).bits;
    if (b & e.wall) return EdgeClass::Wall;
    if (b & e.door) return EdgeClass::Door;
    if (b & e.secret) return EdgeClass::SecretDoor;
    return EdgeClass::Open;
}

bool canMoveFacing(const FloorState& floor, const FloorDef& def, i32 x, i32 y, Facing f) {
    // R5 CanMove: edge != Wall and not rock at target.
    if (stepLeavesMap(def, x, y, f)) return false;  // R7
    if (classifyEdge(floor, x, y, f) == EdgeClass::Wall) return false;
    i32 dx = 0;
    i32 dy = 0;
    facingDelta(f, dx, dy);
    if (fieldFlagsAt(def, mapSquareIndex(x + dx, y + dy)) & kBitRock) return false;
    return true;
}

bool stepLeavesMap(const FloorDef& def, i32 x, i32 y, Facing f) {
    // R7 step off the map.
    i32 dx = 0;
    i32 dy = 0;
    facingDelta(f, dx, dy);
    return !inMap(x + dx, y + dy, mapHeight(def));
}

i32 mapSquareIndex(i32 x, i32 y) {
    // R19 Square = X + 30 * (Y - 1), 1-based.
    return x + kMapWidth * (y - 1);
}

u32 fieldFlagsAt(const FloorDef& def, i32 square) {
    // R19 Walls(square) = (1 <= square <= fields.Length) ? fields[square-1].Flags : 0
    if (square < 1 || square > static_cast<i32>(def.fields.size())) return 0;
    return static_cast<u32>(def.fields[static_cast<std::size_t>(square - 1)].flags);
}

// ============================================================
// Arrival and the automap (R9-R16, R22-R24)
// ============================================================

bool mappingEnabled(const Party& party) {
    // R9 nav.Mapping = !leader.IsLost
    return !party.leader().isLost();
}

void markExplored(Ctx& ctx, const Party& party, FloorState& floor, i32 x, i32 y) {
    (void)ctx;
    // R15: writes only if Mapping is on.
    if (!mappingEnabled(party)) return;
    if (!cellInState(floor, x, y)) return;
    sqAt(floor, x, y).explored = true;
}

void setPartyPosition(Ctx& ctx, Party& party, FloorState& floor, i32 x, i32 y) {
    // R10 SetPosition: X=x; Y=y; mark explored if not rock. No rock check on placement.
    party.x = x;
    party.y = y;
    for (i32 i = 0; i < party.size; ++i) {
        party.members[i].location.x = x;
        party.members[i].location.y = y;
        party.members[i].location.dungeonLevel = party.dungeonLevel;
    }
    if (!squareIsRock(floor, x, y)) markExplored(ctx, party, floor, x, y);
}

void mapRecordOnEntry(Ctx& ctx, FloorState& floor, const FloorDef& def, i32 x, i32 y) {
    (void)ctx;
    // R11-R13 MapRecord.OnEntry. Fog is read once, from the stepped-onto square.
    i32 h = mapHeight(def);
    if (!inMap(x, y, h) || !cellInState(floor, x, y)) return;
    u32 own = fieldFlagsAt(def, mapSquareIndex(x, y));
    SquareState& here = sqAt(floor, x, y);
    here.bits |= (own & kFeatureBitsEntry);  // R13 seven features (fog included)
    if (own & kBitFog) return;               // R11/R12 in fog: no edges recorded
    here.bits |= (own & kEdgeBitsEntry);     // R12/R13 own East and North edges
    // R12 far edges (x = Width-1, y = Height-1) are recorded as seen.
    // UNKNOWN: the spec's "Width-1" is taken as the last column, kMapWidth (1-based x).
    if (x == kMapWidth) here.bits |= kBitWallEast;
    if (y == h) here.bits |= kBitWallNorth;
    // R12 west neighbour contributes only its East wall or door.
    if (x > 1) {
        u32 w = fieldFlagsAt(def, mapSquareIndex(x - 1, y));
        sqAt(floor, x - 1, y).bits |= (w & (kBitWallEast | kBitDoorEast));
    }
    // R12 south neighbour contributes only its North wall or door.
    if (y > 1) {
        u32 s = fieldFlagsAt(def, mapSquareIndex(x, y - 1));
        sqAt(floor, x, y - 1).bits |= (s & (kBitWallNorth | kBitDoorNorth));
    }
}

void recordArrival(Ctx& ctx, const Party& party, FloorState& floor, const FloorDef& def,
                   MapRecordGuard& guard, i32 x, i32 y) {
    // R15 guard: here = (level, x, y); if here == lastRecorded return.
    i32 level = party.dungeonLevel;
    if (guard.valid && guard.level == level && guard.x == x && guard.y == y) return;
    guard.level = level;
    guard.x = x;
    guard.y = y;
    guard.valid = true;
    // R9: the map record is written only when neither LostPosition nor LostLevel is set.
    if (!mappingEnabled(party)) return;
    mapRecordOnEntry(ctx, floor, def, x, y);
}

void rollSecretDoorsOnEntry(Ctx& ctx, FloorState& floor, i32 x, i32 y, i32 leaderAbility5Value) {
    // R16: for each unrecorded secret edge on the squares touched, one roll is made.
    // Touched edges: own N, own E, west neighbour's E, south neighbour's N.
    const EdgeRef edges[4] = {
        edgeRefOf(x, y, Facing::N),
        edgeRefOf(x, y, Facing::E),
        edgeRefOf(x, y, Facing::W),
        edgeRefOf(x, y, Facing::S),
    };
    for (const EdgeRef& e : edges) {
        if (!cellInState(floor, e.cx, e.cy)) continue;
        SquareState& s = sqAt(floor, e.cx, e.cy);
        if (!(s.bits & e.secret)) continue;  // recorded secret doors are never rolled again
        // NoticesSecretDoor: rng.Next(100) < BestAbilityValue(c, 5, gear) / 5 (integer division assumed)
        i32 chance = leaderAbility5Value / kSecretDoorDivisor;
        bool found = ctx.rng.below(100) < chance;
        s.bits &= ~e.secret;
        if (found) {
            s.bits |= e.door;     // success records the secret door
            s.bits &= ~e.wall;    // and clears the wall
        } else {
            s.bits |= e.wall;     // failure records the wall
        }
    }
}

void recordDoorPassage(Ctx& ctx, FloorState& floor, i32 x, i32 y, Facing f) {
    (void)ctx;
    // R17: if secret: Record(secretBit) else if door: Record(doorBit); then Unrecord(wallBit).
    // UNKNOWN: an unrecorded secret edge that is passed through is recorded as a door, clearing
    // the secret marker. The SRD does not say how a secret marker is represented.
    EdgeRef e = edgeRefOf(x, y, f);
    if (!cellInState(floor, e.cx, e.cy)) return;
    SquareState& s = sqAt(floor, e.cx, e.cy);
    if (s.bits & e.secret) {
        s.bits &= ~e.secret;
        s.bits |= e.door;
    } else if (s.bits & e.door) {
        s.bits |= e.door;  // Record(doorBit)
    }
    s.bits &= ~e.wall;  // Unrecord(wallBit)
}

void recordFogWallBump(Ctx& ctx, FloorState& floor, i32 x, i32 y, Facing f) {
    (void)ctx;
    // R8/R18: N:(x,y) WALL_NORTH; E:(x,y) WALL_EAST; S:(x,y-1) WALL_NORTH; W:(x-1,y) WALL_EAST
    EdgeRef e = edgeRefOf(x, y, f);
    if (!cellInState(floor, e.cx, e.cy)) return;
    sqAt(floor, e.cx, e.cy).bits |= e.wall;
}

void arrivalLostCheck(Ctx& ctx, Party& party, const FloorState& floor, i32 x, i32 y) {
    // R20 arrival order. (a) standing on mapped ground while lost: FoundYourself.
    // (b) still lost: TryToWorkItOut. Walking onto mapped ground ends the lost state outright (quirk).
    Character& leader = party.members[0];
    if (!leader.isLost()) return;
    bool mapped = cellInState(floor, x, y) && sqAt(floor, x, y).explored;
    if (mapped) foundYourself(ctx, leader.lost);
    if (leader.isLost()) tryToWorkItOut(ctx, leader.lost, leader.perception);
    shareLeaderLostFlags(ctx, party);
}

// ============================================================
// Lost state (R25-R32)
// ============================================================

void becomeLost(Ctx& ctx, LostState& lost) {
    (void)ctx;
    // R25/R26 BecomeLost sets all three flags.
    lost.level = true;
    lost.position = true;
    lost.facing = true;
}

void foundYourself(Ctx& ctx, LostState& lost) {
    // R25 FoundYourself clears all three; the message shows only if the character was lost.
    bool wasLost = lost.level || lost.position || lost.facing;
    if (wasLost) ctx.say("lost.found", "You have found yourself.", {}, {});
    lost.level = false;
    lost.position = false;
    lost.facing = false;
}

void tryToWorkItOut(Ctx& ctx, LostState& lost, i32 perception) {
    // R28 working out position, each step, in this order.
    // Divisors are the literal R28/R30 values (15, 20, 25).
    double weight = workingOutWeight(perception);
    // (a) floor check, only if LostLevel is set. weight/15 > Rnd*100 (comparison direction UNKNOWN; ">" used).
    if (lost.level && weight / 15.0 > ctx.rng.nextDouble() * 100.0) {
        lost.level = false;
        ctx.say("lost.level_clear", "You work out which floor you are on.", {}, {});
    }
    // (b) facing check, only if LostFacing is set. weight/20 > Rnd*100.
    if (lost.facing && weight / 20.0 > ctx.rng.nextDouble() * 100.0) {
        lost.facing = false;
        ctx.say("lost.facing_clear", "You work out which way you are facing.", {}, {});
    }
    // (c) full position check, always rolled (R27). weight/25 > Rnd*100 -> FoundYourself.
    if (weight / 25.0 > ctx.rng.nextDouble() * 100.0) {
        foundYourself(ctx, lost);
    }
}

double workingOutWeight(i32 perception) {
    // R28: weight = p<=0 ? 0 : Math.Log(p)^2. Natural log assumed (UNKNOWN: log base not confirmed in source).
    if (perception <= 0) return 0.0;
    double l = std::log(static_cast<double>(perception));
    return l * l;
}

void applyRotatorLoss(Ctx& ctx, LostState& lost, i32 ability5Chance, i32 dungeonLevel) {
    // R29: saved if p >= Rnd*70 + DungeonLevel*2. Otherwise LostPosition and LostFacing are set.
    // The floor flag is not touched. Literals are taken from R29 directly.
    double threshold = ctx.rng.nextDouble() * 70.0 +
                       static_cast<double>(dungeonLevel) * 2.0;
    if (static_cast<double>(ability5Chance) >= threshold) return;  // saved
    lost.position = true;
    lost.facing = true;
    ctx.say("rotator.lost", "The rotator has turned you around.", {}, {});
}

void inheritLostOnJoin(Ctx& ctx, const Party& party, Character& newcomer) {
    (void)ctx;
    // R31 sub0848 True: copies lost-on-floor, in-position and in-facing from Chars(0).
    // Runs before dead joiners are refused (quirk: a refused newcomer keeps the flags).
    newcomer.lost = party.leader().lost;
}

void clearLostAtMorgue(Ctx& ctx, LostState& lost) {
    (void)ctx;
    // R32 a morgue visit clears all three lost flags. Silent.
    lost.level = false;
    lost.position = false;
    lost.facing = false;
}

// ============================================================
// Square effects (R33-R34)
// ============================================================

void runSquareEffects(Ctx& ctx, Party& party, FloorState& floor, const FloorDef& def, i32 ability5Chance) {
    // R33 effects run in AfterMove. The leader perceives each bit just before its effect.
    i32 x = party.x;
    i32 y = party.y;
    u32 flags = fieldFlagsAt(def, mapSquareIndex(x, y));
    i32 perception = party.leader().perception;

    // (a) Water or quicksand per-tick costs apply unless levitating.
    // UNKNOWN: the per-tick cost rules (R69) are not in this part. Not applied here.
    (void)flags;

    // (b) anti-magic and stud perception, (c) extinguisher perception, (d) face bits perception,
    // (e) rotator perception. All noticeable types are perceived in one pass (R35).
    perceiveUnderfoot(ctx, party, floor, x, y, perception, true);

    // (c) extinguisher: clears protections for each member.
    if (flags & kBitExtinguisher) extinguishProtections(ctx, party);

    // (d) facing bits: the party turns silently to face.
    if (flags & kFaceBits) faceSquareBits(ctx, party, floor, x, y);

    // (e) rotator: random facing (R94), then the lose-direction check (R29).
    if (flags & kBitRotator) {
        party.facing = rollRotatorFacing(ctx);
        applyRotatorLoss(ctx, party.members[0].lost, ability5Chance, party.dungeonLevel);
        shareLeaderLostFlags(ctx, party);
    }

    // (f) pit falls per member, then the party-list UPDATE (R54, R55).
    if (flags & kBitPit) {
        bool stopStep = resolvePitFall(ctx, party, party.dungeonLevel);
        pitUpdateParty(ctx, party);
        if (stopStep) return;
    }

    // (g) stairs caption. A pit caption on the same square is overwritten (quirk).
    if (flags & kBitStairsUp) ctx.say("stairs.up", "There are stairs up here.", {}, {});
    if (flags & kBitStairsDown) ctx.say("stairs.down", "There are stairs down here.", {}, {});

    // (h) teleporter, (i) chute, (j) destination re-run: handled by the chute and teleporter
    // modules (dungeon1b and dungeon2). Not handled in this unit.
}

void extinguishProtections(Ctx& ctx, Party& party) {
    // R33(c): clears all protection masks and all eleven protection values for each member.
    for (i32 i = 0; i < party.size; ++i) {
        Resistances& r = party.members[i].resistances;
        r.protectionMasks.clear();
        for (auto& v : r.protectionValues) v = 0;
    }
    ctx.say("extinguish", "Your protections are extinguished!", {}, {});
}

void faceSquareBits(Ctx& ctx, Party& party, FloorState& floor, i32 x, i32 y) {
    // R33(d), R45: the FACE bits are perceived and written to the map. The party turns silently.
    // UNKNOWN: the SRD does not say which face wins if several are set. The first in N,E,S,W order wins.
    if (!cellInState(floor, x, y)) return;
    u32 b = sqAt(floor, x, y).bits;
    if (b & kBitFaceN) {
        party.facing = Facing::N;
    } else if (b & kBitFaceE) {
        party.facing = Facing::E;
    } else if (b & kBitFaceS) {
        party.facing = Facing::S;
    } else if (b & kBitFaceW) {
        party.facing = Facing::W;
    }
    if (b & kFaceBits) markExplored(ctx, party, floor, x, y);
}


bool castBlockedBySquare(Ctx& ctx, FloorState& floor, i32 x, i32 y) {
    // R34: casting on an anti-magic square prints "Anti-Magic area!" and writes the square's bit.
    // CONFLICT: R34 also cites extinguish squares, per the modding doc. The preferred behaviour
    // is used here: anti-magic only.
    if (!cellInState(floor, x, y)) return false;
    SquareState& s = sqAt(floor, x, y);
    if (!(s.bits & kBitAntiMagic)) return false;
    s.bits |= kBitAntiMagic;  // writes the square's bit (sub087C, True)
    ctx.say("cast.antimagic", "Anti-Magic area!", {}, {});
    return true;
}

// ============================================================
// Perception underfoot (R35-R40)
// ============================================================

void perceiveUnderfoot(Ctx& ctx, const Party& party, FloorState& floor, i32 x, i32 y,
                       i32 perception, bool messagesOn) {
    if (!cellInState(floor, x, y)) return;
    bool leaderLost = party.leader().isLost();
    u32 bits = sqAt(floor, x, y).bits;

    // R35: a teleporter is always named and mapped, with no roll, unless the character is lost.
    // UNKNOWN: the SRD omits the teleporter from the noticeable list. This follows the
    // "always named" statement. R39: suppressed when messages are off.
    if ((bits & kBitTeleporter) && !leaderLost) {
        if (messagesOn) ctx.say("perceive.teleporter", "There is a teleporter here.", {}, {});
        markExplored(ctx, party, floor, x, y);
    }

    // R35: the noticeable types are checked.
    if (!(bits & kNoticeBits)) return;

    SquareNotice notice = rollSquarePerception(ctx, perception);  // R36
    // R36: identify maps the square unless the leader is lost.
    if (notice.identified && !leaderLost) markExplored(ctx, party, floor, x, y);
    bool mapped = sqAt(floor, x, y).explored;

    if (notice.noticed && notice.identified && messagesOn) {
        struct NameRow {
            u32 mask;
            const char* text;
        };
        const NameRow rows[] = {
            {kBitFaceN, "Something faces north here."},
            {kBitFaceE, "Something faces east here."},
            {kBitFaceS, "Something faces south here."},
            {kBitFaceW, "Something faces west here."},
            {kBitExtinguisher, "An extinguisher is here."},
            {kBitRotator, "A rotator is here."},
            {kBitAntiMagic, "An anti-magic area is here."},
            {kBitStud, "A stud is here."},
        };
        bool rotatorNamed = perception != 0 && perception != 100;  // R38 quirk
        for (const NameRow& row : rows) {
            if (!(bits & row.mask)) continue;
            if (row.mask == kBitRotator && !rotatorNamed) continue;
            ctx.say("perceive.feature", row.text, {}, {});
        }
    }

    if (notice.noticed && !notice.identified) {
        i32 guess = drawWrongGuessBit(ctx);  // R40 wrong guess
        if (messagesOn) {
            ctx.say("perceive.guess", "You think you sense something, but you are not sure what.",
                    {static_cast<i64>(guess)}, {});
        }
    }

    if (messagesOn) reportSquareCertainty(ctx, perception, mapped, leaderLost);  // R37
}

SquareNotice rollSquarePerception(Ctx& ctx, i32 perception) {
    SquareNotice out;
    // R36 Notice: rng.Next(100)+1 <= P
    out.noticed = (ctx.rng.below(100) + 1 <= perception);
    // R36 Identify (only if noticed): rng.Next(100)+1 <= P/2 (integer division)
    if (out.noticed) {
        out.identified = (ctx.rng.below(100) + 1 <= perception / 2);
    }
    return out;
}

void reportSquareCertainty(Ctx& ctx, i32 perception, bool squareMapped, bool leaderLost) {
    // R37 certainty wording. Uses (int) perception.
    if (perception <= kCertaintyLowLimit) {  // R37: at P <= 35 only this message
        ctx.say("perceive.vague", "You sense something different here...", {}, {});
        return;
    }
    if (leaderLost) {  // R37: named as absolutely certain, and not mapped
        ctx.say("perceive.absolute", "You are absolutely certain of what is here.", {}, {});
        return;
    }
    if (perception == 100 && squareMapped) return;  // R38 quirk: says nothing
    if (perception >= 200) {  // R38: passes both rolls
        ctx.say("perceive.complete", "You are completely certain of what is here.", {}, {});
        return;
    }
    if (squareMapped) {  // R37: a mapped square reads "completely"
        ctx.say("perceive.completely", "You are completely sure of this square.", {}, {});
    }
}

i32 drawWrongGuessBit(Ctx& ctx) {
    // R40: a wrong guess drawn from the 23 flag bits.
    // UNKNOWN: the exclusion count does not match the "eight bits always pass" comment. All 23 are used.
    return static_cast<i32>(ctx.rng.below(kWrongGuessBitCount));
}

// ============================================================
// Perceive ahead (R41-R46)
// ============================================================

bool perceiveAheadBlocked(const FloorState& floor, const FloorDef& def, i32 x, i32 y, Facing f) {
    // R41: walls, doors and secret doors on the shared edge, and the map edge, block the view.
    if (stepLeavesMap(def, x, y, f)) return true;
    return classifyEdge(floor, x, y, f) != EdgeClass::Open;
}

i32 capPerceiveAheadChance(i32 bestMemberAbility5Chance) {
    // R46: best = min(max over members, 95).
    return bestMemberAbility5Chance < kPerceiveAheadCap ? bestMemberAbility5Chance
                                                        : kPerceiveAheadCap;
}

i32 perceptionGearLookup(const Character& ch) {
    // R46 gear lookup: WIS + StatMods/4 (MAINMOD2 1962).
    // UNKNOWN: the StatMods index is taken as the WIS entry.
    u32 wis = static_cast<u32>(StatIndex::Wis);
    return ch.abilities.stats[wis] + ch.abilities.statMods[wis] / 4;
}

bool perceiveAheadHit(Ctx& ctx, i32 chance) {
    // R46: hit if rng.NextDouble()*100 < best.
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(chance);
}

void perceiveAheadFeatures(Ctx& ctx, FloorState& floor, const FloorDef& def, i32 aheadX, i32 aheadY,
                           i32 chance, bool standingInQuicksand) {
    // R42-R45. Each feature is checked against the ahead square's static flags. Speech is
    // gated on the chance. No automap check (R43).
    i32 h = mapHeight(def);
    if (!inMap(aheadX, aheadY, h) || !cellInState(floor, aheadX, aheadY)) return;
    u32 truth = fieldFlagsAt(def, mapSquareIndex(aheadX, aheadY));
    SquareState& s = sqAt(floor, aheadX, aheadY);

    // R45: FACE bits are perceived and written to the map (silent).
    if (truth & kFaceBits) s.explored = true;

    struct FeatureRow {
        u32 mask;
        const char* text;
    };
    // R42 AheadFeature order.
    const FeatureRow rows[] = {
        {kBitQuicksand, "Quicksand ahead."},
        {kBitStairsUp, "Stairs up ahead."},
        {kBitStairsDown, "Stairs down ahead."},
        {kBitTeleporter, "A teleporter ahead."},
        {kBitChute, "A chute ahead."},
        {kBitPit, "A pit ahead."},
        {kBitRotator, "A rotator ahead."},
        {kBitExtinguisher, "An extinguisher ahead."},
    };
    for (const FeatureRow& row : rows) {
        // RESOLVED-BY-REFERENCE: PerceiveAhead.Look checks fl.Has(ax, ay, bit) before the roll,
        // so an absent feature does not consume a roll; each present feature has its own
        // Rnd * 100 < perception roll.
        if (!(truth & row.mask)) continue;
        if (!perceiveAheadHit(ctx, chance)) continue;
        // Reference Look: map.Record(bit) happens only when the roll succeeds, before the speech test.
        s.bits |= row.mask;
        if (row.mask == kBitQuicksand) s.explored = true;  // R42 quicksand written to the map
        // R44: quicksand speech is suppressed only while standing in quicksand. The roll was
        // already taken above, and the bit is still mapped.
        if (row.mask == kBitQuicksand && standingInQuicksand) continue;
        ctx.say("ahead.feature", row.text, {}, {});
    }
}

// ============================================================
// Encounters (R47-R50)
// ============================================================

bool encountersAllowed(i32 areaId) {
    // R47: area id > 0.
    return areaId > 0;
}

bool standingGroupKept(i64 nowStamp, const Area& area) {
    // R48: kept while (now - stamp) > 2 * respawnT.
    return (nowStamp - area.stamp) > static_cast<i64>(kStandingGroupAgeFactor) * area.respawnT;
}

void refillRoomIfDue(Ctx& ctx, Area& area, i64 nowStamp, bool neverStamped) {
    (void)ctx;
    // R48: an empty room refills once its stamp is more than 1 * respawnT old, or if never stamped.
    bool due = neverStamped ||
               (nowStamp - area.stamp) > static_cast<i64>(kEmptyRoomAgeFactor) * area.respawnT;
    if (!due) return;
    // Refills are written back immediately.
    // UNKNOWN: the spawn population is not in this part. Only the stamp and emptiedAt fields are reset here.
    area.stamp = nowStamp;
    area.emptiedAt = 0;
}

EncounterSquareTraits encounterTraitsForSquare(bool rock, bool stud, bool fog) {
    // R49: ROCK suppresses encounters. STUD raises the spawn floor by one. FOG leaves the group unidentified.
    EncounterSquareTraits t;
    t.suppressed = rock;
    t.floorBonus = stud ? 1 : 0;
    t.unidentified = fog;
    return t;
}

i32 rollEncounterSpread(Ctx& ctx, i32 level, bool onStud) {
    // R50: roll = Int(Rnd*100) + 1.
    i32 roll = ctx.rng.below(kSpreadRollSides) + 1;
    // R50 / MODULE4.BAS:1481: reference is Round_half_even((roll < 50 ? 1 : 0) + level / 5.0),
    // with float division. level/5.0 has a fractional part of 0, .2, .4, .6 or .8, never .5,
    // so half-even rounding is plain rounding: depth = level/5 + (level%5 >= 3 ? 1 : 0).
    i32 depth = level / 5 + ((level % 5) >= 3 ? 1 : 0);
    // CONFLICT: the SRD ladder tests "<50" first, so the 2 and 3 branches never run. Reproduced as written.
    i32 spread = (roll < 50 ? 1 : 0) + depth;
    // c18: if onStud and Rnd*75 < 50: spread = 1. The c24 replacement is unspecified (UNKNOWN).
    if (onStud && ctx.rng.nextDouble() * kStudSpreadRange < kStudSpreadThreshold) {
        spread = 1;
    }
    // spread = min(spread, level)
    if (spread > level) spread = level;
    // encounterFloor = level + 1 if onStud else level. The caller applies it (R50).
    return spread;
}

// ============================================================
// Pits and chutes (R52-R55)
// ============================================================

bool rollPitHover(Ctx& ctx, i32 pitLevel) {
    // R52: hovers if 5 + level/4.0 < Rnd*100. Literals taken from R52 directly.
    double threshold = 5.0 + static_cast<double>(pitLevel) / 4.0;
    return threshold < ctx.rng.nextDouble() * 100.0;
}

i32 rollPitDamage(Ctx& ctx, i32 pitLevel, i32 raceSize) {
    // R53: dmg = Round_half_even( Int(Rnd*(((level+1)/2)*15.5)) * Ln(raceSize+2) + 1 ).
    i32 span = (pitLevel + 1) / 2;  // integer division (VB)
    double range = static_cast<double>(span) * kPitDamageUnit;
    i64 base = vbInt(ctx.rng.nextDouble() * range);  // Int(Rnd*range)
    double raw = static_cast<double>(base) *
                     std::log(static_cast<double>(raceSize + kPitDamageSizeOffset)) +
                 1.0;
    return static_cast<i32>(roundHalfEven(raw));
}

bool resolvePitFall(Ctx& ctx, Party& party, i32 pitLevel) {
    // R52-R54. Each member falls per the pit rules. Only levitating members roll to float.
    // Returns true when a lone character dies here, which stops the step.
    bool stopStep = false;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[i];
        if (c.ailments.levitating && rollPitHover(ctx, pitLevel)) {
            continue;  // hovers: no damage
        }
        i32 dmg = rollPitDamage(ctx, pitLevel, raceSizeOf(ctx, c));  // R53
        c.vitals.curHits -= dmg;  // hits may drop below zero
        if (i == 0) {
            // R54 character on screen: "You have fallen into a pit for N hits." with FALL and HIT.
            Str text = Str("You have fallen into a pit for ") + std::to_string(dmg) + Str(" hits.");
            ctx.say("pit.fall", text, {static_cast<i64>(dmg)}, {Str("FALL"), Str("HIT")});
        } else {
            // R54 other members get a Messages line with no sound.
            Str text = c.name + Str(" falls into the pit for ") + std::to_string(dmg) + Str(" hits.");
            ctx.say("pit.member", text, {static_cast<i64>(dmg)}, {});
        }
        if (party.size == 1 && c.vitals.curHits <= 0) stopStep = true;  // R54 lone death stops the step
    }
    return stopStep;
}

void pitUpdateParty(Ctx& ctx, Party& party) {
    // R55 party-list UPDATE. Members at zero or below hits are dropped.
    // A member not yet dead is first marked dead (counted, logged, DIE.WAV).
    i32 kept = 0;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[i];
        if (c.vitals.curHits > 0) {
            if (kept != i) party.members[kept] = party.members[i];
            ++kept;
            continue;
        }
        if (!c.isDead()) {
            c.deathState = DeathState::Dead;
            c.deaths += 1;
            ctx.say("pit.die", "A party member has died in the pit.", {}, {c.name, Str("DIE.WAV")});
        }
    }
    // Remaining members move to the leader's square.
    for (i32 i = 0; i < kept; ++i) {
        party.members[i].location.x = party.x;
        party.members[i].location.y = party.y;
        party.members[i].location.dungeonLevel = party.dungeonLevel;
    }
    party.size = kept;
    if (kept == 0) party.wiped = true;
}

}  // namespace mordor
