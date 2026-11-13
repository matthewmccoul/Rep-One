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

// R2 Good and Evil cannot share a party; Neutral may travel with either.
// Equivalent to the quick-reference |a - b| > 1 given Good=0, Neutral=1, Evil=2.
bool oppositelyAligned(Align a, Align b);

// R1 party holds at most kPartyMaxSize characters (Size >= Party.MaxSize)
bool isPartyFull(const Party& party);

// R4 join preconditions (R4.1-R4.3), password (R6), alignment (R7), dead joiner (R8).
// Emits the R4.2, R4.3, R6, R7 and R8 messages through ctx.say; R4.1 is silent.
// Input passwordMatches is the caller's comparison of the entered password (R6).
JoinCheck checkJoin(Ctx& ctx, const Party& party, const Character& joiner,
                    bool inDungeon, bool inCombat, bool passwordMatches);

// R5 candidate: living or dead character at the leader's place, not already in the party
bool isJoinCandidate(const Party& party, const Character& c, bool inCity);

// R5 SamePlace: both in the city, or same dungeon level and square
bool samePlace(const Party& party, const Character& c, bool bothInCity);

// R5 dungeon join list: on file, not in the party, on the current floor and area; dead included
bool isOnJoinList(const Party& party, const Character& c, i32 currentArea);

// R7 some current member is oppositely aligned to the joiner
bool hasOppositeMember(const Party& party, Align joinerAlignment);

// R9 join effects: newcomer takes the leader's square, copies the three lost flags from member 0,
// sets hit and spell points to maximum, joins as the last member, runs companionCarryRun,
// then prunes the dead (R13 join prune trigger).
// UNKNOWN: "Key a" path (fn04CD load, fn16B5 call, no sub0930) is a UI route in another module.
void performJoin(Ctx& ctx, Party& party, Vec<Companion>& companions, Character& joiner);

// R10 leave: refuse in dungeon with monsters present, refuse the leader, otherwise on confirmation
// remove the member, save it at its current level and square, prune the dead. No message on removal.
LeaveResult leaveParty(Ctx& ctx, Party& party, i32 slot,
                       bool inDungeon, bool monstersPresent, bool confirmed);

// R11/R12 pruning step for one member at index: keeps able members; otherwise sets hits to 0,
// announces "<name>" has died! when the member's level equals the leader's, marks dead if not
// already dead, saves and removes it, and moves all remaining members to the leader's level, X and Y.
// Returns true when a member was removed. The caller (pruneTheDead, already declared) loops
// and does not advance the index after a removal (R12.3).
bool pruneOneMember(Ctx& ctx, Party& party, i32 index);

// R13 teleporter and chute prune only if the party size dropped
bool pruneOnSizeDrop(i32 sizeBefore, i32 sizeAfter);

// R14 during the monsters' turn after the characters' turns: if any member is dead, exactly one
// UPDATE fires and its callback removes the dead. Returns true when the UPDATE fired.
bool firePartyUpdateIfDead(Ctx& ctx, Party& party);

// R15 wiped party: size > 1 prunes to one member; size 1 runs sub0930 (slot 0)
void handleWipedParty(Ctx& ctx, Party& party, Vec<Companion>& companions);

// R16 each monster group picks one party member by a weighted roll using ctx.rng.
// Returns the member index, or -1 when no member can be picked.
// UNKNOWN: weight-sequence indexing (kTargetWeights against member index) unresolved; using index i.
i32 pickMonsterTarget(Ctx& ctx, const Party& party);

// R17 the picked character reached 0 hits: sub0930 drops companions, the list is pruned,
// then the group turn stops if one member remains, or the monster turn ends if a lone character died.
GroupTurnStep afterTargetKnockedOut(Ctx& ctx, Party& party, Vec<Companion>& companions,
                                    i32 targetIndex);

// R18 disband to the main menu: refusal while monsters stand (unless the character is dead),
// whether the reload question is asked (party of more than one), dead leader suppresses the record.
// Emits the reload question through ctx.say.
DisbandPlan planDisband(Ctx& ctx, const Party& party, bool monstersStanding);

// R18.2-R18.5 record instruction for the caller to perform after the reload answer.
// Lone leader: BlankRecord with no question (R18.3). Dead leader: None (R18.5).
// UNKNOWN: engine source is silent on the lone case; blanking is taken as the stated behaviour.
RecordAction resolveDisband(const Party& party, bool reloadWhenLoaded);

// R19 autoload when the leader loads. Leader dead: nobody restored (R19.1). Each remembered member
// other than the leader is checked in order: not here (R19.3), dead (R19.4), opposite (R19.5),
// load failed (R19.2), otherwise joins, announces, and runs companionCarryRun (R19.6).
// Every line is prefixed "AUTOLOAD PARTY: " in its ctx.say text (R19.7).
AutoloadResult autoloadParty(Ctx& ctx, Party& party, Vec<Companion>& companions,
                             const Vec<RememberedMember>& remembered);

// R20.1 give is available only when the party has more than one member.
// Emits "Nobody in party to give to!" through ctx.say when alone.
bool giveAvailable(Ctx& ctx, const Party& party);

// R20.5 gold transfer. Amount < 1 gives 0 silently; amount above giver's on-hand gold is refused
// with "Not enough gold!". On success emits "Give of N gold to "X" Completed!".
// UNKNOWN: the gold answer parsing and item answer rules live in Tables, which are not in this part;
// GoldAnswer (items3a) owns parsing. Item transfer (R20.6-R20.7) is delegated to the items module.
// FIXED: the original name giveGold collided with a declaration elsewhere in the include chain that has
// the same parameter list and a different return type. C++ cannot overload on return type alone, so this
// declaration is renamed to giveGoldTransfer. The meaning and signature are otherwise unchanged. Callers
// of this R20 routine must use the new name; the other giveGold declaration is left untouched.
GiveGoldOutcome giveGoldTransfer(Ctx& ctx, Character& giver, Character& receiver, Currency amount);

// R22 pool gold (ALT-p): each other member's on-hand gold moves to the selected character.
// Logs "Pool completed" when the party has two or more members, then runs checkWealthiest.
// UNKNOWN: Formulas section is absent; mod replacement with Bank Party Pool & Deposit is not modelled.
PoolOutcome poolGold(Ctx& ctx, Party& party, i32 selectedIndex, bool inCombat);

// R22.4 CheckWealthiest.
// UNKNOWN: body and owning module are not described in this part; declared because poolGold calls it.
void checkWealthiest(Ctx& ctx, Party& party);

// R23 available gold for an exchange: dungeon uses on-hand gold, city uses purse (on-hand plus bank)
Currency exchangeAvailable(const Character& c, bool inDungeon);

// R23 exchange gold transfer (FRMEXCHA). Zero returns silently; above available is refused with
// "Too Much!". Any on-hand deficit comes out of the bank. Negative amounts are NOT rejected and run
// the transfer backwards (quirk, kept as stated).
GoldTransferOutcome transferExchangeGold(Ctx& ctx, Character& from, Character& to,
                                         Currency amount, bool inDungeon);

// R24 bank party options (Party Pool, Party Deposit, Party Pool & Deposit). Silent.
// On overflow all balances touched are rolled back.
// UNKNOWN: overflow condition is not stated; treated as i64 overflow of any balance touched.
BankPartyOutcome bankPartyOption(Ctx& ctx, Party& party, BankPartyOption option);

// R25.1 parse swap input: must contain a comma, be at most kSwapInputMaxChars characters,
// and parse as exactly two integers. Otherwise ok is false and the refusal is silent.
SwapInputResult parseSwapInput(const Str& text);

// R25 swap items (fn1521): parses the input, calls swapSlots (already declared, slot 1 refused there),
// and on success emits "Item swap completed!". Works in town or dungeon.
bool runItemSwap(Ctx& ctx, Character& c, Inventory& inv, const Str& text, bool buffersFollow);

// R26 swap two party positions and their standing orders (combatOrders, itemOrders).
// The selected index follows the moved character. NowLeader is announced only if index 0 is involved.
// In the dungeon during combat it is refused with "You cannot rearrange the party during combat!".
PartySwapOutcome swapPartyPositions(Ctx& ctx, Party& party, i32 a, i32 b,
                                    bool inDungeon, bool inCombat);

// R27.1-R27.2 next command index: (SelectedIndex + 1) mod member count. Pure.
i32 nextCommandIndex(const Party& party);

// R27.3-R27.4 switch commanded character. Switching to the current index does nothing.
// Otherwise the flags describe what the caller must refresh; in town (leader alive, level 0)
// Store, Confinement, Morgue, Guild and Bank update. Seer does not update.
SwitchResult switchCommandedCharacter(Ctx& ctx, Party& party, i32 targetIndex,
                                      SwitchKind kind, bool inTown);

// R28 Alt+1..4 switches command when size is greater than 1 and the slot exists.
// digit is 1..kCommandKeySlots.
bool altDigitSwitchesCommand(const Party& party, i32 digit);

// R29 on entering a room (sub0A91) and after the party is read: each member with no order
// (combatOrders[i].kind == None) takes their Combat Setting, dead or alive, if a hostile group has engaged.
// UNKNOWN: the Combat Setting is taken to be Character::order; the source is not stated in this part.
void applyRoomEntryCombatSettings(Ctx& ctx, Party& party, bool hostileEngaged);

// R30.1 City Pickup. Refused silently if the owning character is dead or paralysed.
// A hidden target is refused with a message before anything is put down. A carrier already carrying
// someone puts them down first, with a message.
// UNKNOWN: whose state (carrier or target) gates the silent refusal; the carrier is checked here.
PickupOutcome pickUpBody(Ctx& ctx, Party& party, Character& carrier, Character& target,
                         bool targetHidden);

// R30.2 Drop Body. Does nothing if nobody is carried; refuses silently only when paralysed.
BodyDropOutcome dropBody(Ctx& ctx, Character& carrier);

// R30.3 Morgue Grab Body: no Hide test, does not put down the current carry. Returns true when taken.
bool morgueGrabBody(Ctx& ctx, Party& party, Character& target);

// ---- dependencies named by this part, not described here ----

// R9 / R19.6 companion carry run (called after a join and after an autoload join).
// UNKNOWN: owning module is not stated in c17p1; declared because performJoin and autoloadParty call it.
void companionCarryRun(Ctx& ctx, Party& party, Vec<Companion>& companions);

// ---- quick-reference formulas ----

// Quick reference: help cry when CurHits < kHelpCryHitsBelow OR CurHits <= MaxHits * kHelpCryFraction.
bool helpCryNeeded(const Vitals& v);

}  // namespace mordor
