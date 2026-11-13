#pragma once
#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

// Module dungeon1a: SRD c15p1 (dungeon exploration, part 1).
// Edge bits, feature bits and lost flags live in the state structs:
//   SquareState.bits   = live FloorFlag word of the cell (walls, doors, secret doors, rock, fog, features)
//   SquareState.explored = automap mapped bit
// UNKNOWN: the copy of FloorDef field flags into SquareState.bits at level build is not in this part.
// Coordinates x, y are 1-based square coordinates; floor.squares is indexed by mapSquareIndex(x, y) - 1.

// ---- Constants ----
inline constexpr i32 kMapWidth = 30;                  // R19 Square = X + 30 * (Y - 1)
inline constexpr i32 kDungeonSenseAbility = 5;        // R16/R29/R46 ability 5 (SRD numbering)
inline constexpr i32 kLostLevelDivisor = 15;          // R28/R30 floor check: weight/15 > Rnd*100
inline constexpr i32 kLostFacingDivisor = 20;         // R28/R30 facing check: weight/20 > Rnd*100
inline constexpr i32 kLostBothDivisor = 25;           // R28/R30 full position check: weight/25 > Rnd*100
inline constexpr i32 kRotatorLossBase = 70;           // R29 threshold Rnd*70 + DungeonLevel*2
inline constexpr i32 kRotatorLossLevelFactor = 2;     // R29
inline constexpr i32 kSecretDoorDivisor = 5;          // R16 Next(100) < ability / 5
inline constexpr i32 kPerceiveAheadCap = 95;          // R46 cap on perceive-ahead chance
inline constexpr i32 kAheadFeatureCount = 8;          // R42 eight features roll separately
inline constexpr i32 kEntryFeatureCount = 7;          // R13 seven features recorded on entry
inline constexpr i32 kEntryEdgeBitCount = 6;          // R13 six edge bits written on entry
inline constexpr i32 kWrongGuessBitCount = 23;        // R40 wrong guess drawn from 23 flag bits
inline constexpr i32 kCertaintyLowLimit = 35;         // R37 at P <= 35 only "You sense something different here..."
inline constexpr i32 kIdentifyDivisor = 2;            // R36 identify chance is P / 2
inline constexpr i32 kNoticeRange = 100;              // R36 Next(100) + 1 <= P
inline constexpr i32 kStandingGroupAgeFactor = 2;     // R48 kept while age > 2 * respawn time
inline constexpr i32 kEmptyRoomAgeFactor = 1;         // R48 refill once age > 1 * respawn time
inline constexpr i32 kSpreadRollSides = 100;          // R50 roll = Int(Rnd*100) + 1
inline constexpr i32 kStudSpreadRange = 75;           // R50 Rnd*75 < 50 on stud
inline constexpr i32 kStudSpreadThreshold = 50;       // R50
inline constexpr i32 kPitHoverBase = 5;               // R52 hovers if 5 + level/4.0 < Rnd*100
inline constexpr double kPitHoverLevelDivisor = 4.0;  // R52
inline constexpr double kPitDamageUnit = 15.5;        // R53 Int(Rnd*(((level+1)/2)*15.5))
inline constexpr i32 kPitDamageSizeOffset = 2;        // R53 Ln(raceSize + 2)

// ---- Enums and small result structs ----

// R2 move keys (Up steps forward, Down turns around, Left/Right turn)
enum class MoveKey : i32 { Up = 0, Down, Left, Right };

// R1/R5 edge classes: wall bit, else door bit, else secret bit, else open
enum class EdgeClass : i32 { Wall = 0, Door, SecretDoor, Open };

// R42 perceive-ahead features in the SRD roll order
enum class AheadFeature : i32 { Quicksand = 0, StairsUp, StairsDown, Teleporter, Chute, Pit, Rotator, Extinguisher };

// R15 guard: last square recorded (level, x, y). Caller owns it (no static state).
struct MapRecordGuard {
    i32 level = 0;
    i32 x = 0;
    i32 y = 0;
    bool valid = false;
};

// R36 result of the notice and identify rolls for one square
struct SquareNotice {
    bool noticed = false;
    bool identified = false;
};

// R49 encounter traits of a square
struct EncounterSquareTraits {
    bool suppressed = false;    // ROCK suppresses encounters
    i32 floorBonus = 0;         // STUD raises spawn floor by one
    bool unidentified = false;  // FOG leaves the group unidentified
};

// ---- R1-R2 facing helpers (pure) ----
Facing turnRightOf(Facing f);                       // R1 TurnRight: (f+1)&3
Facing turnLeftOf(Facing f);                        // R1 TurnLeft: (f+3)&3
Facing turnAroundOf(Facing f);                      // R2 Opposite: (f+2)&3
void facingDelta(Facing f, i32& dx, i32& dy);       // R1 Delta: N(0,+1) E(+1,0) S(0,-1) W(-1,0)

// ---- R2-R3 movement keys ----
// R2 Up steps forward, Down turns 180, Left/Right turn. R3 shift adds one forward step after a turn.
// Paralysis (leader ailments.paralysed) stops both the turn and the step.
void applyMoveKey(Ctx& ctx, Party& party, FloorState& floor, const FloorDef& def, MoveKey key, bool shift);

// ---- R5-R8 forward step ----
// R5 refused if the edge is Wall or the target is rock. R7 off-map step counts as wall.
// R8 a wall bump in fog records the wall (see recordFogWallBump). Returns true if the party moved.
bool tryMoveForward(Ctx& ctx, Party& party, FloorState& floor, const FloorDef& def);

// ---- R6 wall hit feedback ----
void reportWallHit(Ctx& ctx);                       // R6 plays OUCH with no text

// ---- R4 leader lost flags ----
void shareLeaderLostFlags(Ctx& ctx, Party& party);  // R4 party shares the leader's lost flags

// ---- R1, R5, R7 edge and map queries (pure) ----
bool squareIsRock(const FloorState& floor, i32 x, i32 y);                 // R1/R10 rock flag
bool squareIsFogged(const FloorState& floor, i32 x, i32 y);               // R11 fog read from the square
EdgeClass classifyEdge(const FloorState& floor, i32 x, i32 y, Facing f);  // R1 Classify; S/W read neighbour N/E bit
bool canMoveFacing(const FloorState& floor, const FloorDef& def, i32 x, i32 y, Facing f); // R5 CanMove
bool stepLeavesMap(const FloorDef& def, i32 x, i32 y, Facing f);          // R7 step off the map

// ---- R19 map addressing (pure) ----
i32 mapSquareIndex(i32 x, i32 y);                                         // R19 X + 30 * (Y - 1), 1-based
u32 fieldFlagsAt(const FloorDef& def, i32 square);                        // R19 Walls(square); 0 if out of range

// ---- R9, R15 automap write switch and marking ----
bool mappingEnabled(const Party& party);                                  // R9 nav.Mapping = !leader.IsLost
void markExplored(Ctx& ctx, const Party& party, FloorState& floor, i32 x, i32 y); // R15 writes only if Mapping on

// ---- R10 placement ----
void setPartyPosition(Ctx& ctx, Party& party, FloorState& floor, i32 x, i32 y);  // R10 sets position, no rock check; marks explored if not rock
// R10 StepThrough (Ethereal Portal) is owned by the spells portal code (PortalOutcome); not declared here.

// ---- R11-R13, R21-R22 entry recording ----
// R11 runs only if the square is not fogged (fog read once from that square). R12 writes the square's
// own east/north walls and doors, far edges, west neighbour's east edge, south neighbour's north edge.
// R13 records features pit, stairs up, stairs down, water, quicksand, fog, chute (EntryFeatureCount)
// and writes six edge bits. R14 teleporter, rotator, anti-magic, rock, stud and extinguisher are not
// recorded on entry. CONFLICT: teleporter recording (see perceiveUnderfoot). R21/R22 fog removes walls and
// doors but not the square; the mapped bit is set regardless of fog.
void mapRecordOnEntry(Ctx& ctx, FloorState& floor, const FloorDef& def, i32 x, i32 y);

// ---- R15 arrival guard ----
// Records a square once per (level, x, y): RecordArrival, ArriveBySpell, RerunSquare, PushToAutomap.
void recordArrival(Ctx& ctx, const Party& party, FloorState& floor, const FloorDef& def,
                   MapRecordGuard& guard, i32 x, i32 y);

// ---- R16 secret doors ----
// For each unrecorded secret edge on the squares touched, one roll. Success records the secret door and
// clears the wall; failure records the wall. A recorded secret door is never rolled again.
// leaderAbility5Value = BestAbilityValue(leader, 5, gear), computed by the caller (attributes module).
// UNKNOWN: integer vs real division of ability/5 (kSecretDoorDivisor) is not confirmed in the source.
void rollSecretDoorsOnEntry(Ctx& ctx, FloorState& floor, i32 x, i32 y, i32 leaderAbility5Value);

// ---- R17 passing through a door ----
void recordDoorPassage(Ctx& ctx, FloorState& floor, i32 x, i32 y, Facing f); // R17 record door bit, clear wall bit

// ---- R8, R18, R23 fog wall bump ----
void recordFogWallBump(Ctx& ctx, FloorState& floor, i32 x, i32 y, Facing f); // R8/R18 N:(x,y) WALL_NORTH, E:(x,y) WALL_EAST, S:(x,y-1) WALL_NORTH, W:(x-1,y) WALL_EAST

// ---- R20 arrival lost handling ----
// Order across sources is UNKNOWN in this part. Implemented as: (a) FoundYourself if standing on mapped
// ground while lost, then (b) tryToWorkItOut while still lost. Leader perception is used.
void arrivalLostCheck(Ctx& ctx, Party& party, const FloorState& floor, i32 x, i32 y);

void foundYourself(Ctx& ctx, LostState& lost);              // R25 clears all three; message only if was lost
// R27/R28 working out position: position roll runs even when only floor or facing is lost.
// Steps: (a) floor check if LostLevel, (b) facing check if LostFacing, (c) full check (FoundYourself).
// Each success clears its own flag. UNKNOWN: direction of the comparison ('>' used by the port).
void tryToWorkItOut(Ctx& ctx, LostState& lost, i32 perception);
double workingOutWeight(i32 perception);                    // R28 weight = p<=0 ? 0 : Log(p)^2 (Log presumed natural, UNKNOWN base)


















// R51 waiting groups and forced round: UNKNOWN, not covered in this part.


}  // namespace mordor
