// status2.cpp: Chapter 8 status effects (SRD c08p2).
// Sources: preserved formulas (poison, disease, paralysis), Tables A to L, and the
// quirk and conflict notes in the SRD part. The SRD part has no R-numbers, so
// citations use the table letters and the c-refs from the notes.
#include "mordor/status2.h"

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/companions1b.h"  // stoneChance(con, levelFound) is implemented there

namespace mordor {

namespace {

// Paralysis and stone save floor. CONFLICT: the archive gives a 5% minimum
// (c99, Table K "5..90"), and the engine caps at 95 (c14). Both are applied here.
constexpr i32 kParalysisChanceFloor = 5;
constexpr i32 kStoneChanceFloor = 5;
// UNKNOWN: paralysis hold duration in rounds is not supplied in the SRD part.
constexpr i32 kParalysisHoldRounds = 1;
// Port's help cry threshold: curHits below maxHits / 4. UNKNOWN: original threshold.
constexpr i32 kHelpCryLowDivisor = 4;
// Port ambient sound: flat 10% per tick when nothing is in front (c45). UNKNOWN original.
constexpr i32 kAmbientSoundPercent = 10;

}  // namespace

// ---------------------------------------------------------------------------
// Resistances and affliction (c08p2 preserved formulas, Table B)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Paralysis (c08p2 preserved formula, Table K, quirks list)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Poison and disease cures and ticks (Table F, quirks, CONFLICT 1 to 2)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Death paths (Table J, c30, c32, c34, c44, c45)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Stoning, old age, ambient sound
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Monster special attacks (Tables C and D)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Aging (c23, Table K)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Drain (Table E, quirks, CONFLICT 11 and 12)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Chest traps (Table G)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Cure spells and town effects (Table H, c2)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Death-effect spell names (Table I)
// ---------------------------------------------------------------------------

}  // namespace mordor
