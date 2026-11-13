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

// R4 Id to table index: first entry with that Id, else index 0
i32 monsterTableIndexForId(const Ctx& ctx, i32 monsterId);

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

// R15 true when the lead must be rejected (restart) under the newbie condition
// lead != 4 uses table index 4, not Id 4
bool newbieLeadRejected(const Ctx& ctx, i32 leadIndex, i32 viewerLevel, i32 areaId);

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

// R19 identify level for a new group: fogged (bit 20) gives CLASS_NAME, otherwise OWN_NAME
IdentifyLevel initialIdentifyLevel(u32 groupFlags);

// R19 group name: MonsterGroups[MonsterClasses[cls].Index]
Str monsterGroupName(const Ctx& ctx, i32 classIndex);

// R19 monster display name by identify level (0 own, 1 class, 2 group)
Str monsterDisplayName(const Ctx& ctx, i32 monsterIndex, i32 classIndex, IdentifyLevel level);

// ---- R20 misidentification -------------------------------------------------

// R20 misidentification pick among kin (realIndex is the real monster's position in kin)
// CONFLICT: the trigger is roll > bar, but the shift is stated as upward when roll < bar; direction rule must be chosen by implementer.
// UNKNOWN: Log(0) guard for GuildLevel 0 is not stated; no monster has floor or guild level 0 in MDATA5.
MisidResult rollMisidentification(Ctx& ctx, f64 readerIdSkill, i32 guildLevel, i32 levelFound,
                                  i32 realIndex, const Vec<i32>& kin, ClassRange classRange);

// ---- R21 companion groups --------------------------------------------------

// R21 companion pick chained from the previous CompanionType, SpawnMode and Subtype
// UNKNOWN: the mode table is in the Tables section, not in this part.
// CONFLICT: Combat.cs invents a +/-1 floor and a same-kind rule; the source re-rolls and fills every group.
CompanionPick pickCompanionMonster(Ctx& ctx, const CompanionChain& previous, i32 targetFloor, i32 spread);

// ---- R22 spawn eligibility (test-stated) -----------------------------------

// R22 encounter floor for level and Stud flag
// UNKNOWN: Rules.EncounterFloor formula is not in this part.
i32 encounterFloor(i32 level, bool stud);

// R22 spawn eligibility: spawn mask bit set for class, Stud flag match, LevelFound <= encounter floor
bool spawnEligible(const Ctx& ctx, const Area& area, i32 monsterIndex, bool stud, i32 viewerLevel);

// ---- R23 members behind the front ------------------------------------------

// R23 unrolled member hits: max(1, base Hits)
// UNKNOWN: whether this is the final rule is not stated; see CONFLICT in R23 source.
i32 unrolledMemberHits(i32 baseHits);

// ---- R24 engagement roll ---------------------------------------------------
// R24 engagement roll: hostileBase and hostileRoll are declared in clock.h (API INDEX); call them.
// CONFLICT: once per tick (c45) vs once per second (c18); clock.h engagementTick decides the cadence.

// ---- R25 join offer pause --------------------------------------------------

// R25 true when a group's engagement roll is run (not paused by a live join offer)
bool groupRollsForEngagement(const MonsterGroup& group);

// ---- R26 engagement effects ------------------------------------------------
// R26 engagement effects: engagementTick, startFight and onMove are declared in clock.h (API INDEX).

// ---- R27 combat round order ------------------------------------------------
// R27 step 1 free attack: monstersFreeAttack and freeRound are declared in clock.h (API INDEX).

// R27 step 2 target pick: weights snapshot at start of enemy phase
// CONFLICT: R33 gives five weights (100,50,25,12,6) for four members; the 53/27/13/7 split does not match 100/50/25/12 (53/27/13/6).
Arr<i32, kPartyMaxSize> snapshotTargetWeights(const Party& party);

// R27 step 2 target pick by 1-100 roll against cumulative weights; returns party index or -1
i32 pickMonsterTarget(Ctx& ctx, const Arr<i32, kPartyMaxSize>& weights, const Party& party, i32 livingMembers);

// R27 step 3 melee targeting: true when the draw picks the character rather than a companion
bool meleeDrawPicksCharacter(Ctx& ctx, i32 livingCompanions);

// R27 step 3 companion sample: returns occupied living slot index, or -1 to fall back to the character
i32 pickCompanionSlot(Ctx& ctx, const Arr<Companion, kCompanionSlots>& companions);

// R27 step 4 melee blow resolution (uses monster Attack, STR, guild level, target defence, modifier and crit/backstab)
// UNKNOWN: whose STR (monster or target) is not stated; target's STR is used.
i32 monsterMeleeBlow(Ctx& ctx, const Creature& attacker, const Character& target, const Inventory& targetInv,
                     i32 sizeDiff, bool unseenInvisibleTarget);

// R27 step 5 apply accumulated character damage after the loop
void applyAccumulatedDamage(Ctx& ctx, Party& party, const Arr<i32, kPartyMaxSize>& damage);

// R27 step 5 apply companion damage immediately
void applyCompanionDamage(Ctx& ctx, Companion& companion, i32 amount);

// R27 step 7 lone body: the first group draws its target once; returns party index or -1
// UNKNOWN: the body of sub0E85 (leaves on DeathState) is not in this part.
i32 loneBodyTargetDraw(Ctx& ctx, const Party& party);

// R27 step 8 round continuation: continue while any monster stands and the combat is not over
bool roundPacedContinues(const Encounter& encounter, bool isOver);

// ---- R28 monster hits ------------------------------------------------------

// R28 monster hits rolled on load: sign, swing and MaxHits = Hits
// UNKNOWN: no floor is applied; a 1-hit creature gives 0 or 2.
RolledHits rollMonsterHits(Ctx& ctx, i32 baseHits);

// R28 audit swing bound: Int(Hits*0.5/2)+1 (c66)
// CONFLICT: the audit bound is not reached at all values; the engine bound is Int(Def.Hits*0.25)+1.
i32 auditSwingBound(i32 baseHits);

// ---- R29 standing at nought ------------------------------------------------

// R29 alive state: !Gone && (Hits > 0 || AtNought)
bool creatureAlive(const Creature& creature);

// R29 write hits; any write clears AtNought
void setCreatureHits(Creature& creature, i32 hits);

// ---- R30 melee blow modifier -----------------------------------------------

// R30 melee damage modifier: DamageMod (1 if not positive), +0.75 chest, +2 crit, +1 backstab, size term
// CONFLICT: unseen-invisible -0.5 is audit-only (c23), not in the engine chunk (c18); flag is passed through.
f64 meleeDamageModifier(f64 monsterDamageMod, bool openingChest, bool crit, bool backstab,
                        i32 sizeDiff, bool unseenInvisibleTarget);

// ---- R31 crit and backstab thresholds --------------------------------------

// R31 proc threshold: clamp(100*ln(1+M/k)*ln(floor+1) - 100*ln(1+D/999), lo, hi)
f64 procThreshold(const ProcSpec& spec, i32 attackerGuildLevel, i32 ownerGuildLevel, i32 levelFound);

// R31 proc roll: Int(Rnd*100)+1 <= threshold
bool rollProc(Ctx& ctx, f64 threshold);

// R31 companion melee defence bonus: +4 crit, +1 backstab
// CONFLICT: monster melee uses +2/+1 and companion defence uses +4/+1; flags 10 and 11 are said to enable crit and backstab.
i32 companionDefenceBonus(bool crit, bool backstab);

// ---- R32 free-attack threshold ---------------------------------------------
// R32 free-attack threshold: freeAttackThreshold is declared in clock.h (API INDEX).

// ---- R33 target weights ----------------------------------------------------
// R33 target weights 100, 50, 25, 12, 6: kGroupWeights is declared in clock.h (API INDEX).
// CONFLICT: see snapshotTargetWeights for the 53/27/13/7 split note.

// ---- R34 melee targeting of companions -------------------------------------

// R34 P(character) = 100/(living companions+1), strict below per draw
// (implemented by meleeDrawPicksCharacter above)

// ---- R35 companion breath chance -------------------------------------------

// R35 companion breath chance percent from lead and companion guild levels: 50 default, 80 high, 20 low
i32 companionBreathChance(i32 leadGuildLevel, i32 compLevel);

// R35 companion breath roll: Floor(rnd()*100) < chance
// CONFLICT: Log(0) on guild-level 0 breathers errors in the source; the port's status differs. Chance roll is drawn before any error.
bool rollCompanionBreath(Ctx& ctx, i32 leadGuildLevel, i32 compLevel);

// ---- R36 companion acid ----------------------------------------------------

// R36 companion acid slot draw
// CONFLICT: acid never hits slot 0 and draws a fifth slot that does not exist; the implementation draws only existing slots 1..kCompanionSlots-1.
i32 rollAcidSlot(Ctx& ctx);

// R36 breath and acid remove a companion only when hits are below 0; exactly 0 is removed by the next sweep
bool breathAcidRemoves(i32 hits);

}  // namespace mordor
