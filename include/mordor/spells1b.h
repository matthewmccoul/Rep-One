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

// ---- R56 Ethereal Portal ----
// R56 Ethereal Portal into rock: marks the square as rockRecorded. Port stops rather than hangs
// (original hangs at MAINMOD2.BAS:4586-4587; the one deliberate deviation).
PortalOutcome castEtherealPortal(Ctx& ctx, Party& party, SquareState& square, i32 casterSlot);

// ---- R57 Locate ----
// R57 classify one saved character relative to the caster's location (pure)
LocateLine classifyLocate(const Location& from, const Location& to, i32 rosterIndex);
// R57 relative direction from one square to another (pure)
RelativeDirection relativeDirection(i32 fromX, i32 fromY, i32 toX, i32 toY);
// R57 Locate (effect 14): one line per saved character, excluding the caster at casterIndex
void castLocate(Ctx& ctx, const Vec<Character>& roster, i32 casterIndex);

// ---- R58 Fetch candidates ----
// R58 roster indices of dead characters with deathState Rock, on the party's level, in area kCityArea (pure)
// UNKNOWN: "the leader's level" is read as party.dungeonLevel
Vec<i32> fetchCandidates(const Vec<Character>& roster, const Party& party);

// ---- R59 Fetch reach and move ----
// R59 literal test: a body is refused when its level is at or below kMaxDungeonLevel (pure)
// UNKNOWN: the literal test may be inverted relative to intent; follows the SRD text
bool isFetchWithinReach(i32 bodyDungeonLevel);
// R58/R59 Fetch (effect 15). pickedRosterIndex = -1 is Cancel and spends nothing.
FetchOutcome castFetch(Ctx& ctx, Party& party, Vec<Character>& roster, i32 pickedRosterIndex);

// ---- R60 Lock-opening ----
// R60 OpensLockUpTo(effect) = effect - 29 (trivial arithmetic)
inline constexpr i32 opensLockUpTo(i32 effect) { return effect - kLockOpenBaseEffect; }
// R60 open a chest lock. spellPointCost is the caster's cost from the spell data.
// UNKNOWN: SquareState has no lock-level field; chestLockLevel is supplied by the caller.
OpenLockOutcome castOpenLock(Ctx& ctx, Character& caster, SquareState& square, i32 effect,
                             i32 spellPointCost, i32 chestLockLevel);

// ---- R61 Binding ----
// R61 a candidate is a living companion (hits > 0) with bindingLevel != 0 (pure)
bool isBindCandidate(const Companion& comp);
// R61 indices of all candidates (pure)
Vec<i32> bindCandidates(const Vec<Companion>& companions);
// R61 caster picks pickedIndex (must be a candidate). Spends points, then applies SpellEffect.BindCompanion.
// effect 150 is bind for life. UNKNOWN: whether this is the items2b bindCompanion overload; not called here.
BindTargetOutcome castBinding(Ctx& ctx, Character& caster, Vec<Companion>& companions,
                              i32 pickedIndex, i32 effect, i32 spellPointCost);

// ---- R62 Fate ----
// R62 Fate (effect 19; also chest trap 6). Draws one unused random number, then names party.members[0]
// and rerolls its hits and spell points. Shared by both paths.
// UNKNOWN: whether the original lets spell level 50 cast Fate (only Elixr of Fate) is not verified.
void applyFate(Ctx& ctx, Party& party);
// R62 steps 2: CurHits = Int(rnd * MaxHits) + 1; CurSpellPoints = Int(rnd * MaxSpellPoints) + 1
void rerollFateVitals(Ctx& ctx, Character& who);

// ---- R63 Sanctuary ----
// R63 stored point for a Set Sanctuary cast: X + 1, Y + 1, DungeonLevel (pure)
SanctuaryPoint sanctuaryPointFor(const Party& party);
// R63 Set Sanctuary (effect 16): returns the new stored point
SanctuaryPoint castSetSanctuary(Ctx& ctx, const Party& party);
// R63 Sanctuary return (effect 17): ages the caster by kSanctuaryDays when a point is stored.
// UNKNOWN: whether the party is relocated to the stored point is not stated in this chunk.
SanctuaryOutcome castSanctuaryReturn(Ctx& ctx, Character& caster, const SanctuaryPoint& stored);
// R63 mdrcharAddAge(days). UNKNOWN: which age field (ageDays or daysAged) the port touches.
void addAgeDays(Character& who, i32 days);

// ---- R64 Utility effects 11-13 ----
// R64 Depth Perception and Find Direction end the lost state for the whole party
void clearPartyLost(Ctx& ctx, Party& party);
// R64 dispatch for effects 11, 12, 13.
// UNKNOWN: Detect Rock's effect is not described in this chunk; only the lost-state clearing is specified.
void castUtilityEffect(Ctx& ctx, Party& party, FloorState& floor, UtilityEffect effect);

}  // namespace mordor
