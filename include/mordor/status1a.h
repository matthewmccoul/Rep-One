#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"
#include "mordor/combat1b.h"

namespace mordor {

// ---- Constants ----

// R6/R7/R8 ailment clock: rolls, cure thresholds and tick costs
inline constexpr i32 kAilmentRollRange = 200;     // Int(Rnd*200): draws 0..199
inline constexpr i32 kAilmentConBase = 10;        // roll offset is (effCON - 10) * 2
inline constexpr i32 kAilmentLevelWeight = 2;     // roll offset is - level * 2
inline constexpr i32 kPoisonCureAbove = 185;      // R8 poison cured if roll > 185
inline constexpr i32 kDiseaseCureAbove = 195;     // R8 disease cured if roll > 195
inline constexpr i32 kPoisonTickHits = 2;         // R7 hits lost when poison not cured
inline constexpr i32 kDiseaseTickHits = 1;        // R7 hits lost when disease not cured

// UNKNOWN: acid and electrical resistance indices are not given in this part of the SRD.

inline constexpr Arr<i32, 3> kSpitThresholdPct{{90, 50, 10}};
inline constexpr Arr<i32, 3> kElectrocutionThresholdPct{{90, 50, 10}};

// R30 stoning
inline constexpr i32 kStoneCap = 95;              // cap on engine stoning chance
inline constexpr i32 kBitDisease = 1;
inline constexpr i32 kBitParalysis = 2;
inline constexpr i32 kBitBreathFire = 3;
inline constexpr i32 kBitBreathCold = 4;
inline constexpr i32 kBitSpit = 5;
inline constexpr i32 kBitElectrocution = 6;
inline constexpr i32 kBitDrain = 7;
inline constexpr i32 kBitStone = 8;
inline constexpr i32 kBitAging = 9;
inline constexpr i32 kBitDestruction = 12;
inline constexpr i32 kBitTheft = 13;

// R31 drain (Table E)
inline constexpr i32 kDrainPickSides = 6;         // Int(Rnd*6+1): draws 1..6
inline constexpr i32 kDrainStatPickMax = 5;       // draws 1..5 drain a stat (index = draw)
inline constexpr i32 kDrainMaxHitsPick = 6;       // draw 6 drains MaxHits
inline constexpr i32 kDrainMaxHitsLoss = 2;       // MaxHits -2

// R32 aging (audit)
inline constexpr i32 kAgingDaysCap = 1825;        // max days in one aging event
inline constexpr i32 kAgingDaysPerUnit = 365;     // days per aging unit
inline constexpr i32 kAgingUnitMin = 1;           // clamp of B(M/50)
inline constexpr i32 kAgingUnitMax = 5;
inline constexpr i32 kAgingUnitDivisor = 50;      // M / 50

// R35 destruction (audit)
inline constexpr i32 kDestructionSlotMin = 1;     // pack slot sampled 1..39, empty slots included
inline constexpr i32 kDestructionSlotMax = 39;
inline constexpr i32 kDestructionSaveBase = 45;
inline constexpr i32 kDestructionSaveMin = 25;
inline constexpr i32 kDestructionSaveMax = 90;

// R36 theft (audit)
inline constexpr i32 kTheftScoreRange = 100;      // draw 0..99
inline constexpr i32 kTheftGoldThreshold = 50;    // gold attempted if score - 2M < 50
inline constexpr i32 kTheftGoldMonsterWeight = 2;

// R27/R29 band of a character level against monster level M
enum class RatioBand : i32 {
    High = 0,   // level > 1.25 M
    Even,       // 0.75 M to 1.25 M
    Low         // level < 0.75 M
};

// R16-R18 actions refused (or allowed) while held
enum class HeldAction : i32 {
    Move = 0,
    Turn,
    StairsUp,
    Exit,
    PickUpBody,
    DropBody,
    GiveBody,
    Fight,
    OpenChest,
    StairsDown,   // allowed while held (R18)
    Chute         // allowed while held (R18)
};

// R6/R7 result of one ailment-clock tick
struct AilmentTickResult {
    bool poisonCured = false;
    bool diseaseCured = false;
    i32 hitsLost = 0;
};

// R31 result of one drain attempt
struct DrainOutcome {
    i32 trials = 0;
    Vec<i32> picks{};     // Table E draw (1..6) for each failed save, in order
};

// ---- Rule routines ----




// R15 the stat part of Att/Def is removed while held: applied by paralyse (API INDEX), no routine here

// R13 Paralyse(level) and R14 Unparalyse: provided by API INDEX (paralyse, unparalyse)

// R21 other paralysis clears (companion kill cure, Cure Paralysis, EndEveryAilment) are provided
//     by their own modules (API INDEX endEveryAilment); not declared here

// R3 stone is DeathState 2 and survives the city-gate clear: state only, no routine

// R1/R4/R5 status storage and the design-guide bit table are state only, no routine









































}  // namespace mordor
