#pragma once
// Module partya: SRD c17p1 "The Party" (R1-R30).
// R21 (naming another character in the interact box) is player text-screen behaviour: out of scope.
// CONFLICT: the wiki says up to nine active characters; R1 and kPartyMaxSize say 4. Using 4.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants and tables ----
// R3 the leader is always member index 0
inline constexpr i32 kLeaderIndex = 0;
// R28 command keys Alt+1..Alt+4 address slots 1..4
inline constexpr i32 kCommandKeySlots = 4;
// FIXED: kSwapInputMaxChars (R25: at most 5 characters) is already defined in this translation unit;
// the duplicate definition was removed so the existing one is used.
// Quick reference: help cry when CurHits < 10 OR CurHits <= MaxHits * 0.1
inline constexpr i32 kHelpCryHitsBelow = 10;
inline constexpr double kHelpCryFraction = 0.1;
// R16 monster target weights: 100, 50, 25, 12, 6, 3, 1, 0
// UNKNOWN: weight-sequence indexing against party member index is unresolved (see R16 / Unknowns).
inline constexpr i32 kTargetWeightCount = 8;
inline constexpr i32 kTargetWeights[kTargetWeightCount] = {100, 50, 25, 12, 6, 3, 1, 0};

// ---- result types ----

// R4/R6/R7/R8 join refusal reasons
enum class JoinRefusal : i32 {
    None = 0,
    LeaderDead,          // R4.1 refused silently
    DeadInCombat,        // R4.2 dead joiner in dungeon fight
    PartyFull,           // R4.3 Size >= Party.MaxSize
    WrongPassword,       // R6
    OppositeAlignment,   // R7
    DeadNoPendingRaise   // R8
};

struct JoinCheck {
    JoinRefusal refusal = JoinRefusal::None;
    // R4.2 a living joiner is set up with sub0BDE (body not described in this part; caller runs it)
    bool runSub0BDE = false;
};

// R10 leave outcomes
enum class LeaveOutcome : i32 {
    Left = 0,
    RefusedMonstersPresent,  // R10.1
    RefusedLeader,           // R10.2
    NotConfirmed             // R10.3 no effect until confirmed
};

struct LeaveResult {
    LeaveOutcome outcome = LeaveOutcome::NotConfirmed;
    Character leaver{};      // the removed member, saved at its current level and square (R10.3)
};

// R19 per-member autoload outcomes
enum class AutoloadMemberOutcome : i32 {
    Joined = 0,
    NotHere,        // R19.3
    Dead,           // R19.4
    Opposite,       // R19.5
    LoadFailed      // R19.2
};

// One remembered member from the saved party record. The caller performs the file read;
// loadOk is false when that member's load failed (R19.2).
struct RememberedMember {
    Character character{};
    bool loadOk = true;
};

struct AutoloadResult {
    Vec<AutoloadMemberOutcome> outcomes{};
    i32 joined = 0;
    bool leaderDead = false;   // R19.1 nobody restored
};

// R18 record instruction; the caller performs the actual file operation (core never touches files)
enum class RecordAction : i32 {
    None = 0,
    WriteRecord,     // Yes: writes party-<leader>.json
    DeleteRecord,    // No: deletes the leader's file if one exists
    BlankRecord      // lone leader: blanked without a question (R18.3)
};

struct DisbandPlan {
    bool refused = false;     // R18 q/Exit refused while monsters stand (unless dead)
    bool askReload = false;   // R18.1 ask the reload question (party of more than one)
    bool leaderDead = false;  // R18.5 a dead leader's party record is not written
};

// R20 gold give outcomes
enum class GiveGoldOutcome : i32 {
    Done = 0,
    ZeroNoOp,        // R20 / gold answer: Val < 1 gives 0
    NotEnoughGold    // gold answer: above on-hand gold gives "Not enough gold!"
};

// R22 pool outcomes
enum class PoolOutcome : i32 {
    Pooled = 0,
    RefusedAlone,       // R22.1
    RefusedInCombat     // R22.2
};

// R23 exchange gold transfer outcomes
enum class GoldTransferOutcome : i32 {
    ZeroNoOp = 0,   // R23.1 returns silently
    Done,
    TooMuch         // R23.3
};

// R24 bank party options
enum class BankPartyOption : i32 {
    PartyPool = 0,
    PartyDeposit,
    PartyPoolAndDeposit
};

enum class BankPartyOutcome : i32 {
    Done = 0,
    RolledBack      // R24 overflow rolls back every balance touched
};

// R25 parsed swap input
struct SwapInputResult {
    bool ok = false;
    i32 first = 0;
    i32 second = 0;
};

// R26 party position swap outcomes
enum class PartySwapOutcome : i32 {
    Swapped = 0,
    RefusedInCombat,   // R26.4
    InvalidIndex
};

// R27 command switch kinds
enum class SwitchKind : i32 {
    Tap = 0,   // single tap: changes command target, does not switch the full view (mdrmulSwitch)
    Full       // double tap, Switch To, or Alt+S
};

struct SwitchResult {
    bool changed = false;        // false when switching to the character already in command
    bool resetSpellType = false; // R27.4 Spell Type box resets to the first class
    bool townRefresh = false;    // R27.4 in town: Store, Confinement, Morgue, Guild and Bank update
};

// R17 result of a knocked-out target during a group turn
enum class GroupTurnStep : i32 {
    Continue = 0,
    GroupStops,       // R17 one member left: the group's turns stop
    MonsterTurnEnds   // R17 a lone character's death ends the whole monster turn
};

// R30 pick-up outcomes
enum class PickupOutcome : i32 {
    Picked = 0,
    RefusedSilent,      // R30.1 dead or paralysed
    RefusedHidden,      // R30.1 hidden target, refused with a message
    PutDownThenPicked   // R30.1 carrier put down the current carry first, with a message
};

enum class BodyDropOutcome : i32 {
    Dropped = 0,
    NothingCarried,     // R30.2 does nothing
    RefusedSilent       // R30.2 paralysed
};

// ---- inline trivial queries ----

// R12 member stays when able: alive, hits above 0, not stoned (Stoned is a DeathState, so isDead covers it)
inline bool isAble(const Character& c) {
    return !c.isDead() && c.vitals.curHits > 0;
}

// ---- rule routines ----














// R16 each monster group picks one party member by a weighted roll using ctx.rng.
// Returns the member index, or -1 when no member can be picked.
// UNKNOWN: weight-sequence indexing (kTargetWeights against member index) unresolved; using index i.
i32 pickMonsterTarget(Ctx& ctx, const Party& party);














// R26 swap two party positions and their standing orders (combatOrders, itemOrders).
// The selected index follows the moved character. NowLeader is announced only if index 0 is involved.
// In the dungeon during combat it is refused with "You cannot rearrange the party during combat!".
PartySwapOutcome swapPartyPositions(Ctx& ctx, Party& party, i32 a, i32 b,
                                    bool inDungeon, bool inCombat);








// ---- dependencies named by this part, not described here ----


// ---- quick-reference formulas ----

// Quick reference: help cry when CurHits < kHelpCryHitsBelow OR CurHits <= MaxHits * kHelpCryFraction.


}  // namespace mordor
