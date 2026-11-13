#pragma once
// partyb: party management rules (SRD part c17p1).
// Rule numbers R1..R37 are local to this part, assigned in source order.
// Already declared in the API INDEX and only called here, not redeclared:
//   pruneTheDead, setDead, sub0930 (status1a/status1b), paralyse (status2),
//   maxSpellPoints (casting2/casting3), swapSlots (items2b).
// Out of scope (text screens, file IO, UI refresh, message boxes):
//   name and password prompts, typed-number parsing (Val/CLng, max lengths),
//   party file binary layout, AUTOLOAD/SAVE prompts, party position label.
// Give text parsing is reduced to the slot formula (R30); the amount and slot are
// passed in already parsed.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants and tables ----

// R1 help cry thresholds: Needed = CurHits < 10 OR CurHits <= MaxHits * 0.1
inline constexpr i32 kHelpCryHitsAbsolute = 10;
inline constexpr double kHelpCryHitsFraction = 0.1;

// R2 monster target weights by party position (front first), halving thereafter
inline constexpr i32 kMonsterTargetWeightCount = 8;
inline constexpr i32 kMonsterTargetWeights[kMonsterTargetWeightCount] = {100, 50, 25, 12, 6, 3, 1, 0};
inline constexpr i32 kMonsterTargetRollSides = 100;

// R22 combat setting values: 0 Nothing, 1 Fight, 2 Defend, 3 + k Buffer k
inline constexpr i32 kCombatSettingNothing = 0;
inline constexpr i32 kCombatSettingFight = 1;
inline constexpr i32 kCombatSettingDefend = 2;
inline constexpr i32 kCombatSettingBufferBase = 3;

// R23 party status codes
inline constexpr char kStatusPoisoned[] = "Po";
inline constexpr char kStatusDiseased[] = "Di";
inline constexpr char kStatusParalysed[] = "Pa";
inline constexpr char kStatusOk[] = "OK";

// R25 autoload alignment gap: clash when |a - b| > 1 (CONFLICT, see R25)
inline constexpr i32 kAutoloadAlignGap = 1;

// R30 Give slot: slot = (n > 1) ? n - 1 : 0; slot 0 is the Hands slot
inline constexpr i32 kGiveHandsSlot = 0;

// R16 / R37 swap and command-switch bounds
// UNKNOWN: swap slot maximum is mc0190; 40 inferred from PackSize (kPackSlots). Not confirmed.
inline constexpr i32 kSwapSlotMax = kPackSlots;
// CONFLICT: Alt+1..4 in dungeon, Alt+0 in city; index base unknown (R37)
inline constexpr i32 kCommandSwitchSlotsMax = kPartyMaxSize;

// ---- result enums ----

// R3 joinParty outcome; checks run in SRD order (see R3)
enum class PartyJoinOutcome : i32 {
    Joined = 0,
    Full,          // "Your party is too full!"
    AlreadyInParty,// "Character is already in Party!"
    Dead,          // "Character is Dead!"
    NotHere,       // not SamePlace as leader
    Opposed,       // "... is oppositely aligned ..."
    LeaderDead,    // first member dead (message UNKNOWN)
    DeadInCombat   // "Cannot raise character in Combat!"
};


// R8-R12 pool and deposit kinds
enum class PartyPoolKind : i32 {
    ToSelected = 0, // PoolGold: all members to Selected, on-hand
    ToBank,         // PoolToBank
    PartyDeposit,   // PartyDeposit: each member to own bank
    PoolAndDeposit  // PoolAndDeposit
};

// R12 pool command outcome
enum class PartyPoolOutcome : i32 {
    Completed = 0,
    NobodyToPoolTo, // "Nobody else in party to Pool to!"
    InCombat,       // "Cannot pool gold while in combat!"
    OverflowStopped // Completed but a pool step stopped on overflow (see R8)
};

// R29 give gold outcome
enum class PartyGiveGoldOutcome : i32 {
    Given = 0,
    NothingToGive,  // amount < 1
    NotEnoughGold,  // "Not enough gold!"
    NobodyToGiveTo  // "Nobody in party to give to!"
};

// R31 give item outcome
// UNKNOWN: overlaps items1a GiveOutcome / items2b GiveResult; kept local for this part
enum class PartyGiveItemOutcome : i32 {
    Given = 0,
    NoItemToGive,   // "No Item to give!" (slot 0, Hands)
    Equipped,       // "Item is Equipped!"
    NobodyToGiveTo  // "Nobody in party to give to!"
};

// R32 leave party outcome
enum class PartyLeaveOutcome : i32 {
    Left = 0,
    LeaderCannotLeave, // "Leader cannot leave Party!"
    MonstersInArea,    // "X cannot leave while monsters are in the area!"
    NotConfirmed
};

// R16 rearrange outcome
enum class RearrangeOutcome : i32 {
    Swapped = 0,
    InCombat,   // "You cannot rearrange the party during combat!" (dungeon only, see CONFLICT R16)
    OutOfRange,
    SameSlot
};

// R24 autoload join outcome
enum class AutoloadOutcome : i32 {
    Joined = 0,
    NotHere,   // "<name>" is not here anymore!
    Dead,      // "<name>" is dead!  Can't join!
    Opposed,
    Full,
    LeaderDead,
    Error      // "An error occurred auto-loading ..."
};

// R26 city join outcome
enum class CityJoinOutcome : i32 {
    Joined = 0,
    LeaderDead,
    Full,           // "Your party is too full!"
    WrongPassword,  // "Incorrect Password!"
    Opposed,
    DeadNotPending, // "X is Dead! ..."
    NotHere,        // not SamePlace as leader
    AlreadyInParty
};

// R34 default options outcome
enum class DefaultOptionsOutcome : i32 {
    Applied = 0,
    InCombat,      // "Cannot set default options while in Combat!"
    MonstersInArea // "...monsters are in the area!"
};

// R35 disband: host action for the party file (core does no IO)
enum class PartySaveAction : i32 {
    None = 0,
    SaveParty,        // remember = Yes: write party file
    DeleteSavedParty  // remember = No: delete party file
};

// ---- rules ----




// R2 pick the target index for a monster group. roll = ctx.rng.below(100) + 1,
// walks the weights, clamps to partySize-1.
// UNKNOWN: whether acc / total * 100 is integer or floating point (chunk 14)
i32 pickMonsterTarget(Ctx& ctx, i32 partySize);






// R8 pool all members' on-hand gold into the Selected member. Overflow ends the loop
// silently with earlier members already pooled. Returns false if overflow stopped it.
bool poolGoldToSelected(Ctx& ctx, Party& party);






















// R29 give gold to a member. amount < 1: NothingToGive. amount > giver on-hand: NotEnoughGold.
// Otherwise giver.onHand -= amount, receiver.onHand += amount.
PartyGiveGoldOutcome giveGoldToMember(Ctx& ctx,
                                      Party& party,
                                      Character& giver,
                                      i32 receiverIndex,
                                      Currency amount);


// R31 give item to a member. packSlot is the value from itemSlotIndex.
// kGiveHandsSlot refuses NoItemToGive; equipped refuses Equipped.
PartyGiveItemOutcome giveItemToMember(Ctx& ctx,
                                      Party& party,
                                      Character& giver,
                                      i32 receiverIndex,
                                      i32 packSlot);







}  // namespace mordor
