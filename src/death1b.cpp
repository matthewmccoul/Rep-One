// death1b.cpp: death and raise rules (R12 to R23, constants of R24).
// Module death1b.

// CONFLICT: death1b.h declares resolveRaise returning RaiseResult, which is defined in
// death1a.h. death1a.h is included first so the declaration is complete.
#include "mordor/death1a.h"
#include "mordor/death1b.h"
#include "mordor/math_util.h"

#include <cmath>
#include <cstddef>

// CONFLICT: death1b.h uses HallRecord (HallOfRecords::records). No given header defines
// HallRecord, so the header does not compile until it is defined elsewhere. Nothing here
// defines it.

// CONFLICT: hallStatField and hallDateField are declared both in death1b.h and in death3.h.
// They are defined once here (this module owns the death1b.h declarations). death3.h must not
// define them as well, or the link fails.

namespace mordor {

namespace {

// Module-local constants (constexpr only, no mutable state).
constexpr i64 kGoldToCurrency = 10000;             // Currency is 1/10000 gold
constexpr double kMorgueFeeFactor = 2912.32;       // R15 morgue fee factor
constexpr double kMorgueFeeDivisor = 1.5;          // R15 fee divisor
constexpr double kCompanionMorgueFactor = 1244.34; // R16 companion cost factor
constexpr i32 kCompanionMorgueBase = 75;           // R16 min(98, 75 + CON)
constexpr i32 kRaiseChanceCap = 98;                // R13, R14, R16 upper clamp
constexpr i32 kSpellChanceFloor = 10;              // R13 lower clamp
constexpr i32 kRockSpellChance = 10;               // R13 rock chance on the spell path
constexpr i32 kMorgueOwnBonus = 40;                // R14 own body bonus (CONFLICT 1)
constexpr i32 kMorgueCounterBonus = 10;            // R14 counter bonus (CONFLICT 1)
constexpr i32 kMorgueChanceFloor = 1;              // R14 quirk: tier lifted to 1
constexpr i32 kGuildPaysMaxLevel = 5;              // R15 lowLevel bound
constexpr i32 kRaiseRollRange = 100;               // Int(Rnd*100)
// UNKNOWN: the complication chance used on a quiet raise failure is not given in the SRD chunk.
// Stub 0 means a quiet failure never complicates.
constexpr i32 kComplicationChanceStub = 0;
// UNKNOWN: racial minimum stat values are not given in the SRD chunk, and RaceDef field names
// are not given. Stub floor 1.
constexpr i32 kRacialMinimumStub = 1;
constexpr i32 kCompanionConFallback = 10;          // R16 "?? 10" fallback
constexpr double kOldAgeThresholdPct = 98.0;       // R19 threshold
constexpr double kOldAgeChanceBase = 5.0;          // R19 "5 + Ln(...)"
constexpr i64 kHallStatMultiplier = 10000;         // R22 stat field factor
constexpr i64 kHallDateOffset = 2;                 // R22 date field offset
constexpr i32 kStoneConLoss = 1;                   // R12 quirk: stone costs 1 CON

}  // namespace

// ---------------------------------------------------------------------------
// Raise resolution helpers (R12)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Raise chances and fees (R13 to R15)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Companion raises (R16)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Death routines (R17, R18)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Old-age death (R19)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Character creation, deletion and kills (R20, R21)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Hall of Records and guild log (R22)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Retrieve Soul (R23) and input helpers
// ---------------------------------------------------------------------------

}  // namespace mordor
