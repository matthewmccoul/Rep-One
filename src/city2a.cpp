// city2a.cpp - Library writes and grades, Seer service, Guild Log clean,
// temple/morgue fill, gold transfer parse, companion number entry,
// equip toggle, price models. Rules cited as R-numbers from SRD c16p2.

#include "mordor/city1.h"   // FIXED: ChargeResult type for charPay prototype
#include "mordor/city2a.h"
#include "mordor/city2b.h"  // charPay (R64 step 3)

#include <algorithm>
#include <cmath>
#include <string>

namespace mordor {

// FIXED: charPay is declared in city2b.h per the API index, but the compiler
// reports it as not declared here. Restated with the same signature so this TU
// sees it; it is defined elsewhere, so it is not static.
ChargeResult charPay(Ctx& ctx, Character& payer, Currency cost);

namespace {

// UNKNOWN: floor dimensions are not in this chunk; spot range is a placeholder.
constexpr i32 kSeerSpotSpan = 20;
// UNKNOWN: wiki success roll is unsourced (R64 step 6); 50% is a placeholder.
constexpr i32 kWikiSuccessPercent = 50;
// R55: skill-0 readers facing a creature fifteen or more levels harder get grade 1.
constexpr i32 kMonsterGradeLevelGate = 15;
// R62 / R49-R56 style constants.
constexpr i64 kMillisPerDay = 86400000LL;
// Days from 1899-12-30 (serial 0) to 1970-01-01 (Unix day 0).
constexpr i64 kUnixToSerialDays = 25569LL;
// UNKNOWN: CombatEngine.IdLevelRoll die size is not in this chunk.
constexpr i32 kIdRollDie = 4;
// R49 / R56: grades for the item grade mapping.
constexpr i32 kGradeFull = 4;
constexpr i32 kGradeMostly = 3;

}  // namespace

// ---------------------------------------------------------------------------
// Library grade mapping
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Library writes
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Library merge, keys, settling
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// ID level roll
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Seer
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Guild Log
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Temple and Morgue
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Gold transfer input (R67)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Companion number entry (R68)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Equip toggle (R70)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Price models (R72, R73; wiki transcript, unverified)
// ---------------------------------------------------------------------------

}  // namespace mordor
