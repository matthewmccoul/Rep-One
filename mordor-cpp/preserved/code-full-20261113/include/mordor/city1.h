#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants (SRD c16p1) ----
inline constexpr i32 kTownDeepestFloor = 14;         // R8, R18, R28 mc0118 - 1 (original off-by-one, see R18)
inline constexpr i32 kShelfRowCap = 9;               // R16, R19 store row cap // UNKNOWN: engine value not in chunks; docs say 9
inline constexpr i32 kPenShelfCap = 9;               // R21, R24, R25 pen cap per alignment
inline constexpr i32 kRestockChanceMin = 1;          // R8, R28 chance clamp low
inline constexpr i32 kRestockChanceMax = 50;         // R8, R28 chance clamp high
inline constexpr i32 kAddProbabilityPct = 60;        // R8, R28 add when rnd*100 < 60
inline constexpr double kRestDaysDivisor = 3.25;     // R3 days divisor
inline constexpr i32 kCrestScanFirst = 1;            // R5 pack slots scanned from 1 (slot 0 not examined)
inline constexpr i32 kCrestScanLast = 39;            // R5 last pack slot scanned
inline constexpr i32 kPenAlignCount = 3;             // R30 pen indices 0..2 = Good, Neutral, Evil
inline constexpr i32 kAltPoolMinParty = 2;           // CityAltP minimum party size for ALT-p

// Starting artifact stock (T-C), R7.
struct StartingArtifact {
    i32 itemId;
    ItemAlign alignment;
    i32 count;
};
inline constexpr StartingArtifact kStartingArtifacts[6] = {
    {356, ItemAlign::Neutral, 1},
    {357, ItemAlign::Neutral, 1},
    {358, ItemAlign::Good, 1},
    {359, ItemAlign::Evil, 1},
    {360, ItemAlign::Neutral, 1},
    {361, ItemAlign::Neutral, 1},
};

// Crest charges by guild index, Ailments 500..511 in order (T-Q), R5.
inline constexpr i32 kCrestCharges[kGuildCount] = {0, 0, 12, 0, 6, 6, 12, 6, 6, 6, 6, 6};

// One (bit, rate) pair for base worth (T-A, T-B), R29.
struct BitRate {
    i32 bit;
    i32 rate;
};
inline constexpr BitRate kSpecialAttackRates[7] = {
    {3, 3250}, {4, 3250}, {5, 2050}, {6, 2050}, {8, 1500}, {10, 1500}, {11, 500},
};
inline constexpr BitRate kSpellRates[7] = {
    {0, 5250}, {1, 3250}, {2, 4250}, {3, 3250}, {4, 5250}, {5, 5250}, {6, 4250},
};

// ---- enums ----

// R6 town sections, in UPDATE order.
enum class TownSection : i32 { Store = 0, Confinement, Morgue, Guild, Bank };

// R31 deposit outcome.
enum class VaultDeposit : i32 { Refused = 0, Stored = 1, EmptySlot = 2 };

// CityAltP outcome (R37 step 6).

// R47 TownArrival answer kind.
enum class ArrivalAction : i32 { None = 0, PartyDeposit, PartyPool, Other };

// ---- small state and result structs ----

// Pen counts per alignment index (Good, Neutral, Evil), R30.
using PenCounts = Arr<i32, kPenAlignCount>;

// Pens indexed by monster reference (R21, R27, R28).
struct PenBook {
    Vec<PenCounts> byRef{};
};

// Town stock the core mutates on arrival and restock.
struct TownStock {
    Vec<Item> store{};        // General Store shelf (R7 to R19)
    PenBook pens{};           // Confinement pens (R20 to R30)
    i32 deepestReached = 0;   // deepest floor the party has reached (R18)
};

// Store Buy selection state (R12, R13).
struct StoreSelection {
    i32 row = -1;
    i32 alignment = -1;
    bool choosing = false;
    bool buyEnabled = false;
    bool infoEnabled = false;
};

// Alignment choice result (R11, R12, R14). Buttons: index 0 = Unaligned (U), 1 = Good, 2 = Neutral, 3 = Evil.
struct AlignChoiceRow {
    Arr<bool, 4> enabled{};
    i32 stocked = 0;
    i32 last = -1;
    bool asks = false;
    i32 chosen = -1;
};

struct StoreSaleResult {
    bool sold = false;
    bool shelved = false;
    i32 payee = -1;           // member index paid; -1 for refused
    Currency amount = 0;
};

struct ChargeResult {
    bool paid = false;
    Currency fromHand = 0;
    Currency fromBank = 0;
};

struct ShortfallGift {
    i32 giver = -1;
    Currency amount = 0;
};

struct ShortfallCover {
    bool covered = false;
    Vec<ShortfallGift> gifts{};
};

// Answer from the TownArrival mod hook (R47). Plain data filled by the mod adapter.
struct ArrivalHookAnswer {
    bool answered = false;                       // a non-null answer exists
    ArrivalAction action = ArrivalAction::None;
    i32 poolTo = -1;                             // member index named by PartyPool
    bool hasMessage = false;
    Str message{};
};

// Mod answers for one call (R37 to R47). Plain data, resolved by the mod adapter outside the core.
struct TownModAnswers {
    i32 liveMods = 0;                  // R37: 0 means no mod hooks run
    Vec<i32> hiddenRows{};             // R38: rows a StoreRowVisible mod answered false for
    bool rowOrderMatches = true;       // R39: SameRows check result
    bool rowOrderDisabled = false;     // R44: session flag set on mismatch
    bool rowChangeAllowed = false;     // R40: default refused
    bool alignChoiceUnlocked = false;  // R43: default refused
    bool incomeAnswered = false;       // R41: non-null TownIncomeBank answer exists
    i32 incomePayee = -1;              // R41: member index named by TownIncomeBank
    i32 messagesKept = 0;              // R42: first non-null answer; 0 = no answer
    bool altPoolAnswered = false;      // CityAltP: mod chose a mode
    bool altPoolDeposit = false;       // CityAltP: PoolAndDeposit
    Vec<i32> coverOrder{};             // R35: named giver order; empty = party order
    ArrivalHookAnswer arrival{};       // R47
};

struct ArrivalDepositResult {
    Currency deposited = 0;
    bool messageDue = false;           // R47 step 4: deposited > 0 and a message exists
    Str line{};
    bool wealthiestCheckDue = true;    // R47 step 5
};

struct ArrivalResult {
    Arr<bool, kPartyMaxSize> rested{};     // R1 step 3 and 4: member rested in town
    bool townReplenished = false;          // R1 step 5 ran
    ArrivalDepositResult deposit{};        // R1 step 6
};

// ---- rule declarations ----

// R1 Arrival in town: clear lost flags, hall check, rest gate and city rest, replenish, arrival hook.
// UNKNOWN: order of steps 2, 4 and 5 relative to each other (c48, c2, c29).
// CONFLICT: step 5 trigger vs wiki waking refresh (R72), not in this part; implemented as stated.
ArrivalResult arriveInTown(Ctx& ctx, Party& party, TownStock& town, i64& mostExperience, const TownModAnswers& mods);

// R1 step 2 Hall of Records: compare each member's TotalExperience against mostExperience; save via reference.
void checkHallOfRecords(Ctx& ctx, const Party& party, i64& mostExperience);

// R2 Rest gate: city rest runs only if DungeonLevel == 0 and character is not dead.
bool cityRestGatePasses(const Character& ch);

// R3 City rest days added to Age. Formula: round_half_even((100 - CurHits / MaxHits * 100) / 3.25) when CurHits < MaxHits, else 0.
// UNKNOWN: integer or float division for CurHits / MaxHits (c2). Full rest body is restInCity (progression2.h, already declared).
i64 cityRestDays(i32 curHits, i32 maxHits);

// R4 Only the first party member's rest replenishes store and pens (index 0).
bool replenishesOnRest(i32 memberIndex);

// R5 Refill guild-issued items in pack slots 1..39 to their full charges. Returns the number refilled.
// Message: one line alone; in a party, one line with the trimmed owner name in quotes.
i32 refillGuildCrests(Ctx& ctx, Inventory& inv, const Str& ownerName, bool inParty);

// R6 Town switch update order. Empty unless leader is not in dungeon and not dead. Seer receives nothing.
Vec<TownSection> townSwitchOrder(const Party& party);

// R7 Starting store stock (CreateInitial): artifacts 356..361, then general items by floor, price, cursed and alignment.
// CONFLICT: c22 tests Items.Align > 1 (original); c34 says port tests more than one bit. Original followed.
void createInitialStore(Ctx& ctx, Vec<Item>& store);

// R8 Store replenish (sub071B). Draws from rng; days aged from rest; deepest clamped to 1..14.
// UNKNOWN: literal vs integer division in days / 100 * 100 (equal to days for days >= 0 in float form).
void replenishStore(Ctx& ctx, Vec<Item>& store, i64 daysAged, i32 deepestReached);

// R9 Item draw (DrawOne). Returns item index into ctx.data.items for the floor, or 0 when nothing matches.
i32 drawItemIndex(Ctx& ctx, i32 floor);

// R10 Rounded index: round_half_even(Rnd * n). Index n matches nothing and is retried by callers.
// CONFLICT: port (c19) rounds; original (c61) uses VB Integer truncation. Rounding used per R9.
i32 roundedIndex(Ctx& ctx, i32 n);

// R11 Alignment choice shared by General Store and Confinement. chosenBefore is the previous choice.
AlignChoiceRow alignmentChoice(const Arr<i32, 4>& counts, i32 chosenBefore);

// R12 Store Buy row click: enabled = counts > 0; asks when more than one enabled; one enabled is taken at once.
AlignChoiceRow storeBuyRowClick(const Arr<i32, 4>& counts, i32 chosenBefore);

// R13 Clear selection when highlighted row sold out or is hidden (withdraws pending choice).
void settleStoreSelection(StoreSelection& sel, bool soldOutOrHidden);

// R13 step 1 and R40: a row click while choosing is allowed only for the same row or if a mod allows row change.
bool storeRowClickAllowed(const StoreSelection& sel, i32 newRow, const TownModAnswers& mods);

// R14 Confinement alignment choice (buttons G, N, E); same logic as R11. Frame hides after a pick.
AlignChoiceRow confinementRowClick(const Arr<i32, 4>& penCounts, i32 chosenBefore);

// R15 Store sale: income to seller's hand by default, offered first to TownIncomeBank (R41). Shelves only if R16 holds.
// Refuses with nothing moved on overflow.
StoreSaleResult storeSale(Ctx& ctx, Party& party, i32 sellerIndex, Item& item, Vec<Item>& store, const TownModAnswers& mods);

// R16 Shelving test: !cursed and !(unaligned and floor == 0) and row count for alignment < 9.
// UNKNOWN: definition of row count (c36 FRMCTYS.BAS:264-267).
bool storeShelvesItem(const Ctx& ctx, const Item& item, const Vec<Item>& store);

// R16 Row count for an alignment bucket on the shelf (Unaligned, Good, Neutral, Evil).
i32 storeRowCount(const Vec<Item>& store, ItemAlign align);

// R17 General Store buys at 50% of base value.
// UNKNOWN: rounding of the half value at currency precision (wiki source c87).
Currency generalStorePayout(Currency baseValue);

// R18 Shelf floor cap: deepest floor used for shelves is min(deepestReached, 14). Original off-by-one, reproduced.
i32 storeShelfFloorCap(i32 deepestReached);

// R19 Store row cap is kShelfRowCap (9). Constant only; see R16.

// R20 Confinement buy: price from pen stock (R22). Takes from pen only when pen count > 0, then charges payer.
bool confinementBuy(Ctx& ctx, Party& party, i32 payerIndex, PenBook& pens, i32 monsterRef, i32 align, const TownModAnswers& mods);

// R20 Confinement sell: pen Add must succeed (R21) before paying the seller's hand price (R24). Pays seller's hand.
bool confinementSell(Ctx& ctx, Party& party, i32 sellerIndex, PenBook& pens, const Companion& companion, const TownModAnswers& mods);

// R21 Pen Add (PenOne): +1 when LevelFound != 0, ref >= 0 and cap test passes.
// Original quirk: cap test reads the Good pen (index 0) until a take-away has run in the same call (c9).
bool penOne(PenBook& pens, i32 ref, i32 align, i32 levelFound);

// R21 Pen Take (ReleaseOne): -1 with no floor check; caller guards with penTakeAllowed.
void releaseOne(PenBook& pens, i32 ref, i32 align);

// R20 and R21: buy guard, true when the pen count is above 0.
bool penTakeAllowed(const PenBook& pens, i32 ref, i32 align);

// R22 Pen buy price (engine): value = max(base, 10); stock = min(penned - 1, 9); price = 1.5*value / (ln(stock+1) + 1.15).
// Returns 0 when penned == 0 (Log(0) error trap reproduced). Rounded half to even.
// Original quirk: Stats(0) strength read from an unassigned variable (c9); port keeps it.
Currency penBuyPrice(Currency baseValue, i32 pennedCount);

// R23 Pen buy price (Set 2, port-pinned): round_half_even(1.5 * base / 1.15) if pen > 0, else 0.
// CONFLICT: contradicts R22 (engine). R22 preferred; this variant is declared for reference only.
Currency penBuyPriceSet2(Currency baseValue, i32 pennedCount);

// R24 Pen sell price: stock = penned (1 if 0 and any alignment penned), min 9; price = (0.9 * base / (stock + 1.15)) / (idLevel + 1).
Currency penSellPrice(Currency baseValue, i32 pennedCount, bool anyAlignmentPenned, i32 idLevel);

// R25 Pen identify price (only when idLevel > 0): v = (0.13 * base) / ((stock / 2.0 + 1) * 0.9) / idLevel + 1.
// UNKNOWN: whether the trailing +1 is inside the division (as written, after it). Preference: as written.
Currency penIdentifyPrice(Currency baseValue, i32 pennedCount, i32 idLevel);

// R26 Realign price and fees: already declared as priceToRealign(const MonsterDef&, CompanionAlignment, CompanionAlignment) in items2b.h.
// CONFLICT: R26 formula conflicts with c29 and c65 store and Set 2 fees. Use the declared function.

// R27 Starting companion stock: 10 table picks, two of each (20 creatures). Writes pens per AlignmentOf(monster).
// CONFLICT: text says ten creatures, two of each; picks give twenty creatures. Twenty followed.
void seedStartingPens(Ctx& ctx, PenBook& pens);

// R28 Creature restock on rest: passes capped at 500; add or release one creature per pass by rng.
void restockPens(Ctx& ctx, PenBook& pens, i64 daysAged, i32 deepestReached);

// R29 Base worth of a creature (monster definition) at a guild level, using the SRD formula and bit tables.
// Only positive resistances count (c9). If Hits == 1, value is 1.
Currency creatureBaseWorth(const MonsterDef& def, i32 guildLevel);

// R30 Map item alignment (1-based: Good 1, Neutral 2, Evil 3) to pen index (0-based: Good 0, Neutral 1, Evil 2).
// Unaligned returns -1. UNKNOWN: pen index for Unaligned goods.
i32 penIndexOfItemAlign(ItemAlign align);

// R31 Deposit to vault: refuses slot 0 and >= 40, hands, equipped items; first empty vault slot, holes included.
VaultDeposit depositToVault(Ctx& ctx, Inventory& inv, i32 packSlot);

// R32 Withdraw: each selected vault line to first free pack slot (re-identified). Buffers naming the freed slot are let go.
// UNKNOWN: buffer representation; implemented against party.itemOrders naming the pack slot.
i32 withdrawFromVault(Ctx& ctx, Inventory& inv, Party& party, const Vec<i32>& bankSlots);

// R33 Free vault slots: kBankSlots minus occupied vault slots (ItemId > 0).
i32 freeVaultSlots(const Inventory& inv);

// R34 Town charge: taken from gold on hand first, then bank; refuses with "Not enough gold!" if the purse is short.
// Before refusal, cover shortfall (R35) may be tried.
ChargeResult townCharge(Ctx& ctx, Party& party, i32 payerIndex, Currency cost, const TownModAnswers& mods);

// R35 Shortfall cover (mod): gifts from other members' banks to the payer's hand. All or nothing.
ShortfallCover coverShortfall(Ctx& ctx, Party& party, i32 payerIndex, Currency cost, const TownModAnswers& mods);

// R36 WouldCoverShortfall: plan only, moves nothing. Used for purse tests before a charge.
ShortfallCover wouldCoverShortfall(const Party& party, i32 payerIndex, Currency cost, const TownModAnswers& mods);

// R37 Mod decision order: overview. CityAltP (step 6): ALT-p in town with party of two or more; never in dungeon.
// Party of one is refused before any mod is asked.
AltPoolOutcome chooseAltPPool(const Party& party, bool inTown, const TownModAnswers& mods);

// R38 Store Buy row visibility: visible unless a live mod answered false for the row.
bool storeRowVisible(const TownModAnswers& mods, i32 row);

// R39 Store row order check: same count and same multiplicity of each row identifier.
// Mismatch means a Fault for that mod (caller sets rowOrderDisabled per R44).
bool storeRowOrderAccepted(const Vec<i32>& defaultRows, const Vec<i32>& modRows);

// R40 Store row change while choosing: refused unless some mod answered true. A throwing mod counts as false.
bool storeRowChangeAllowed(const TownModAnswers& mods);

// R41 Town income bank: returns member index who receives income, or -1 for seller's hand (original).
// A payee who is not a party member also yields -1.
i32 townIncomePayee(const Party& party, i32 sellerIndex, const TownModAnswers& mods);

// R42 Message window line count: max(1, mod answer) when answered; otherwise kDefaultMessageLines (20).
i32 messageWindowLines(const TownModAnswers& mods);

// R43 Store alignment choice unlock (Quality of Life 1.12.0 toggle). Default refused.
bool storeAlignChoiceUnlocked(const TownModAnswers& mods);

// R44 Store Buy list order disabled for the session after a row-order mismatch (R39).
void disableRowOrderForSession(TownModAnswers& mods);

// R45 Row changes while choosing: governed by storeRowChangeAllowed (R40). No separate declaration.
// R46 Mod Set 2 note: mod hooks are port additions. No declaration.

// R47 Arrival deposit and pool: PartyDeposit or PartyPool per TownArrival answer; default nothing.
// Message only when deposited > 0. Wealthiest record check is reported via wealthiestCheckDue.
// UNKNOWN: storage of the Wealthiest record (not in this part); caller performs the check.
ArrivalDepositResult arrivalDepositAndPool(Ctx& ctx, Party& party, const TownModAnswers& mods);

// R47 PartyDeposit: each member's gold on hand goes into their own bank account. Returns amount moved.
Currency partyDepositOnHand(Party& party);

// R47 PartyPool and CityAltP pool: all members' gold on hand goes to member toMember's bank, all or nothing on overflow.
// Returns amount moved; 0 when toMember is not in the party or on overflow.
Currency partyPoolOnHandToBank(Party& party, i32 toMember);

}  // namespace mordor
