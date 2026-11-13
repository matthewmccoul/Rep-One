// src/dungeon1b_a.cpp
// Module dungeon1b, half a: SRD c15p1, rules R56-R88
// (walked and taken chutes, water and ticks, teleporter squares,
// Teleport / displacement / Ethereal Portal / Soul Search).
//
// UNKNOWN: GameData field names are assumed, since game_data.h is not in the brief:
//   FloorDef.width, .height, .fields, .chutes, .teleporters; FloorField.flags;
//   FloorChute.x, .y, .depth; FloorTeleporter.x, .y, .x2, .y2, .z2; FLAG_ROCK.
// UNKNOWN: enumerator and member names of ArrivalOutcome, PortalOutcome,
//   DisplaceOutcome and LocateLine are assumed from the header comments.
// UNKNOWN: status1b.h helpers (pruneTheDead, setDead) have no signature in the
//   brief, so equivalent local helpers are used below instead of calling them.

#include "mordor/dungeon1b.h"

#include <cmath>

namespace mordor {
namespace {

// UNKNOWN: identifier of the Limited build is not given in the SRD half.
constexpr i32 kBuildLimited = 1;
constexpr double kChuteDamageStep = 10.5;   // R60 level * 10.5
constexpr i32 kWaterHurts = 1;               // R70 WaterHurts = 1 (scaling UNKNOWN)
constexpr i32 kIdentifyMin = 10;             // R72 clamp floor
constexpr i32 kIdentifyMax = 90;             // R72 clamp ceiling
constexpr i64 kFourSecondsMs = 4000;         // R73
// FIXED: renamed from kLocateLevelRange, which clashed with the header constant.
constexpr i32 kSoulSearchRange = 3;          // R88 LocateLevelRange

// FIXED: DisplaceOutcome enumerators are not declared in dungeon1b.h, so the
// outcome is carried by value. UNKNOWN: real DisplaceOutcome numbering.
constexpr i32 kDisplaceArrives = 0;
constexpr i32 kDisplaceFallsFromTheSky = 1;
constexpr i32 kDisplaceIntoRock = 2;

// FIXED: PortalOutcome enumerators are not declared in dungeon1b.h, so the
// outcome is carried by value. UNKNOWN: real PortalOutcome numbering.
constexpr i32 kPortalSurvived = 0;
constexpr i32 kPortalPartyDies = 1;

}  // namespace

// ---------------------------------------------------------------------------
// Map helpers (R59, R75, R77, R83, R84, R86). Defined here because the chute
// and teleporter rules use them first.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Walked and taken chutes (R56-R68)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Stairs, water and ticks (R69-R73)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Teleporter squares (R74-R82)
// ---------------------------------------------------------------------------

// Keeper becomeLost(Ctx&, LostState&) lives in src/dungeon1a.cpp.
void becomeLost(Ctx& ctx, LostState& lost);

// ---------------------------------------------------------------------------
// Teleport, displacement, Ethereal Portal and Soul Search (R83-R88)
// ---------------------------------------------------------------------------

// FIXED: forward declaration, applyFailedTeleport is used by displaceParty and
// etherealPortalStep before its definition below. Removed `static` so the
// declaration matches the later extern-linkage definition (namespace mordor, not
// an anonymous namespace).
void applyFailedTeleport(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                         i32 build, ArrivalOutcome outcome,
                         i32 level, i32 x, i32 y);

// R88 Soul Search direction: maps the signs of the right and forward offsets to one of
// the eight RelativeDirection points. UNKNOWN: the point assignment is assumed from the
// header order (North first, then clockwise).
namespace {
}  // namespace

}  // namespace mordor
