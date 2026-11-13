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

// R2 a character is held when ParalysisLevel is not 0
bool isHeld(const Character& c);

// R16 a held character does nothing on its own action (companions still fight)
bool canActOnOwnTurn(const Character& c);

// R17/R18 whether the held character is refused this action (down stairs and chute are allowed)
bool heldActionRefused(const Character& c, HeldAction action);

// R15 the stat part of Att/Def is removed while held: applied by paralyse (API INDEX), no routine here

// R13 Paralyse(level) and R14 Unparalyse: provided by API INDEX (paralyse, unparalyse)

// R21 other paralysis clears (companion kill cure, Cure Paralysis, EndEveryAilment) are provided
//     by their own modules (API INDEX endEveryAilment); not declared here

// R3 stone is DeathState 2 and survives the city-gate clear: state only, no routine

// R1/R4/R5 status storage and the design-guide bit table are state only, no routine

// R7 effective CON term of the ailment rolls: (effCon - 10) * 2 - level * 2
// CONFLICT: effCon = base CON + gear CON (fidelity doc); the port passes CON without gear.
//           This header follows the fidelity doc.
i32 ailmentRollOffset(i32 effCon, i32 level);

// R6/R7 poison then disease on the four-second tick: mutates hits and levels
// Poison fail costs 2 hits; disease fail costs 1 hit. Disease roll uses poisonLevel as 0 when
// poison was cured on this same tick.
// UNKNOWN: the source gives both the port form (disease reads disease level) and the fidelity form
//          (disease reads poison level). The fidelity form is the measured quirk and is used.
AilmentTickResult poisonDiseaseTick(Ctx& ctx, Character& c);

// R9 melee poison and disease: each save is skipped if the victim already has that affliction
void inflictPoisonAndDisease(Ctx& ctx, Character& victim, i32 attackerLevel);

// R10 clear poison (flag and level)
// CONFLICT: the port clears the flag but leaves the level. This header clears both.
void curePoison(Ctx& ctx, Character& c);

// R10 clear disease (flag and level)
// CONFLICT: as curePoison.
void cureDisease(Ctx& ctx, Character& c);

// R11 disease halves healing while diseased
// UNKNOWN: which healing is affected (single source). Halving uses integer division.
i32 healingWhileDiseased(const Character& c, i32 healing);

// R11 poison is cured while resting in town
// UNKNOWN: the exact town-rest trigger (single source).
void townRestCuresPoison(Ctx& ctx, Character& c);

// R12 melee paralysis save on a damaging hit; calls paralyse when it fails and not already in a round
// Returns true when this call paralysed the character. Same routine serves the R34 audit form.
bool meleeParalysisCheck(Ctx& ctx, Character& c, i32 monsterLevel);

// R12/R34 paralysis chance percent: min(Res8 + CON, 95)
i32 meleeParalysisChance(const Character& c);

// R19 paralysis wear-off chance (clamped 5..90) on the four-second tick; pure arithmetic
// CONFLICT: the port gates wear-off on a combat object; the engine gates on monsters on the square (R20).
// UNKNOWN: whether monsterGuildLevel equals ailments.paralysisLevel (both are used in the formula).
double paralysisWearOffChance(const Character& c, i32 monsterGuildLevel);

// R19 wear-off roll on the four-second tick; returns true when paralysis ends (calls unparalyse)
bool paralysisWearOffTick(Ctx& ctx, Character& c, i32 monsterGuildLevel);

// R20 paralysis always ends when no monster is on the square; returns true when cleared
bool paralysisNoMonsterClear(Ctx& ctx, Character& c, bool monsterOnSquare);

// R22 monster group special-attack turn, in this order: breath, damage, electrocution (back to front),
// melee side effects (poison, disease, paralysis, stone, drain, aging, destruction, theft).
// UNKNOWN: relative ordering among the melee side effects. Companion-slot breath is handled
//          by the companion module and is not applied here.
void monsterGroupSpecialTurn(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);

// R23 special-attack resistance: VbInt(dmg - dmg * ((res / 2.0) * 0.01)); no change when res <= 0
// UNKNOWN: whether VbInt means floor (Int) or half-even (CInt). Uses vbInt (floor).
i32 applySpecialResistance(i32 damage, i32 resistPercent);

// R24 resistance index for an attack kind; returns -1 where UNKNOWN (acid spit, electrocution)
i32 specialResistIndex(SpecialAttackKind kind);

// R27 band of a character level against monster level M (1.25 and 0.75 edges, audit c23)
RatioBand ratioBand(i32 characterLevel, i32 monsterLevel);

// R27 breath threshold percent, using the character's highest guild level
// CONFLICT: archive band edges differ (c99); audit edges used.
i32 breathThresholdPercent(i32 highestGuildLevel, i32 monsterLevel);

// R27 spit threshold percent, using the active level
// UNKNOWN: mapping of spit values to branches is inferred by position (see R27 table).
i32 spitThresholdPercent(i32 activeLevel, i32 monsterLevel);

// R28 breath damage (audit form): x = B(M*ln(M+1)); uses one uniform draw
// UNKNOWN: rounding mode of B and range of U. Uses roundHalfEven for B.
i32 rollBreathDamage(Ctx& ctx, i32 monsterGuildLevel, i32 victimActiveLevel);

// R28 spit damage (audit form): x = M+1; uses one uniform draw
i32 rollSpitDamage(Ctx& ctx, i32 monsterGuildLevel, i32 victimActiveLevel);

// R29 electrocution chance percent by level band (archive form, 90/50/10)
// UNKNOWN: the level the engine uses. Archive uses character level.
i32 electrocutionChancePercent(i32 characterLevel, i32 monsterLevel);

// R30 engine stoning chance: min(tCon * (ln(max(1,tCon)) / ln(2.5)) - LevelFound, 95); pure
double stoneChance(const Character& target, i32 attackerLevel);

// R30 engine stoning save: stones the target (DeathState Stoned) when the roll fails the chance
// UNKNOWN: exact comparator (source is garbled). Uses Rnd(100)+1 > chance.
bool stoneCheck(Ctx& ctx, Character& target, i32 attackerLevel);

// R22 step 5 / R30 companion test: threshold stoneValue = CON * (ln(CON) / ln(2.5)) - stoneFloor
// UNKNOWN: general value of stoneFloor (implied 0 in the test example). Pure arithmetic.
double companionStoneValue(i32 companionCon, i32 stoneFloor);

// R22 step 5 / R30 companion blow: stoned when Rnd*100 > stoneValue; a stoned monster loses its hits
bool companionBlowStones(Ctx& ctx, Creature& target, i32 companionCon, i32 stoneFloor);

// R31 engine drain: trials = Int(Rnd * VbInt(LevelFound/2.0 + 0.5)) + 1
// Per trial: skip on a resistance-9 save, then on the CON save; each failed save draws a Table E pick.
// Strength (stat index 0) is unreachable.
// CONFLICT: manual (c84) says undead drain strength, intelligence and life. Engine and fidelity used.
DrainOutcome drainCharacter(Ctx& ctx, Character& target, i32 attackerLevel);

// R31 Table E effect: pick 1..5 drains stat index pick by 1; pick 6 drains MaxHits by 2
void applyDrainPick(Ctx& ctx, Character& c, i32 pick);

// R32 aging save chance (audit): 2.5 * CON - 2 * F; pure
double agingSaveChance(const Character& c, i32 monsterFloor);

// R32 aging (audit): on a failed save ages 1..min(1825, 365 * clamp(B(M/50), 1, 5)) days
// Returns days aged. Adds to ageDays.
// CONFLICT: archive day cap is 1825 with M*7.3 scaling (c99); manual says about twenty years (c84).
//           Audit form used.
// UNKNOWN: whether daysAged or ageDays is the aged counter. ageDays is used.
i32 rollAgingDays(Ctx& ctx, Character& c, i32 monsterGuildLevel, i32 monsterFloor);

// R33 audit petrification: saves at Stone resistance, then at min(95, CON*ln(max(1,CON))/ln(2.5) - F).
// Both must fail to stone the character. Returns true when stoned.
// CONFLICT: overlaps R30 engine stoning; engine (stoneCheck) is preferred for stoning.
// Removal of a companion is handled by the companion module.
bool petrifyCheck(Ctx& ctx, Character& target, i32 monsterFloor);

// R35 destruction save chance percent: clamp(45 - F + 4*DEX - 2*monDEX, 25, 90); pure
// UNKNOWN: the DEX term drops to DEX while paralysed (meaning not explained). Uses c.abilities DEX.
i32 destructionSaveChance(const Character& c, i32 monsterFloor, i32 monsterDex);

// R35 destruction (audit): samples a pack slot 1..39 (empty slots included); on a failed save the
// item in that slot is destroyed. Returns true when an item was destroyed.
bool destructionCheck(Ctx& ctx, Character& c, Inventory& inv, i32 monsterFloor, i32 monsterDex);

// R36 theft score: Int(Rnd*100) + I(U*(thiefLevel*thiefCoefficient) + 0.75*thiefLevel)
// Defending adds the integer half of the score.
// UNKNOWN: thiefCoefficient values are not supplied in this part.
i32 theftScore(Ctx& ctx, i32 thiefLevel, i32 thiefCoefficient, bool defending);

// R36 gold theft is attempted when score - 2*M < 50; pure
bool theftGoldAttempted(i32 score, i32 monsterGuildLevel);

// R36 item theft slot draw, 2..40. Slot 40 is outside the pack (pack size is 40).
i32 theftItemSlot(Ctx& ctx);

// R37 archive acid or electrocution damage (fallback only, lowest precedence):
// minimum = (ln(M) + M/15) * 4.55; base = minimum + Random[0..minimum];
// -0.7 damage every 10 character levels; final = base * (1 - resistPercent/2)
// UNKNOWN: archive resistance scale; resistPercent is used as given in the archive.
i32 archiveSpecialDamage(Ctx& ctx, i32 monsterGuildLevel, i32 characterLevel, i32 resistPercent);

// R37 archive breath damage (fallback only): LevelMod = M*ln(M); minimum = Int((ln(LevelMod) + LevelMod/15) * 1.1375);
// base = minimum + Random[0..2*minimum]; final = base * (1 - resistPercent/2)
// CONFLICT: audit form R28 takes precedence.
i32 archiveBreathDamage(Ctx& ctx, i32 monsterGuildLevel, i32 resistPercent);

// R37 archive stone chance percent (minimum 5): (100 + SpawnLevel) - CON*ln(CON)*1.09
i32 archiveStoneChance(i32 spawnLevel, i32 con);

// R37 archive paralysis chance percent (minimum 5): 100 - (Resistance + CON)
i32 archiveParalysisChance(i32 resistPercent, i32 con);

// R37 archive aging and drain chance percent: (100 + 2*SpawnLevel) - 2.5*CON
i32 archiveAgeChance(i32 spawnLevel, i32 con);

}  // namespace mordor
