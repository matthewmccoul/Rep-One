#pragma once
// Status effects: poison, disease, paralysis, stoning, drain, death routines, chest traps, cures.
// SRD part c08p2 gives no R-numbers, so cites use its source tags [cNN].
// Not declared here (no engine mechanics in this part): status flag bits (Table L, design guide only),
// curse, sleep, fear, invisibility, item destruction (all UNKNOWN in the SRD).

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants and tables ----

// Four-second ailment tick interval [c45]
inline constexpr i32 kAilmentTickMs = 4000;

// Ailment store indices (Table A) [c11][c68]
enum class AilmentSlot : i32 {
    Poison = 0,
    Disease = 1,
    Paralysis = 2,
    Levitating = 3,
    Invisible = 4,
    SeeInvisible = 5,
    Protected = 6
};

// Resistance indices used in this part (Table B) [c14][c4][c23][c20]
// Acid and Electrical indices are UNKNOWN and deliberately not declared.
inline constexpr i32 kResistFire = 0;
inline constexpr i32 kResistCold = 1;
inline constexpr i32 kResistDisease = 4;
inline constexpr i32 kResistPoison = 5;
inline constexpr i32 kResistStone = 7;
inline constexpr i32 kResistParalysis = 8;
inline constexpr i32 kResistDrain = 9;

// Caps and clamps [c18][c23] (Table K)
inline constexpr i32 kParalysisChanceCap = 95;
inline constexpr i32 kStoneChanceCap = 95;
inline constexpr i32 kTickSaveMin = 5;   // monster tick save clamp
inline constexpr i32 kTickSaveMax = 90;

// Poison and disease cure (Table F) [c19][c25][c29][c45]
inline constexpr i32 kPoisonCureRollAbove = 185;
inline constexpr i32 kDiseaseCureRollAbove = 195;
inline constexpr i32 kCureRollSides = 200;          // Int(Rnd*200)
inline constexpr i32 kCureConBase = 10;             // (effCON-10)*2
inline constexpr i32 kCureConFactor = 2;
inline constexpr i32 kCureLevelFactor = 2;          // -ailmentLevel*2
inline constexpr i32 kPoisonHitsPerFail = 2;
inline constexpr i32 kDiseaseHitsPerFail = 1;

// Breath and spit bands (Table D) [c23]
// CONFLICT: archive c99 uses a different lower band edge (1.25M); resolution is c23.
inline constexpr double kBandHighRatio = 1.25;      // victim level > 1.25 * monster level
inline constexpr double kBandLowRatio = 0.75;       // victim level < 0.75 * monster level
inline constexpr i32 kBreathHighPercent = 80;
inline constexpr i32 kBreathMidPercent = 50;
inline constexpr i32 kBreathLowPercent = 20;
inline constexpr i32 kSpitHighPercent = 90;
inline constexpr i32 kSpitMidPercent = 50;
inline constexpr i32 kSpitLowPercent = 10;

// Monster aging (c23) [c23][c18]
// CONFLICT: archive c99 uses min(1825, M*7.3); resolution is c23.
inline constexpr i32 kMaxAgingDays = 1825;
inline constexpr i32 kAgingDaysPerYear = 365;
inline constexpr i32 kAgingClampMin = 1;
inline constexpr i32 kAgingClampMax = 5;
inline constexpr i32 kAgingMonsterDivisor = 50;     // B(M/50); B() UNKNOWN, see rollAgingDays

// Death spell names (Table I) [c97]
inline constexpr const char* kKilledSpellNames[6] = {
    "Cause Wounds", "Cause Heavy Wounds", "Power", "Harm", "Cause Fatal Wounds", "Word of Death"};
inline constexpr const char* kDrainedSpellNames[1] = {"Draining Touch"};

// Monster special-attack bits (Table C) [c23]
enum class SpecialBit : i32 {
    Poison = 0,
    Disease = 1,
    Paralysis = 2,
    BreathFire = 3,
    BreathCold = 4,
    Spit = 5,
    Electrocution = 6,
    Drain = 7,
    Petrification = 8,
    Aging = 9,
    Destruction = 12,
    Theft = 13
};

// Cause of death. UNKNOWN: full list not supplied; only Rock is named as DeathRock [c44].
enum class DeathCause : i32 {
    Unspecified = 0,
    Rock = 1,
    Drowning = 2,
    OldAge = 3,
    Poison = 4,
    Disease = 5
};

// Death spell effect (Table I) [c97]
enum class DeathSpellEffect : i32 {
    None = 0,
    Killed = 1,
    Drained = 2
};

// Cure spells that act on a target (Table H) [c89]
// UNKNOWN: Table H "Control" heading is garbled; RemovePoison follows its text.
// Cure Paralysis is handled by castCureParalysis.
enum class CureSpellKind : i32 {
    RemovePoison = 0,
    RemoveDisease = 1,
    Cure = 2                // heals hits (amount UNKNOWN), removes poison and disease
};

// Drain outcome (Table E) [c14][c27][c63]
// hitsDrain true: MaxHits -2 (amount -2). Otherwise stat drain of -1 on stat.
// Strength is unreachable (draw starts at 1). CONFLICT: c84 says strength can be drained; resolution is engine.
struct DrainPick {
    bool hitsDrain = false;
    StatIndex stat = StatIndex::Int;
    i32 amount = 0;
};

// ---- c2 Poison, disease and paralysis rolls ----














// ---- c45 Ailment tick (per-member checks; the tick loop itself is ailmentTick in clock.h) ----






// ---- c2/c44/c45 Death ----











// ---- c23 Monster special attacks ----











// ---- c4 Chest traps and c89 cures ----









}  // namespace mordor
