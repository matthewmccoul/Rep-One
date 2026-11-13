// progression1b.cpp
// Implements include/mordor/progression1b.h.
// Rule numbers (R13-R27) refer to SRD part c05p1.
#include "mordor/progression1b.h"

#include <cmath>

#include "mordor/math_util.h"

namespace mordor {
namespace {

// R13 XP closed-form constants (test/spreadsheet/archive forms).
constexpr double kXpCoeff = 88.09020776;
constexpr double kXpLinear = 500.0 / 11.0;  // 45.45454...
// CONFLICT: research and wiki use -1; test, spreadsheet and archive use -0.264.
// Resolution per SRD: engine-aligned closed form, so -0.264 is used everywhere here.
constexpr double kXpOffset = -0.264;

// R16 gold constant (combat-audit, archive loop, closed forms).
constexpr double kGoldStep = 30.1;

// R18 guild join constants.
constexpr double kJoinAbilityFactor = 1.25;
constexpr double kJoinCostScale = 5356.0;

// R19/R20 aging constants (days).
constexpr i32 kDaysPerYear = 365;

// R21 ailment tick constants.
constexpr i32 kPoisonClearAbove = 185;
constexpr i32 kDiseaseClearAbove = 195;
constexpr i32 kPoisonHitCost = 2;
constexpr i32 kDiseaseHitCost = 1;
constexpr i64 kAilmentTickMs = 4000;
constexpr double kOldAgePercentOnset = 98.0;

// R22/R24 rest constant.
constexpr double kRestPercentDivisor = 3.25;

// R26 CON loss constants.
constexpr i32 kConLossBase = 50;

// R27 play clock: SRD counts seconds, inputs here are milliseconds.
constexpr i64 kMsPerSecond = 1000;

}  // namespace

}  // namespace mordor
