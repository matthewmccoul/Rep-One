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








// ---- R24 pen sell price and R25 pen identify price ----
// Declared in the API INDEX as priceToSell and priceToIdentify; not redeclared here.












// ---- rounded index draw ----
// Terms: rounded index = round_half_even(Rnd * n). Result in 0..n; index n matches nothing,
// index 0 is half as likely. Consumes one ctx.rng.nextDouble().


}  // namespace mordor
