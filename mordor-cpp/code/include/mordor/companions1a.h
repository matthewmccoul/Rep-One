#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"

namespace mordor {

struct GameState;  // defined by the game state layer; passed by reference only

// ---- Shared companion types ----

// Per-owner companion slots 0..3 (kCompanionSlotsPerCharacter, see state_companion.h)
using CompanionSlots = Arr<Companion, kCompanionSlotsPerCharacter>;

// R3 / R4 constants
inline constexpr i32 kCompanionId307 = 307;
inline constexpr i32 kId307NeutralRef = 278;  // Morgul
inline constexpr i32 kId307EvilRef = 389;     // Shadow Ninja

// R7 / R11 binding constants (kBindingMax == 100 is ForLife, see state_companion.h)
inline constexpr i32 kFreeWillBinding = -10;

// R8 starting and bought binding
inline constexpr i32 kBindingBase = 5;
inline constexpr i32 kStartBindingCap = 10;
inline constexpr i32 kBoughtBindingCap = 10;

// R9 binding cost
inline constexpr i32 kBindingStep = 4;
inline constexpr i32 kBindingPurchaseMax = 12;
inline constexpr i32 kBindingCostConst = 1237;
inline constexpr i32 kBindingCostDivisor = 4;

// R10 / R13 join offers
inline constexpr i32 kJoinRollMax = 3000;
inline constexpr i32 kJoinBarChaScale = 10;
inline constexpr i32 kJoinBarLevelScale = 10;
inline constexpr i32 kJoinOfferGroupCap = 4;
inline constexpr f64 kVoluntaryJoinDivisor = 300.0;

// R15 subtype draw limits
inline constexpr i32 kSubtypeResetTries = 150;
inline constexpr i32 kSubtypeFailTries = 300;


// R23 breath
inline constexpr i32 kBreathSweepLast = 4;  // sweep index limit; the fifth kill gives up
inline constexpr i32 kBreathChanceStrong = 80;
inline constexpr i32 kBreathChanceWeak = 20;
inline constexpr f64 kBreathLogBase = 1.08;
inline constexpr f64 kBreathSeedDivisor = 15.0;


// R25 spit retry cap (port only)
inline constexpr i32 kSpitRetryCap = 1000;

// R24 mishap kinds: factor and clamp range per kind
enum class MishapKind : i32 {
    Breath = 0,
    Spell = 1,
    Spit = 2
};

struct MishapRow {
    f64 factor;
    f64 low;
    f64 high;
};

inline constexpr MishapRow kMishapRows[3] = {
    {2.0, 2.0, 40.0},   // Breath
    {1.5, 1.0, 40.0},   // Spell
    {1.0, 0.5, 20.0}    // Spit
};

// R19 pen stock. UNKNOWN: pen layout is not given in this SRD part; one shelf per monster and alignment is assumed.
struct PenShelf {
    i32 monsterRef = -1;
    CompanionAlignment alignment = CompanionAlignment::Good;
    i32 stock = 0;
};

struct Pen {
    Vec<PenShelf> shelves{};
};

// R15 result of a subtype draw limit check
struct SubtypePick {
    i32 subtype = 0;
    i32 tries = 0;
    bool failed = false;
};

// R16 spawn window (levels below and above the lead)
struct SpawnWindow {
    i32 below = 0;
    i32 above = 0;
};

// R16 result of a spawn mode lookup
struct SpawnPick {
    SpawnWindow window{};
    i32 tableEntry = -1;   // -1 means no table entry
    bool leadItself = false;
};

// ---- Trivial accessors ----

// R7 bound companion: positive binding, can turn against its owner
inline bool companionBound(const Companion& c) { return c.bindingLevel > 0; }

// R7 ForLife binding (100): never drifts, leaves or is bought
inline bool companionForLifeBinding(const Companion& c) { return c.bindingLevel == kBindingMax; }

// ---- Declarations ----


























// R14 monsters that join of their own will are bound at -10
// UNKNOWN: whether a Bind spell can raise a joiner's -10 binding is not stated; no routine declared here












// R23 companion breath range: a = atk + 1; Int((Log(a) + a/15) / Log(1.08) - tgt/10 + 1)
// UNKNOWN: whether tgt/10 is real or integer division











}  // namespace mordor
