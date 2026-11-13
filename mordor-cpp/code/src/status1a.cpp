// status1a.cpp: status effects (SRD c08p1): poison, disease, paralysis, stone,
// drain, aging, monster special attacks, cures.
// Event text is English; numeric payloads are omitted (nums/strs passed empty).
#include <cmath>

#include "mordor/status1a.h"
#include "mordor/math_util.h"
#include "mordor/combat1b.h"  // FIXED: declares SpecialAttackKind
#include "mordor/combat3.h"

namespace mordor {
namespace {

// R26 resistance indices.
constexpr i32 kResFire = 0;
constexpr i32 kResCold = 1;
constexpr i32 kResDisease = 4;
constexpr i32 kResPoison = 5;
constexpr i32 kResStone = 7;
constexpr i32 kResParalysis = 8;
constexpr i32 kResDrain = 9;

constexpr i32 kBitPoison = 0;       // R25 (no header constant)
constexpr i32 kConWeight = 2;       // R7: (effCON - 10) * 2
constexpr i32 kParalysisCap = 95;   // R12 cap on paralysis save chance
constexpr i32 kArchiveChanceFloor = 5;  // archive stone/paralysis minimum 5%
constexpr i32 kTheftSlotMin = 2;    // theft samples slots 2..40 (R36)
constexpr i32 kTheftSlotCount = 39;

// R27 audit breath thresholds, indexed by RatioBand (High, Even, Low).
constexpr Arr<i32, 3> kBreathThresholdPct{{80, 50, 20}};

}  // namespace

// ---------------------------------------------------------------- held state

// ---------------------------------------------------------- poison / disease

// ---------------------------------------------------------------- paralysis

// ------------------------------------------------------------ special attacks

// FIXED: forward declaration; defined below, used by monsterGroupSpecialTurn.
static i32 specialResistIndexOf(SpecialAttackKind kind);

// ------------------------------------------------------------ archive (c99)

}  // namespace mordor
