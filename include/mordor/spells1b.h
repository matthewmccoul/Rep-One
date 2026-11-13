#pragma once
// spells1b: SRD part c10p1 (R56-R64).
// Ethereal Portal, Locate, Fetch, lock-opening, Binding, Fate, Sanctuary, utility effects 11-13.
// NOTE: state_character.h, state_common.h and state_companion.h have no include guards.
// Include them only through state_party.h, and forward-declare SquareState and FloorState.
#include "mordor/ctx.h"
#include "mordor/state_party.h"

namespace mordor {

struct SquareState;  // defined in state_dungeon.h
struct FloorState;   // defined in state_dungeon.h

// ---- constants ----
// R56 slots 0 and 1 kill the whole party; slot >= this value kills the caster and all after
inline constexpr i32 kPortalPartyWipeSlotLimit = 2;
// R57/R58 area 0 is the city
inline constexpr i32 kCityArea = 0;
// R57 UNKNOWN: LocateLevelRange value is not in this chunk; 15 chosen as a placeholder
inline constexpr i32 kLocateLevelRange = 15;
// R62 Fate unused draw is rng.below(3); the result is never read
inline constexpr i32 kFateUnusedDraw = 3;
// R60 OpensLockUpTo(effect) = effect - kLockOpenBaseEffect (effects 30..32 give levels 1..3)
inline constexpr i32 kLockOpenBaseEffect = 29;
// R63 days aged on sanctuary return (earlier port used 28)
inline constexpr i32 kSanctuaryDays = 365;

// ---- enums and small result structs ----
enum class PortalOutcome : i32 {
    PartyRock = 0,          // slot 0 or 1: whole party dies of rock
    CasterAndLaterRock = 1  // slot >= 2: caster and all after them die of rock
};

enum class LocateKind : i32 {
    TooFar = 0,     // outside kLocateLevelRange
    InCity = 1,     // target is in the city
    Direction = 2   // target is in range; a relative direction is given
};

// UNKNOWN: the SRD says "a relative direction is given" without naming the set; 8 points assumed
enum class RelativeDirection : i32 {
    North = 0,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest
};

struct LocateLine {
    i32 rosterIndex = -1;
    LocateKind kind = LocateKind::TooFar;
    RelativeDirection direction = RelativeDirection::North;
};

enum class FetchOutcome : i32 {
    Cancelled = 0,      // R58 cancel: nothing spent, nothing moved
    NotWithinReach = 1, // R59 "is not within reach"
    Retrieved = 2       // R59 body moved to caster and saved
};

enum class OpenLockOutcome : i32 {
    NoLockedChest = 0,  // R60 rule 1: nothing spent
    Unlocked = 1,       // R60 rule 2: lock level at or below opening level
    StillLocked = 2     // R60 rule 2: lock level above opening level
};

enum class BindTargetOutcome : i32 {
    NoBoundCompanions = 0,  // R61 Interact.NoBoundCompanions, nothing spent
    Bound = 1               // R61 points spent and bind applied
};

// R63 stored sanctuary location; x == 0 and y == 0 means never cast
struct SanctuaryPoint {
    i32 x = 0;
    i32 y = 0;
    i32 dungeonLevel = 0;
    bool isNever() const { return x == 0 && y == 0; }
};

enum class SanctuaryOutcome : i32 {
    NeverCast = 0,  // R63 no stored point
    Returned = 1    // R63 return performed, caster aged kSanctuaryDays
};

// R64 utility effect ids
enum class UtilityEffect : i32 {
    DetectRock = 11,
    DepthPerception = 12,
    FindDirection = 13
};





// ---- R60 Lock-opening ----
// R60 OpensLockUpTo(effect) = effect - 29 (trivial arithmetic)
inline constexpr i32 opensLockUpTo(i32 effect) { return effect - kLockOpenBaseEffect; }





}  // namespace mordor
