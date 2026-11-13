// death2a.cpp - Death module part A: complication roll, CON-loss rolls, raise age.
// SRD: R25 (complication chance), R26 (CON-loss roll), Table 4 (age added by cause).
#include "mordor/death2a.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

constexpr i32 kDaysPerYear = 365;        // R25 days per year
constexpr i32 kComplicationDrawRange = 100;  // R25 rng.Next(100): integer draw 0..99
constexpr i32 kConLossBase = 50;         // R26 constant 50
constexpr i32 kConLossPerCon = 2;        // R26 constant 2
constexpr i32 kConLossDrawRange = 100;   // R26 Int(Rnd*100): integer draw 0..99
constexpr i32 kConLossStep = 1;          // CONFLICT 27 (stone costs 1 CON); UNKNOWN: loss amount for other paths
constexpr i32 kAgeStoned = 84;           // Table 4 (stoned)
constexpr i32 kAgeRock = 124;            // Table 4 (rock)
constexpr i32 kAgeOther = 54;            // Table 4 (other causes)

}  // namespace

}  // namespace mordor
