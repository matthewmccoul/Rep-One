#pragma once
// SRD part c16p2: ratio, city rest and shelf/pen pricing helpers, vault and bank slot tests,
// town charges, library day text, seer gate and fee, guild log visibility, class wear,
// companion price bands, crest charges.
// Rules already in the API INDEX (restInCity, priceToSell, priceToIdentify, priceToRealign,
// WearRefusal) are called from there and are not redeclared.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/items3a.h"
#include "mordor/city1.h" // FIXED: ChargeResult is declared in city1.h; include it so charPay can name it.

namespace mordor {

// ---- constants ----
inline constexpr i32 kShelfCap = 9;                // R16/R21 ShelfCap = 9; max creatures per pen alignment and row cap [c9, c55]
inline constexpr i32 kDeepestStocked = 14;         // R18 / T-J deepest stocked floor = mc0118 - 1
inline constexpr i32 kFloorLimit = 15;             // mc0118 = 15 (floor limit constant) [c63]
inline constexpr i32 kRestNumerator = 100;         // R3 rest numerator
inline constexpr double kRestDivisor = 3.25;       // R3 rest divisor
inline constexpr double kRealignFactor = 0.72;     // R26 store realign fee factor
inline constexpr double kBuyFactor = 1.5;          // R23 set 2 buy factor
inline constexpr double kBuyDivisor = 1.15;        // R23 set 2 buy divisor

// T-H library day text selector (the text itself is produced by the caller's screen)
enum class LibraryDayPhrase : i32 {
    NotKnown = 0, // today == 0: "it is not known how many days ago."
    Today = 1,    // days == 0: "just Today."
    OneDay = 2,   // days == 1: "just 1 day ago."
    Days = 3      // else: "{days} days ago."
};

// T-R confinement companion price band by levelFound [c92].
// Table is the file's data; not verified as the rule.
// UNKNOWN: unit of the table prices (raw file units vs Currency 1/10000 gold).
struct CompanionPriceBand {
    i32 level = 0;
    Currency lowest = 0;
    Currency highest = 0;
    const char* lowestName = "";
    const char* highestName = "";
};

inline constexpr CompanionPriceBand kCompanionPriceBands[20] = {
    {1, 1000, 714870, "Slave", "Aboleth"},
    {2, 131384, 936909, "Ant", "Dungeonweed"},
    {3, 256602, 1515992, "Giant Raven", "Giant Slug"},
    {4, 385785, 2143695, "Giant Ant", "Flame Dragon"},
    {5, 470997, 1547760, "Vampire Bat", "Twisted Elf"},
    {6, 454991, 3990716, "Lost Soul", "Minotaur"},
    {7, 618968, 2781271, "Spell Binder", "Gibbering Yethwa"},
    {8, 761529, 2372381, "Earwig", "Ninja"},
    {9, 360836, 1752641, "Rot Grub", "Swordsman"},
    {10, 1271816, 5373431, "Kyu t'Salli", "Asteryex"},
    {11, 990620, 4023402, "Beechi", "Griffin"},
    {12, 1645361, 5279246, "Coltpixie", "Mother of Serpents"},
    {13, 1521211, 4019242, "Tarantula", "Elven Lord"},
    {14, 1853002, 5775805, "Fire Lord", "Dragon Turtle"},
    {15, 982988, 6454997, "Slither", "Dragon King"},
    {16, 2855101, 15511619, "T'Hota", "Asmodeus"},
    {17, 6555443, 12450882, "Iron Golem King", "Grand Magus"},
    {18, 13555221, 22550110, "T'Nardana", "Ancient Dragon"},
    {19, 35555440, 45110221, "Shadow Monarch", "Abyssal Sovereign"},
    {20, 99999999, 99999999, "The Void", "The Void"}
};

// ---- R74 Ratio ----
// R74 ratio_raw = Price(a) / Price(b), unrounded. Example: Giant Spider / Kobold ~ 2.22.
// UNKNOWN: which price (buy, sell, base) Price() denotes is not stated; caller supplies both.
// UNKNOWN: zero denominator not stated; return 0.0.
double ratioRaw(Currency priceNumerator, Currency priceDenominator);

// ---- R3 city rest ----
// R3 City rest gate: DungeonLevel == 0 and not dead.
bool cityRestGate(i32 dungeonLevel, bool dead);
// R3 City rest days: (CurHits < MaxHits) ? round_half_even((100 - CurHits/MaxHits*100) / 3.25) : 0.
// UNKNOWN: CurHits / MaxHits is decimal (float) in the examples; original integer behaviour unknown.
// FIXED: return type changed from i32 to i64 to match the existing declaration in city1.h
// (i64 cityRestDays(i32, i32)); two declarations differing only by return type are ambiguous.
i64 cityRestDays(i32 curHits, i32 maxHits);

// ---- R11 alignment choice ----
// R11 Alignment choice is asked when Stocked > 1 (more than one stocked alignment in a row).
bool alignmentChoiceAsked(i32 stockedAlignmentCount);

// ---- R16 shelving test ----
// R16 shelves = !cursed AND !(unaligned AND floor == 0) AND alignRowCount < kShelfCap.
bool shelvingAllowed(bool cursed, bool unaligned, i32 floor, i32 alignRowCount);

// ---- R18 shelf floor cap ----
// R18 if deepestReached >= kFloorLimit - 1 then deepestReached = kDeepestStocked (14).
i32 cappedShelfDepth(i32 deepestReached);

// ---- R21 pen add ----
// R21 if penCount < kShelfCap then penCount += 1. Returns true when a creature was added.
bool penAddOne(i32& penCount);

// ---- R23 pen buy price, set 2 ----
// R23 round_half_even(1.5 * BaseValue / 1.15) if penCount > 0, else 0.
// UNKNOWN: whether the max(BaseValue, 10) floor from the engine buy price applies in set 2.
Currency penBuyPriceSet2(Currency baseValue, i32 penCount);

// ---- R24 pen sell price and R25 pen identify price ----
// Declared in the API INDEX as priceToSell and priceToIdentify; not redeclared here.

// ---- R26 store realign fee ----
// R26 fee = Round(kRealignFactor * price), round_half_even.
// Example: price 4 -> fee 3.
Currency realignStoreFee(Currency price);

// ---- R31 vault deposit slot test ----
// R31 slot is refused if slot <= 0 or slot >= kBankSlots (40). Returns true when accepted.
bool vaultSlotAccepted(i32 slot);

// ---- R33 bank free slots ----
// R33 free slots = kBankSlots - count(bank item with id > 0).
i32 bankFreeSlots(const Inventory& inv);

// ---- R35 shortfall and town charge ----
// R35 need = max(0, cost - payerPurse).
Currency shortfallNeed(Currency cost, Currency payerPurse);
// CharPay: town charge. On-hand gold first, then bank. Refuses without changing state if the
// purse (on hand + bank) is short. Emits text through ctx.say.
// FIXED: CharPayOutcome exists nowhere; replaced by ChargeResult (the existing town charge
// result type used by townCharge in city1.h).
ChargeResult charPay(Ctx& ctx, Character& payer, Currency cost);

// ---- R62 library day count ----
// R62 days = |seenSerial - todaySerial|. Serial = whole days since 1899-12-30.
i64 libraryDaysAgo(i64 seenSerial, i64 todaySerial);
// T-H library day text selector. todaySerial == 0 gives NotKnown.
LibraryDayPhrase libraryDayPhrase(i64 todaySerial, i64 days);

// ---- R64 seer ----
// R64 seer gate: refuse if level > deepestHeCanReach (original kSeerDeepestOriginal).
bool seerRefuses(i32 level, i32 deepestHeCanReach);
// R64 seer fee base = kSeerFeeBase * ln(level + 1) * level (natural log).
// UNKNOWN: rounding of the fractional fee, and the "halved" adjustments, are not stated in this part.
Currency seerFeeBase(i32 level);

// ---- R65 guild log clean visibility ----
// R65 Clean visible = GuildLog.NeedsCleaning(...). File reading is done by the caller;
// this takes the counts it needs.
// UNKNOWN: NeedsCleaning criterion and KeepOnClean value are not stated in this part.
bool guildLogCleanVisible(i32 logEntryCount, i32 guildCount);

// ---- R67 gold input rounding ----
// R67 entered gold is rounded to whole gold, round half to even ("2.5" -> 2, "3.5" -> 4),
// then converted to Currency.
// UNKNOWN: whether rounding is to whole gold or to 1/10000; whole gold assumed.
Currency goldInputToCurrency(double enteredGold);

// ---- R70 class wear ----
// R70 maxWorn == 0 -> NotWearable; maxWorn == -1 uncapped; maxWorn > 0 and wornCount >= maxWorn -> ClassFull.
WearRefusal classWearCheck(i32 maxWorn, i32 wornCount);

// ---- T-M item alignment to pen index ----
// T-M pen index: Good 0, Neutral 1, Evil 2 (item scheme is 1-based).
// UNKNOWN: pen index for Unaligned; -1 returned.
i32 penAlignIndex(ItemAlign align);

// ---- T-R companion price band lookup ----
// Returns the band for a levelFound 1..20.
// UNKNOWN: behaviour outside 1..20 not stated; caller must pass a level in range.
CompanionPriceBand companionPriceBand(i32 level);

// ---- rounded index draw ----
// Terms: rounded index = round_half_even(Rnd * n). Result in 0..n; index n matches nothing,
// index 0 is half as likely. Consumes one ctx.rng.nextDouble().


}  // namespace mordor
