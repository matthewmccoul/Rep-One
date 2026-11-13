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

// R1 help cry needed: CurHits < 10 OR CurHits <= MaxHits * 0.1
bool helpCryNeeded(const Character& c);

// R2 weight of a party position (0-based, front = 0); 0 past the table
i32 monsterTargetWeight(i32 position);

// R2 total of weights over the party positions 0..partySize-1
// UNKNOWN: whether weight is indexed by position per member (chunk 14)
i32 monsterTargetTotal(i32 partySize);

// R2 pick the target index for a monster group. roll = ctx.rng.below(100) + 1,
// walks the weights, clamps to partySize-1.
// UNKNOWN: whether acc / total * 100 is integer or floating point (chunk 14)
i32 pickMonsterTarget(Ctx& ctx, i32 partySize);

// R3 join party (Add). Checks in order: empty party joins directly; count >= 4 Full;
// name clash AlreadyInParty; dead Dead; !samePlace NotHere; opposed Opposed.
// With inCombat: first member dead LeaderDead; joiner dead DeadInCombat;
// living joiner gets setupJoinerInCombat (R33). On Joined, inheritLostFlags (R7) applies.
PartyJoinOutcome joinParty(Ctx& ctx, Party& party, Character& newcomer, bool inCombat);

// R4 opposed alignment: (Good, Evil) or (Evil, Good)
bool opposedAlignment(Align a, Align b);

// R5 same place: both on dungeon level 0, or same level, X and Y
bool samePlace(const Character& a, const Character& b);

// R6 here-to-join list: roster indices not in party, with dungeonLevel == partyLevel,
// 1 <= partyLevel <= floorCount, and location.area == area.
// UNKNOWN: savedArea(c) is not defined in the SRD; read as c.location.area.
Vec<i32> hereToJoinCandidates(const Party& party,
                              const Vec<Character>& roster,
                              i32 partyLevel,
                              i32 floorCount,
                              i32 area);

// R7 newcomer inherits the first member's lost flags (position, level, facing)
void inheritLostFlags(Ctx& ctx, const Character& first, Character& newcomer);

// R8 pool all members' on-hand gold into the Selected member. Overflow ends the loop
// silently with earlier members already pooled. Returns false if overflow stopped it.
bool poolGoldToSelected(Ctx& ctx, Party& party);

// R9 pool all members' on-hand gold into the Selected member's bank (all members, Selected included)
bool poolGoldToBank(Ctx& ctx, Party& party);

// R10 each member deposits own on-hand gold into own bank
bool partyDepositGold(Ctx& ctx, Party& party);

// R11 pool banks and on-hand into Selected's bank, then Selected's on-hand into its bank
// UNKNOWN: overflow behaviour; original has no error handler (port rolls back, see deviations)
bool poolAndDepositGold(Ctx& ctx, Party& party);

// R12 pool command: refuse in combat; alone refuses NobodyToPoolTo; otherwise run the
// requested kind, log "Pool completed" with two or more members, then checkWealthiest (R13).
PartyPoolOutcome poolPartyGold(Ctx& ctx, Party& party, PartyPoolKind kind, bool inCombat);

// R13 checkWealthiest after a pool; behaviour not in the source chunks
// UNKNOWN: CheckWealthiest body is not stated in the SRD excerpt.
void checkWealthiest(Ctx& ctx, Party& party);

// R14 exchange gold between two characters. amount == 0 is a no-op. Available is on-hand
// in the dungeon, purse otherwise. Amount above available: TooMuch.
// Transfers onto `to`, debits `from`, and moves any negative on-hand into the bank.
// CONFLICT: negative amounts run backwards with no check (original quirk, kept).
// FIXED: GoldTransferOutcome names no type in any header; i32 used as the outcome code.
i32 transferGold(Ctx& ctx,
                 Character& from,
                 Character& to,
                 Currency amount,
                 bool inDungeon);

// R15 remove a named member. False if not in party or Members.Count == 1.
// Clamps SelectedIndex to [0, Count - 1] after removal.
bool removeMember(Ctx& ctx, Party& party, const Str& name);

// R16 rearrange: swap two member slots. Refuses in dungeon combat. Refuses slot out of range.
// SelectedIndex follows the swapped member; announce NowLeader if a or b is 0.
// CONFLICT: combat refusal is dungeon-only per port comment; original rule not stated.
RearrangeOutcome rearrangeParty(Ctx& ctx, Party& party, i32 a, i32 b, bool inCombat);

// R18 select member by slot. Returns false when slot is already selected or invalid.
bool selectMember(Ctx& ctx, Party& party, i32 slot);

// R19 select the next able member after SelectedIndex (wraps). Able = not dead, not stoned,
// CurHits > 0. Returns false if none is able.
bool selectNextAble(Ctx& ctx, Party& party);

// R20 cycle selection to the next member when size > 1 (wraps; same for dungeon and city)
void cycleSelectedMember(Ctx& ctx, Party& party);

// R21 for each member whose order.kind is None, apply the default order; keep target
// UNKNOWN: ApplyDefault body is not in the SRD excerpt.
void ensureDefaultCombatOrders(Ctx& ctx, Party& party);

// R22 combat setting value to order kind (0 None, 1 Fight, 2 Defend, >= 3 Buffer)
CombatOrderKind combatSettingKind(i32 setting);

// R22 buffer index k for a setting value >= 3 (k = setting - 3); -1 otherwise
i32 combatSettingBufferIndex(i32 setting);

// R22 inverse of the mapping above; bufferIndex used only for Buffer
i32 combatSettingFromKind(CombatOrderKind kind, i32 bufferIndex);

// R23 party status code: "Po" if poisoned, "Di" if diseased, "Pa" if paralysed; "OK" if none
// UNKNOWN: poisoned = ailments.poisonLevel > 0, diseased = ailments.diseaseLevel > 0.
Str partyStatusCode(const Character& c);

// R24 autoload join for a saved member. stillHere is the SamePlace/candidate result.
// Checks in order: leader dead LeaderDead; NotHere; dead Dead; opposed Opposed; full Full.
AutoloadOutcome autoloadMember(Ctx& ctx, Party& party, Character& candidate, bool stillHere);

// R25 autoload alignment clash: integer |a - b| > kAutoloadAlignGap
// CONFLICT: Opposed (Good vs Evil only) and integer difference agree only if the enum is
// ordered Good, Neutral, Evil. Align is ordered that way here. Both kept per the SRD.
bool autoloadAlignmentClash(Align a, Align b);

// R26 join from city (Ask to Join). Checks in order: leader dead; full; password (if set);
// opposed; dead without pending raise; then CurHits = MaxHits, CurSpellPoints = MaxSpellPoints,
// PruneTheDead (index). Join completes only if candidate passes R5 with the leader.
// UNKNOWN: the add-to-party step is not shown in the SRD; assumed to append as in joinParty.
// CONFLICT: port admits dead joiners with pending raise; the engine refuses them (chunk 48).
CityJoinOutcome joinFromCity(Ctx& ctx, Party& party, Character& who, const Str& said);

// R27 password check against stored plain-text password (exact match)
// UNKNOWN: case sensitivity not stated.
bool passwordMatches(const Character& who, const Str& said);

// R28 body is available in the city: dead, not self, dungeonLevel == 0,
// and no party member carries it (carryingCharacterName == body.name).
bool isBodyInCity(const Character& body, const Character& self, const Party& party);

// R29 give gold to a member. amount < 1: NothingToGive. amount > giver on-hand: NotEnoughGold.
// Otherwise giver.onHand -= amount, receiver.onHand += amount.
PartyGiveGoldOutcome giveGoldToMember(Ctx& ctx,
                                      Party& party,
                                      Character& giver,
                                      i32 receiverIndex,
                                      Currency amount);

// R30 Give slot from typed number n: (n > 1) ? n - 1 : 0
i32 itemSlotIndex(i32 n);

// R31 give item to a member. packSlot is the value from itemSlotIndex.
// kGiveHandsSlot refuses NoItemToGive; equipped refuses Equipped.
PartyGiveItemOutcome giveItemToMember(Ctx& ctx,
                                      Party& party,
                                      Character& giver,
                                      i32 receiverIndex,
                                      i32 packSlot);

// R32 leave party for the selected member. Leader (selected index 0) refused.
// monstersInArea refuses. confirmed == false: NotConfirmed. On Left: removeMember (R15),
// then pruneTheDead (index).
PartyLeaveOutcome leaveParty(Ctx& ctx, Party& party, bool confirmed, bool monstersInArea);

// R33 mid-fight setup for a living joiner (original sub0BDE). Dead joiners are never passed here.
void setupJoinerInCombat(Ctx& ctx, Party& party, Character& newcomer);

// R34 set default combat options. Refused in combat, or with monsters in the area.
DefaultOptionsOutcome setDefaultCombatOptions(Ctx& ctx,
                                              Party& party,
                                              bool inCombat,
                                              bool monstersInArea);

// R35 disband party: event "Disbanding Party...". Save only with rememberForReload.
// Host deletes or writes the party file from the returned action (core does no IO).
PartySaveAction disbandParty(Ctx& ctx, Party& party, bool rememberForReload);

// R36 gold leader for town actions: first member; if dead or not in the city, the next
// living city member in party order. Returns -1 if none.
// UNKNOWN: "in the city" means dungeonLevel == 0; fallback per deviation list (chunk 36).
i32 townGoldLeaderIndex(const Party& party);

// R37 command-switch slot validity. Dungeon: party size > 1 and slot in 1..kCommandSwitchSlotsMax
// and within the party. City: slot 0 or higher.
// CONFLICT: index base and Alt+0 behaviour in the city are not confirmed (chunks 43, 48).
bool commandSwitchSlotValid(i32 slot, i32 partySize, bool inDungeon);

}  // namespace mordor
