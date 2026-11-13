#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_game.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants (SRD c06p1). Existing clock intervals are in state_game.h:
// kCombatIntervalMs (1000), kPlayIntervalMs (250), kAilmentIntervalMs (4000),
// kMeleeWaitMs (250), kMidnightWrapSeconds (86340), kSecondsPerDay (86400).

// R26 free-attack threshold: floor applies on moves and level change.
inline constexpr i32 kFreeAttackFloor = 5;
// R22 free-attack threshold: cap applies on level change (ChangedLevel) only.
// CONFLICT: community source marks 98 as uncertain ("maximum 98?"); audit/fidelity prevail.
inline constexpr i32 kFreeAttackCap = 98;
// Formulas: threshold = max(5, 50 + 5*(monsterDex - charDex)).
inline constexpr i32 kFreeAttackBase = 50;
inline constexpr i32 kFreeAttackPerDex = 5;

// R19 hostile roll: T = trunc(base - 7.5 * ln(max(1, cha))); draw 1..100, true if draw < T.
inline constexpr double kHostileLnCoef = 7.5;
inline constexpr i32 kHostileRollMax = 100;

// R27 weights down the party: 100, 50, 25, 12, 6.
// UNKNOWN: mapping of these weights to the four monster groups or party slots.
inline constexpr i32 kGroupWeights[5] = {100, 50, 25, 12, 6};

// R19 hostile base table. Rows: monster alignment (Good, Neutral, Evil).
// Columns: character alignment (Good, Neutral, Evil).
inline constexpr i32 kHostileBase[3][3] = {
    {0, 15, 100},
    {25, 15, 25},
    {35, 35, 25},
};

// R10 CombatSetting mapping (SRD Tables).
inline constexpr i32 kSettingFight = 1;
inline constexpr i32 kSettingDefend = 2;
// Settings 3 or more select buffer (setting - 3).
// CONFLICT: SettingNames labels buffer 10 as "0"; its number is stored as 13.
inline constexpr i32 kSettingBufferBase = 3;

// P1 timers: form designed combat timer at 1500 ms; code resets to kCombatIntervalMs.
// CONFLICT: 1500 (form design) vs 1000 (code); operative value is kCombatIntervalMs.
inline constexpr i32 kFormCombatIntervalMs = 1500;
// R31 port melee hold is kMeleeWaitMs per side.
// CONFLICT: VB3 trace describes a 500 ms hold per group result; kept as reference only.
inline constexpr i32 kMeleeHoldTraceMs = 500;

// ---- Result structs

// R10 / R14 / Tables: outcome of mapping a CombatSetting to a standing order.
struct OrderChoice {
    CombatOrderKind kind = CombatOrderKind::None;
    i32 spellId = 0;
    // A pack item in a buffer gives Use; the caller handles the Use command.
    // UNKNOWN: which caller handles the Use command.
    bool useItem = false;
};

// ---- R1-R8 Clock and round structure

void combatTick(Ctx& ctx, GameState& game);                 // R1 one round below ground per combat tick; R5 guard against overlap
void playerRound(Ctx& ctx, GameState& game);                // R2 each living character then companions; R4 stops when no foes remain
void characterAct(Ctx& ctx, GameState& game, Character& who); // R6 retarget then standing order unless paralysed
// UNKNOWN: companion storage is not in shared state; implementer locates companions of owner.
void companionsAct(Ctx& ctx, GameState& game, Character& owner); // R6 runs companions' actions, even when owner is paralysed
bool roundGuard(const Ctx& ctx, const GameState& game);     // R4 fn0EEF: false when no monsters remain, so the round exits
void noteDeadAndSendUpdate(Ctx& ctx, GameState& game);      // P2.4 note dead members; send party list update (modUpdateCharacterList)
// UNKNOWN: "chest tick" in P2.2a (meaning unknown); not declared here.

// R7 / R28-R30 play clock. Time unit of nowSeconds follows the 86340/86400 formula.
// CONFLICT: PlaySeconds is rounded to 4 dp per formula; Character::playSeconds is i64.
void playClockTick(Ctx& ctx, Vec<Character>& members, bool paused, i64 nowSeconds); // R28 PlayClock.Shared.Tick
void ailmentTick(Ctx& ctx, GameState& game);                // R8 ailment timer, guarded by inAilment
void clockSetup(Ctx& ctx, GameState& game);                 // P1 start-up: combat 1000 ms, ailment 4000 ms, play 250 ms

// ---- R9-R15 Standing orders

void startFight(Ctx& ctx, GameState& game);                 // R10 set standing orders from CombatSetting; R11 roll no initiative
// UNKNOWN: SRD "Spell" order is represented by CombatOrderKind::Buffer (no Spell kind in shared state).
// UNKNOWN: buffer "holds spell idx" test (assumed spellBuffers value >= 0); SpellDef id field name not visible.
OrderChoice orderFromSetting(const Ctx& ctx, i32 combatSetting, const Arr<i32, kSpellBuffers>& buffers); // R10 CombatSetting table
void endFight(Ctx& ctx, GameState& game);                   // R10 clear standing orders; CombatSetting unchanged
void checkCast(Ctx& ctx, Character& who);                   // R12 Fight and Defend become None; queued spell kept
void openSpellList(Ctx& ctx, Character& who);               // R13 Att = false; Fight and Defend become None; queued spell kept
void attReorder(Ctx& ctx, Character& who, i32 delta);       // R14 Sub102F: Att -1 reorder; Kind = Defend if None/Fight, else Fight; spell dropped
void retarget(Ctx& ctx, GameState& game, Character& actor); // R15 if aimed group has no standing monsters, aim at first standing group
i32 standingGroupIndex(const Ctx& ctx, const GameState& game, i32 delta); // R15 fn0DCD: index of first standing monster group

// ---- R16-R26 Monster attacks and engagement

void engagementTick(Ctx& ctx, GameState& game);             // R16-R18 encounter timer: hostile waiting attacks; peaced with join offer stays
i32 hostileBase(CharAlign monsterAlign, Align charAlign);   // R19 hostile base table lookup (rows monster, columns character)
bool hostileRoll(Ctx& ctx, i32 base, i32 cha);              // R19 T = trunc(base - 7.5*ln(max(1,cha))); true if draw 1..100 < T
// UNKNOWN: effective CHA derivation is not given; caller supplies cha.
i32 freeAttackThreshold(i32 monsterDex, i32 charDex, bool cap); // R22/R26 max(5, 50 + 5*(monsterDex - charDex)); capped at 98 when cap set
void monstersFreeAttack(Ctx& ctx, GameState& game, i32 caller); // R25/R26 sub0A64: caller 0 attacks with no roll; clamp per caller
void freeRound(Ctx& ctx, GameState& game);                  // R25 clock stops, monstersFreeAttack runs once, clock restarts
// FIXED: MoveArg is declared nowhere in the project headers; replaced by i32.
void onMove(Ctx& ctx, GameState& game, i32 arg);            // R21-R24 Moved uncapped; ChangedLevel capped at 98; same-area none

// ---- R31-R34 Display timing (port)

void meleeHold(Ctx& ctx, i32 ms);                           // R31 display wait on each side of a melee result
void endCombat(Ctx& ctx, GameState& game);                  // R34 key pressed while a won round is shown; keys in a hold are queued

}  // namespace mordor
