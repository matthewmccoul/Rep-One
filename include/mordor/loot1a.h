#pragma once
// loot1a.h - SRD part c14p1: chest generation, traps, locks, gold and loot.
// Declarations only. Bodies live in src/loot1a*.cpp.
//
// Conventions used here:
//  - A function takes Ctx& if it rolls, mutates state or emits events.
//  - Pure value/state queries take no Ctx and use const references.
//  - identifyFee and sellPrice are declared in items1a.h / items3a.h and are not redeclared.
// CONFLICT: state_common.h is included both directly and through state_character.h /
//   state_dungeon.h, and state_character.h / state_item.h have no include guards.
//   The foundation owner must add guards before this header compiles.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_dungeon.h"

namespace mordor {

struct Party;   // defined in state_party.h
struct Shelf;   // defined in items3a.h

// ---- constants ----
inline constexpr i32 kTreasureClassNone = 0;           // R1 no chest
inline constexpr i32 kTreasureClassBox = 2;            // R1 box (class 1 is never generated)
inline constexpr i32 kTreasureClassChest = 3;          // R1 chest
inline constexpr i32 kLairTreasureBonus = 25;          // R9 lair bonus to both thresholds
inline constexpr i32 kTrapBitCount = 12;               // R12 trap bits 0..11
inline constexpr i32 kTrapDrawAttempts = 200;          // R12 port cap, CONFLICT-5
inline constexpr i32 kTrapChanceByClass[4] = {0, 25, 75, 90};  // R2 F3, indexed by treasure class
inline constexpr i32 kLockSquareBitExtinguisher = 10;  // R10 square bit EXTINGUISHER
inline constexpr i32 kLockSquareBitAntimagic = 18;     // R10 square bit ANTIMAGIC
inline constexpr double kLockedGoldFactor = 1.5;       // R11 locked chest gold multiplier
inline constexpr double kClassGoldFactor[4] = {0.5, 0.5, 1.0, 1.5};  // R4 F5, indexed by class
inline constexpr i32 kLowGroupGoldLimit = 500;         // R15 groups below this are replaced
inline constexpr i32 kLowGroupGoldBase = 2;            // R15 replacement is Int(Rnd*500)+2
inline constexpr i32 kItemRollClassMod[4] = {0, 15, 0, -25};  // R7 F7 modifier by class
inline constexpr i32 kItemRollCap = 90;                // R7 roll cap when ItemChance != 100
inline constexpr i32 kItemRollCapExempt = 100;         // R7 ItemChance equal to this disables the cap
inline constexpr i32 kItemFurtherDivisor = 10;         // R18 F8 threshold is Int(ItemChance/10)
inline constexpr i32 kChestDisarmMin = 5;              // R23 disarm clamp low (percent)
inline constexpr i32 kChestDisarmMax = 98;             // R23 disarm clamp high (percent)

// Per-group inputs to F4.
struct MonsterGroupGold {
    i32 goldFactor = 0;       // GoldFactor (shipped values 2..7)
    i32 levelFound = 0;       // LevelFound
    i32 groupCount = 0;       // GroupCount
};

// Item drop-level range from R19.
struct ChestDropBand {
    i32 top = 0;              // highest drop level allowed
    i32 low = 0;              // lowest drop level allowed
};

// Outcome of one trap resolution (R13, R14, R33).
struct ChestTrapResult {
    ChestTrap trap = ChestTrap::NotTrapped;
    bool disarmed = false;          // R23 trap was disarmed, no effect
    i32 damage = 0;                 // R13 damage applied to the opener
    bool poisoned = false;          // R14 trap 1
    bool diseased = false;          // R14 trap 2
    bool itemsDestroyed = false;    // trap 5 item destruction
    bool partyKilled = false;       // R33 trap killed the party
    bool teleported = false;        // R33 trap teleported the party
};

// Result of starting a chest open in combat (R21, R22).
enum class ChestOpenStart : i32 {
    RefusedDead = 0,     // R21 opener is dead
    RefusedParalysed,    // R21 opener is paralysed
    RefusedMagicLock,    // R21 chest is magically locked
    CountdownSet,        // R22 countdown is running
    OpenNow              // R22 countdown is 0, open on this turn
};

// Result of a chest open in combat (R23, R24, R33).
struct ChestOpenResult {
    bool opened = false;          // chest reached the fight-record and room-write steps
    bool openerDied = false;      // R24 leader fell below 1 Hits during the trap
    bool payoutStopped = false;   // R33 trap stopped gold and items
    Currency goldPaid = 0;        // R23 gold paid out
    i32 itemsPlaced = 0;          // R23 items placed with takers
    ChestTrapResult trap{};       // R23 trap outcome
};

// ---- encounter setup ----


// R1 roll the treasure class (F1): 0 none, 2 box, 3 chest.
// FIXED: LeadChestStats exists nowhere in the API index; replaced by plain i32 (lead Id/stats not modelled).
i32 rollTreasureClass(Ctx& ctx, i32 lead);
// R2 clear the chest trap at every generation pass; the lock is kept.
void clearChestTrapForPass(Ctx& ctx, SquareState& chest);
// R12 draw trap bits until a set bit is found, up to kTrapDrawAttempts; 0 means none (CONFLICT-5).
i32 drawChestTrapBit(Ctx& ctx, i32 trapFlags);
// R2 set a trap if class > 0, TrapFlags > 0 and the chance roll passes (F3).
ChestTrap rollChestTrap(Ctx& ctx, i32 treasureClass, i32 trapFlags);
// R3 lock roll passes when Int(Rnd*100)+1 <= LockedChance and class > 1 (F2, CONFLICT-1).
bool rollChestLock(Ctx& ctx, i32 treasureClass, i32 lockedChance);
// R10 square allows magic: not ANTIMAGIC (bit 18) and not EXTINGUISHER (bit 10). Pure.
bool squareAllowsMagicLock(u32 squareBits);
// R10 magic lock: square check first, then the R3 roll. Returns MAGIC_LOCKED or UNLOCKED.
ChestLock applyMagicLock(Ctx& ctx, u32 squareBits, i32 treasureClass, i32 lockedChance);
// R4 base group gold from F4 before the low-gold floor, using GoldFactor and LevelFound.
Currency rollBaseGroupGold(Ctx& ctx, const MonsterGroupGold& group);
// R15 replace group gold below 500 with Int(Rnd*500)+2 (Q6).
Currency applyLowGroupGoldFloor(Ctx& ctx, Currency groupGold);
// R4 per-group weighting: gold * (lg*(lg/2)), lg = ln(GroupCount+1). Pure.
Currency weightGroupGold(Currency groupGold, i32 groupCount);
// R4 scale encounter gold by standing: gold * (standing/2.0). Pure.
Currency scaleGoldByStanding(Currency gold, i32 standing);
// R11 locked chest: gold * 1.5. Pure.
Currency scaleGoldForLock(Currency gold);
// R4 scale encounter gold by treasure class factor (0/1 -> 0.5, 2 -> 1, 3 -> 1.5). Pure.
Currency scaleGoldForClass(Currency gold, i32 treasureClass);
// R5 VB Currency from a double: d*10000, round half to even. Pure.
Currency vbCurrencyFromDouble(double d);
// R5 store class, trap, lock and gold on the room record.
void storeChestOnSquare(Ctx& ctx, SquareState& chest, i32 treasureClass, ChestTrap trap, ChestLock lock, Currency gold);
// R7 item-spawn roll on chest creation (F7; CONFLICT-21, CONFLICT-17 for the lock term).
bool rollChestItemSpawn(Ctx& ctx, i32 treasureClass, i32 itemChance, ChestLock lock, i32 dungeonLevel);
// R18 further items while Int(Rnd*100)+1 <= Int(ItemChance/10) (F8).
bool rollAnotherChestItem(Ctx& ctx, i32 itemChance);
// R19 drop-level band (F9): class 1 lowers by one, class 3 raises the low end; BoxChance[3] may force low = top-1.
ChestDropBand chestDropBand(Ctx& ctx, i32 itemDropLevel, i32 boxChance3, i32 treasureClass);
// R19 draw one item from the band (F9 entry and rarity draw). UNKNOWN: exact item table lookup from GameData.items.
Item rollChestItem(Ctx& ctx, const ChestDropBand& band, i32 treasureClass);
// R20 place an item with a random taker. Returns false if nobody had room (message shown).
// UNKNOWN: Character carries no Inventory in the state; takers are a parallel array here.
bool placeChestItem(Ctx& ctx, Arr<Inventory, kPartyMaxSize>& takerInventories, i32 partySize, const Item& item);
// Encounter setup chain for R1 to R11 and R18 to R19 in rule order. Standing is the sum of groupCount.
// FIXED: LeadChestStats replaced by plain i32.
void generateEncounterChest(Ctx& ctx, SquareState& chest, i32 lead, const Vec<MonsterGroupGold>& groups, i32 itemDropLevel, i32 dungeonLevel);

// ---- opening a chest mid-fight ----

// R22 decrement the opener's countdown; returns true when it reaches 0 and the chest opens.
bool tickChestCountdown(Ctx& ctx, Character& opener);
// R13 trap effects (T3) applied to the party. UNKNOWN-6: T3 table is not in the dossier.
ChestTrapResult resolveChestTrap(Ctx& ctx, Party& party, i32 openerIndex, ChestTrap trap);
// R13 blast damage (F25): floor(U*d*k*ln(d+2))+1, k = 4 explosion, 6 fire. UNKNOWN-6: d is undefined in the source.
i32 chestBlastDamage(Ctx& ctx, i32 d, i32 k);
// R24 leader Hits below 1 during the trap. Pure.
bool trapKilledLeader(const Party& party);

// ---- gold split and payout ----


// ---- town charges, gifts and thieves ----

// R26 Give: a whole amount of 0 does nothing; otherwise moves gold from giver to receiver (F15).
bool giveGold(Ctx& ctx, Character& giver, Character& receiver, i64 wholeGold);

// ---- shop ----

// R28 buy price (F18): base price if Floor==0; 0 if no stock; else shelf and charge scaled. Pure.


// ---- hall record ----


}  // namespace mordor
