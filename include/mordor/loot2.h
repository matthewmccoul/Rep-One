#pragma once
// loot2.h: SRD part c14p2 (loot, chests, traps, locks, chest gold, shelf sale, quests, drops).
// UNKNOWN: the supplied SRD part contains only the source notes (UNKNOWN/CONFLICT list). No R-numbers
// were given, so declarations are grouped by source chunk id [cNN] instead of rule number.
// Rename the groups to R-numbers once the rule text is available.
// Items already declared in the API INDEX (for example withering() in progression2.h) are called, not redeclared.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants
inline constexpr i32 kChestCountStart = 2;      // [c65][c24] SquareState.chestCount at creation; CONFLICT-22
inline constexpr i32 kChestTrapTries = 200;     // [c19] port cap on trap-bit draws, returns 0 on exhaustion; CONFLICT-5
inline constexpr i32 kLairBoxBonus = 25;        // [c18] lair adds 25 to BoxChance[2] and to both thresholds; CONFLICT-2
inline constexpr i32 kLockGoldNum = 3;          // [c5][c7] magic lock multiplies chest gold by 3/2; CONFLICT-19
inline constexpr i32 kLockGoldDen = 2;
inline constexpr i32 kItemRollCapThreshold = 90; // [c12] item roll above 90 is capped; CONFLICT-4
inline constexpr i32 kBuyOutDrawCap = 100;      // [c22] buy-out draw cap quirk; CONFLICT-13 (pass -1 for unbounded)
inline constexpr i32 kWitheringWeeks = 4;       // [c64] Withering adds 4 weeks; CONFLICT-28 (engine: message only from sub085B)
inline constexpr i32 kTrapIdentifyMin = 2;      // [c107] clamp 2..98; UNKNOWN-27 (clamp applied to conf per engine [c19])
inline constexpr i32 kTrapIdentifyMax = 98;
inline constexpr i32 kKnownScaleMax = 3;        // [c22] Known scale 0..3; CONFLICT-15
inline constexpr i32 kStartingGoldWhole = 1500; // UNKNOWN-22: port value not in sources; manual value 1,500 [c84]

// ---- result enums and structs
enum class ChestOpenOutcome : i32 { Waiting = 0, Opened, Refused };
enum class DisarmOutcome : i32 { Disarmed = 0, Failed, Triggered };

struct ChestTuning {
    // UNKNOWN: BoxChance[0..3], LockedChance and ItemChance values are not in the SRD part; supplied by data.
    // BoxChance[3] is the lock-trap percentage (CONFLICT-16).
    Arr<i32, 4> boxChance{};
    i32 lockedChance = 0;
    i32 itemChance = 0;
};


struct DisarmResult {
    DisarmOutcome outcome = DisarmOutcome::Failed;
    i32 identifyChance = 0;
};

// ---- [c5][c7][c12][c18][c24] chest creation on square generation
// Chest class from BoxChance thresholds; lair bonus applies to both thresholds, gated on BoxChance[2] > 0 (CONFLICT-2/3).
ChestClass rollChestClass(Ctx& ctx, const ChestTuning& tuning, bool lair);
// Magic lock when ChestClass > 1 and the square allows magic (CONFLICT-1); result is MAGIC_LOCKED or UNLOCKED.
ChestLock rollChestLock(Ctx& ctx, const ChestTuning& tuning, ChestClass cls, bool squareAllowsMagic);
// Draw a set trap bit from trapFlags; returns 0 after kChestTrapTries failed draws (CONFLICT-5).
i32 drawTrapBit(Ctx& ctx, i32 trapFlags);
// Map an engine trap bit (0..11) to ChestTrap; names trap7/trap9/trap10 are placeholders (CONFLICT-29, CONFLICT-34).
ChestTrap trapForBit(i32 bit);
// Chest item-spawn roll; LN floor bonus when lock is CHARM_OPENED (CONFLICT-21); roll above 90 capped (CONFLICT-4).
bool rollChestHasItems(Ctx& ctx, const ChestTuning& tuning, ChestLock lock, i32 dungeonLevel);
// Chest contents; Asmodeus band and high-band adjustments unknown (UNKNOWN-1, UNKNOWN-4).
void rollChestItems(Ctx& ctx, const GameData& data, i32 dungeonLevel, ChestClass cls, Vec<Item>& out);
// Base chest gold from dungeon level; formula not stated (UNKNOWN-3 is chest XP, gold formula unknown).
Currency rollChestGold(Ctx& ctx, ChestClass cls, i32 dungeonLevel);
// Apply class and magic-lock scaling; rounding placement unresolved (UNKNOWN-15, CONFLICT-19).
Currency scaleChestGold(Currency base, ChestClass cls, bool magicLocked);
// Fill a square's chest fields (hasChest, class, lock, trap, gold, items, chestCount = kChestCountStart).
// Trap draw and item roll order unresolved (UNKNOWN-25).
void createChest(Ctx& ctx, const ChestTuning& tuning, SquareState& sq, i32 dungeonLevel,
                 bool lair, bool squareAllowsMagic);

// ---- [c24][c28][c34][c64] trap effects
// Poison: caught when U*100 < resistance/2, as written by the port (CONFLICT-6).
bool chestPoisonTakes(Ctx& ctx, i32 poisonResistance);
// Disease test for trap 2; exact engine test not given separately (UNKNOWN-5).
bool chestDiseaseTakes(Ctx& ctx, i32 resistance);
// Remaining hits after fire trap damage; SRD prefers round-half-even (CONFLICT-7).
i32 fireTrapRemainingHits(i32 hits, i32 damage);
// Apply one trap to the opener and party. d is undefined in the SRD (UNKNOWN-6). Withering calls withering() (progression2.h).
void applyChestTrap(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest, ChestTrap trap, i32 d);
// Blackout: removes spell-granted effects (CONFLICT-26, manual detail unverified).
void applyBlackoutTrap(Ctx& ctx, Party& party, i32 openerIndex);

// ---- [c19][c46][c82][c107] trap identify and disarm
// Identify chance. Natural Wis and gear Wis are separate; disarmRating is the disarm skill (CONFLICT-24, UNKNOWN-28).
i32 trapIdentifyChance(i32 naturalWis, i32 gearWis, i32 monGuildLevel, i32 monDex,
                       i32 thiefGuildLevel, double disarmRating);
// Disarm roll: Rnd(100)+1 <= thieving (CONFLICT-20, engine form taken).
bool trapDisarmRoll(Ctx& ctx, i32 thievingSkill);
// One disarm attempt by a thief in the party. Order versus identify is unknown (UNKNOWN-24).
DisarmResult attemptChestDisarm(Ctx& ctx, const Party& party, i32 thiefIndex, SquareState& chest,
                                i32 thievingSkill, i32 identifyChance);

// ---- [c12][c24][c34][c65] chest countdown and opening
// One chest tick: count 2 -> 1 -> 0 -> opens on the next tick (CONFLICT-22). Returns true when the chest opens.
bool tickChestCountdown(Ctx& ctx, SquareState& chest);
// Opener attempts the chest: trap, then item attach, then gold. Opener refusal rules unknown (UNKNOWN-16, CONFLICT-8).
// FIXED: ChestOpenResult named no type; replaced by ChestOpenOutcome (defined above in this file).
i32 openChest(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest);
// Receiver of the chest gold: index = CInt(Rnd * (PartySize - 1)) (CONFLICT-9, engine taken).
i32 chestReceiverIndex(Ctx& ctx, i32 partySize);
// Pay chest gold to the party. Payout when opener killed by a trap is unknown (UNKNOWN-16).
void distributeChestGold(Ctx& ctx, Party& party, Currency gold, i32 openerIndex, bool openerKilled);
// Share of gold per member; integer display (CONFLICT-11). Split rounding unknown.
Currency splitChestGold(Currency pot, i32 shareCount);
// Chest curse storage: true when the item is cursed. Kind 1 and 2 are lost (CONFLICT-32).
bool slotCursedFromItem(const Item& item);

// ---- [c33][c55] thief take of gold
// Thief take from purse; zeroing branch and equal-roll case unresolved (UNKNOWN-9, CONFLICT-23).
Currency rollThiefTake(Ctx& ctx, Currency purse);

// ---- [c27][c48][c49] payments and bank
// Price check uses Gold + BankGold; wording versus check unresolved (UNKNOWN-10).
bool canPayFromPurse(const Purse& wallet, Currency price);
// Deposit refused if greater than carried gold or <= 0 (original, CONFLICT-10).
bool bankDepositGold(Purse& wallet, Currency amount);
// Withdrawal refused only if greater than bank balance; negative accepted (port, CONFLICT-10).
bool bankWithdrawGold(Purse& wallet, Currency amount);

// ---- [c106] guild quest assignment
// Quest parameters unverified (F37); "chance" field undefined (UNKNOWN-19); monster set (CONFLICT-33).
void assignGuildQuest(Ctx& ctx, const GameData& data, GuildMembership& membership, i32 guildLevel);

// ---- [c91][UNKNOWN-1] monster drops
// Drop band for a monster. Named laired drops are community-only (c91). Asmodeus band unknown (UNKNOWN-1).
void rollMonsterDrops(Ctx& ctx, const GameData& data, i32 monsterIndex, bool laired, Vec<Item>& out);

// ---- [c22] item alignment and identification
// Draw item alignment 0..3. Only Good, Neutral and Evil named; 0 is Unaligned (UNKNOWN-14, UNKNOWN-17).
ItemAlign rollItemAlignment(Ctx& ctx);
// Shelved when not (floor == 0 and align == Unaligned); test on floor vs alignment unresolved (CONFLICT-14).
bool shelvedBySelling(i32 floor, ItemAlign align);
// Map Known scale (0..3) to IdLevel. Direction unresolved; engine id = 3 - Known (CONFLICT-15, UNKNOWN-13).
IdLevel knownToIdLevel(i32 known);

// ---- [c22][c48] shelf buy-out and replenish
// Remove one item from stock by random draw. maxDraws = kBuyOutDrawCap for the quirk, -1 for unbounded (CONFLICT-13).
bool buyOutRemoveOne(Ctx& ctx, Vec<Item>& stock, i32 maxDraws);
// Shelf replenish mechanics beyond call signature unknown (UNKNOWN-8).
void replenishShelf(Ctx& ctx, const GameData& data, Vec<Item>& shelf);

}  // namespace mordor
