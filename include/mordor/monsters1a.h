#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_creature.h"

// Forward declaration: Party is defined in state_party.h, which must not be
// included here together with state_dungeon.h (state_character.h has no guard).
namespace mordor {
struct Party;
}

namespace mordor {

// ---- constants from the SRD (no routine) ----------------------------------

// R3 Asmodeus identity
inline constexpr i32 kAsmodeusId = 277;

inline constexpr i32 kMorgulTableIndex = 278;
inline constexpr i32 kShadowNinjaTableIndex = 389;

// R8 preconditions
inline constexpr i32 kRockSquareId = 1432;
inline constexpr i32 kNoEncounterAreaId = 0;

// R9 lair roll
inline constexpr i32 kLairChancePercent = 75;

// R10 class pick: roll must be below 2 (1% any-class), newbie area limit
inline constexpr i32 kAnyClassRollBelow = 2;
inline constexpr i32 kNewbieLevel = 1;
inline constexpr i32 kNewbieMaxAreaId = 18;

// R12/R13 window and spread
// UNKNOWN: the initial spread value is not stated in this part; 0 is used.
inline constexpr i32 kInitialSpread = 0;
inline constexpr i32 kSpreadRestartLimit = 3;

// R15 newbie lead guard
inline constexpr i32 kNewbieLeadMaxGuildLevel = 2;
inline constexpr i32 kNewbieExemptLeadIndex = 4;

// R16 group count minimum (maximum is kMaxGroups from state_creature.h)
inline constexpr i32 kGroupCountMin = 1;

// R18 group 0 hostility: percent chance a Neutral lead is peaceful
inline constexpr i32 kNeutralPeacePercent = 40;

// R19 fog flag bit in group special property flags
inline constexpr u32 kFogFlagBit = 1u << 20;

// R20 misidentification bar and redraw limit
inline constexpr i32 kMisidBarBase = 50;
inline constexpr i32 kMisidBarPerSkill = 10;
inline constexpr i32 kMisidBarMin = 5;
inline constexpr i32 kMisidBarMax = 95;
inline constexpr i32 kMisidRedrawTries = 100;

// R27 melee target attempts cap
inline constexpr i32 kMaxTargetAttempts = 5;


// R31 threshold owner scale (the 999 in the formula)
inline constexpr f64 kProcOwnerScale = 999.0;

// R35 companion breath chances (percent) and level band
inline constexpr i32 kBreathChanceLow = 20;
inline constexpr i32 kBreathChanceMid = 50;
inline constexpr i32 kBreathChanceHigh = 80;
inline constexpr f64 kCompanionLevelBand = 0.25;

// ---- small result types ----------------------------------------------------

// R14 first and last table index of one class (-1 when absent)
struct ClassRange {
    i32 first = -1;
    i32 last = -1;
};

// R8 outcome of the encounter precheck
enum class EncounterPrecheck : i32 {
    Blocked = 0,
    UseStored = 1,
    Roll = 2
};

// R9 outcome of the lair roll
enum class LairOutcome : i32 {
    NotTaken = 0,
    Failed = 1,
    Lead = 2
};

struct LairRoll {
    LairOutcome outcome = LairOutcome::NotTaken;
    i32 leadIndex = -1;
};

// R13 monster pick within a class window
struct MonsterPick {
    bool found = false;
    i32 monsterIndex = -1;
};

// R21 companion chain state carried between picks
struct CompanionChain {
    i32 companionType = 0;
    i32 spawnMode = 7;
    i32 subtype = 0;
};

struct CompanionPick {
    bool restart = false;
    i32 monsterIndex = -1;
    CompanionChain chain{};
};

// R20 misidentification result
struct MisidResult {
    bool misidentified = false;
    i32 pickIndex = -1;
};

// R28 hits rolled on load
struct RolledHits {
    i32 hits = 0;
    i32 maxHits = 0;
};

// R31 proc threshold constants (crit and backstab)
struct ProcSpec {
    f64 k = 0.0;
    i32 lo = 0;
    i32 hi = 0;
};

inline constexpr ProcSpec kCritProcSpec{650.0, 2, 50};
inline constexpr ProcSpec kBackstabProcSpec{500.0, 4, 75};

// ---- R1..R7: documentation only, no routine --------------------------------
// R1 source precedence, R2 data table addressing, R5 c66 tag meaning,
// R6 known gaps, R7 stat block meaning: no routines in this part.

// ---- R3 identity by Id -----------------------------------------------------

// R3 Asmodeus is identified by Id alone
inline bool isAsmodeus(i32 monsterId) { return monsterId == kAsmodeusId; }

// ---- R4 shared Id lookup ---------------------------------------------------


// ---- R8 preconditions ------------------------------------------------------

// R8 encounter precheck: rock square or area 0 blocks; positive area with stored encounter uses it
// CONFLICT: zero spawn mask has a conflict whose detail is not in this part; implementation returns Roll
EncounterPrecheck encounterPrecheck(i32 squareCode, i32 areaId, bool hasStoredEncounter, i32 spawnMask);

// ---- R9 lair roll ----------------------------------------------------------

// R9 lair roll (redrawn on every restart); RndF()*100 <= 75 when LairId > 0
LairRoll rollLair(Ctx& ctx, const Area& area);

// ---- R10 class pick --------------------------------------------------------

// R10 class count: MonsterClasses.Count if > 0, else max(m.Type)+1
i32 monsterClassCount(const Ctx& ctx);

// R10 newbie flag: viewer level 1 and area id below 18
bool isNewbie(i32 viewerLevel, i32 areaId);

// R10 class pick (skipped on lair): newbie check, 1% any-class roll, else mask-enabled classes
// CONFLICT: R10 empty-list fallback is Int(Rnd*nc); R11 fallback is list[CInt(Rnd*(nc-2))]. Implementation follows R10 for the empty list.
i32 pickEncounterClass(Ctx& ctx, const Area& area, i32 viewerLevel, i32 areaId, const Vec<ClassRange>& ranges);

// ---- R11 weighted class pick -----------------------------------------------

// R11 weighted class pick by cumulative Chance
// Original quirk: acc starts at v[0] and the first class weight is counted twice; leftovers go to the last class.
i32 pickWeightedClass(Ctx& ctx, const Vec<i32>& list, const Vec<i32>& weights, i32 classCount);

// ---- R12 target floor and eligibility window -------------------------------

// R12 target floor: viewer level, plus 1 on a Stud square
i32 encounterTargetFloor(i32 viewerLevel, bool stud);

// R12 eligibility window: LevelFound in [targetFloor - spread, targetFloor]
bool inEligibilityWindow(i32 levelFound, i32 targetFloor, i32 spread);

// ---- R13 monster within class ----------------------------------------------

// R13 monster within class by cumulative weight among window entries; found=false when no weight
MonsterPick pickMonsterInClass(Ctx& ctx, ClassRange range, i32 targetFloor, i32 spread);

// R13 true when spread has gone past the restart limit
bool spreadExhausted(i32 spread);

// R13 widen the spread by one step
inline i32 widenSpread(i32 spread) { return spread + 1; }

// ---- R14 class ranges ------------------------------------------------------

// R14 first and last table index of each class
// UNKNOWN: the scan as written starts at table index 1; whether index 0 belongs to a class is not stated.
Vec<ClassRange> buildClassRanges(const Ctx& ctx);

// ---- R15 newbie lead guard -------------------------------------------------


// ---- R16 group count -------------------------------------------------------

// R16 group count from lead NumGroups, clamped to 1..kMaxGroups
// CONFLICT: the c24 form clamp(NumGroups - roll(NumGroups/8), 1, 4) differs from the c7 form; c7 form is implemented.
i32 rollGroupCount(Ctx& ctx, i32 numGroups);

// ---- R17 group size --------------------------------------------------------

// R17 monsters in this group: InGroup - CInt(Rnd*(InGroup/2.0)), minimum 1
i32 rollGroupSize(Ctx& ctx, i32 inGroup);

// ---- R18 group 0 hostility -------------------------------------------------

// R18 group 0 engagement: returns true when engaged (hostile)
// UNKNOWN: Evil lead engagement rule is not in this part; Engaged stays true.
bool rollLeadEngaged(Ctx& ctx, i32 leadAlignmentValue, Align viewerAlign);

// ---- R19 front hits, fog and naming ----------------------------------------




// ---- R20 misidentification -------------------------------------------------


// ---- R21 companion groups --------------------------------------------------


// ---- R22 spawn eligibility (test-stated) -----------------------------------



// ---- R23 members behind the front ------------------------------------------

// R23 unrolled member hits: max(1, base Hits)
// UNKNOWN: whether this is the final rule is not stated; see CONFLICT in R23 source.
i32 unrolledMemberHits(i32 baseHits);

// ---- R24 engagement roll ---------------------------------------------------
// R24 engagement roll: hostileBase and hostileRoll are declared in clock.h (API INDEX); call them.
// CONFLICT: once per tick (c45) vs once per second (c18); clock.h engagementTick decides the cadence.

// ---- R25 join offer pause --------------------------------------------------


// ---- R26 engagement effects ------------------------------------------------
// R26 engagement effects: engagementTick, startFight and onMove are declared in clock.h (API INDEX).

// ---- R27 combat round order ------------------------------------------------
// R27 step 1 free attack: monstersFreeAttack and freeRound are declared in clock.h (API INDEX).


// R27 step 2 target pick by 1-100 roll against cumulative weights; returns party index or -1
i32 pickMonsterTarget(Ctx& ctx, const Arr<i32, kPartyMaxSize>& weights, const Party& party, i32 livingMembers);








// ---- R28 monster hits ------------------------------------------------------

// R28 monster hits rolled on load: sign, swing and MaxHits = Hits
// UNKNOWN: no floor is applied; a 1-hit creature gives 0 or 2.
RolledHits rollMonsterHits(Ctx& ctx, i32 baseHits);


// ---- R29 standing at nought ------------------------------------------------



// ---- R30 melee blow modifier -----------------------------------------------


// ---- R31 crit and backstab thresholds --------------------------------------


// R31 proc roll: Int(Rnd*100)+1 <= threshold
bool rollProc(Ctx& ctx, f64 threshold);


// ---- R32 free-attack threshold ---------------------------------------------
// R32 free-attack threshold: freeAttackThreshold is declared in clock.h (API INDEX).

// ---- R33 target weights ----------------------------------------------------
// R33 target weights 100, 50, 25, 12, 6: kGroupWeights is declared in clock.h (API INDEX).
// CONFLICT: see snapshotTargetWeights for the 53/27/13/7 split note.

// ---- R34 melee targeting of companions -------------------------------------

// R34 P(character) = 100/(living companions+1), strict below per draw
// (implemented by meleeDrawPicksCharacter above)

// ---- R35 companion breath chance -------------------------------------------



// ---- R36 companion acid ----------------------------------------------------



}  // namespace mordor
