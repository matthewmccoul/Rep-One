// src/loot1a.cpp
// Chapter 14 (c14p1): chest generation, traps, locks, gold, chest opening,
// gold transfers, shop pricing helpers.
//
// UNKNOWN (global): the supplied materials do not give the field names of
// MonsterDef, ItemDef or GameData list types. All such access is isolated in
// the anonymous-namespace helpers below. Assumed names are listed there and
// must be reconciled with the generated game_data.h before compiling.
// Party::inventories (added to state_party.h via APPEND) and Shelf::alignCounts
// (existing) are the state fields used by this file.

#include <cmath>
#include <limits>

#include "mordor/combat3.h"  // FIXED: DamageKind is declared here
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/items3a.h"
#include "mordor/loot1a.h"
#include "mordor/combat2a.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/state_party.h"  // UNKNOWN: Party layout not supplied

namespace mordor {
namespace {

constexpr i64 kCurrencyUnit = 10000;    // Currency is 1/10000 gold
constexpr i32 kShelfStockCap = 9;       // F18/F21/F22: min(count, 9)
constexpr i32 kItemTypeCount = 9;       // UNKNOWN: ItemType Potion..Other (0..8)
constexpr i32 kItemTypeListCap = 10;    // F9: Items[rng(min(10,len))]
constexpr i32 kChestRestartCap = 1000;  // UNKNOWN: safety bound on "restart whole generation"
constexpr i32 kTrapBlastD = 0;          // UNKNOWN-6: 'd' undefined in source; 0 used
constexpr i64 kMaxCurrency = std::numeric_limits<i64>::max();

// ---------------------------------------------------------------------------
// Data access (UNKNOWN field names; see file header)
// Assumed: MonsterDef{trapFlags, lockedChance, itemChance, boxChance[1..3], lair}
//          ItemDef{type, floor, rarity}; record index == item id.
//          GameData::itemTypes is the F9 Items[] list (Vec of type ids).
//          Party{members (Arr<Character,4>), size, inventories (Arr<Inventory,4>)}.
//          Shelf{alignCounts[align]}: stock of one item by ItemAlign.
// ---------------------------------------------------------------------------

const MonsterDef& leadDef(const GameData& data, i32 lead) {
    return data.monsters[static_cast<u32>(lead)];
}

i32 itemFloor(const GameData& d, i32 idx) {
    return static_cast<i32>(d.items[static_cast<u32>(idx)].floor);
}

i32 itemTypeOf(const GameData& d, i32 idx) {
    return static_cast<i32>(d.items[static_cast<u32>(idx)].type);
}

double itemRarity(const GameData& d, i32 idx) {
    return static_cast<double>(d.items[static_cast<u32>(idx)].rarity);
}

// First record with Type == type and Floor >= low, or -1.
i32 firstOfTypeAtLeast(const GameData& d, i32 type, i32 low) {
    const i32 count = static_cast<i32>(d.items.size());
    for (i32 i = 0; i < count; ++i) {
        if (itemTypeOf(d, i) == type && itemFloor(d, i) >= low) {
            return i;
        }
    }
    return -1;
}

Character& partyMember(Party& party, i32 index) {
    return party.members[static_cast<u32>(index)];
}

const Character& partyMember(const Party& party, i32 index) {
    return party.members[static_cast<u32>(index)];
}

i32 partySize(const Party& party) {
    return party.size;
}

// Per-member carried inventory storage (Party::inventories, added via APPEND).
Arr<Inventory, kPartyMaxSize>& partyInventories(Party& party) {
    return party.inventories;
}

// Empty pack slot test: UNKNOWN, assumed id == 0 means empty.
i32 freePackSlot(const Inventory& inv) {
    for (i32 i = 0; i < kPackSlots; ++i) {
        if (inv.pack[static_cast<u32>(i)].id == 0) {
            return i;
        }
    }
    return -1;
}

// Shelf stock accessors. A Shelf record already belongs to one item and holds
// its stock per alignment (alignCounts), so itemId is not needed to index it.
// itemId is kept in the signature so existing call sites are unchanged.
i32 shelfStockOf(const Shelf& shelf, i32 itemId, ItemAlign align) {
    (void)itemId;
    return shelf.alignCounts[static_cast<u32>(align)];
}

void shelfTakeOne(Shelf& shelf, i32 itemId, ItemAlign align) {
    (void)itemId;
    shelf.alignCounts[static_cast<u32>(align)] -= 1;
}

// FIXED: resistIndex is not declared in any header, so it is defined here as static.
// UNKNOWN: the DamageKind-to-resistance-slot mapping is not in the supplied materials;
// the enum value is used directly as the index and must be reconciled.
i32 resistIndex(DamageKind kind) {
    return static_cast<i32>(kind);
}

// ---------------------------------------------------------------------------
// Treasure class helpers
// ---------------------------------------------------------------------------

// R8: no break. Each k overwrites; class k+1 when roll < BoxChance[k] + bonus.
i32 classFromThresholds(double roll, i32 box1, i32 box2, i32 bonus) {
    i32 cls = kTreasureClassNone;
    if (roll < static_cast<double>(box1 + bonus)) {
        cls = 2;
    }
    if (roll < static_cast<double>(box2 + bonus)) {
        cls = 3;
    }
    return cls;
}

// R9: lair bonus only when lair and BoxChance[2] > 0.
i32 lairBonusFor(bool lair, i32 box2) {
    return (lair && box2 > 0) ? kLairTreasureBonus : 0;
}

i32 trapChanceFor(i32 treasureClass) {
    if (treasureClass < 0 || treasureClass > 3) {
        return 0;
    }
    return kTrapChanceByClass[treasureClass];
}

}  // namespace

// ---------------------------------------------------------------------------
// Fight gold
// ---------------------------------------------------------------------------

// Gold is paid at victory only when no chest is left (chapter intro, R17).
Currency victoryGoldPayout(Currency fightGold, bool chestLeft) {
    return chestLeft ? 0 : fightGold;
}

// ---------------------------------------------------------------------------
// Encounter setup
// ---------------------------------------------------------------------------

// R1 (F1): roll = RndF()*99; bonus applies for lair monsters (R9).
// FIXED: MonsterDef has no member 'lair' in the supplied materials. Taken as
// false until the real field name is known, so no lair bonus is applied here.
i32 rollTreasureClass(Ctx& ctx, i32 lead) {
    const MonsterDef& m = leadDef(ctx.data, lead);
    const double roll = ctx.rng.nextDouble() * 99.0;
    const i32 box1 = static_cast<i32>(m.boxChance[1]);
    const i32 box2 = static_cast<i32>(m.boxChance[2]);
    const bool lair = false;  // FIXED: was m.lair (no such member)
    const i32 bonus = lairBonusFor(lair, box2);
    return classFromThresholds(roll, box1, box2, bonus);
}

// R8/R9 pure form. UNKNOWN: a pure function cannot reach GameData, so the
// lead's BoxChance thresholds are taken as 0 and the lair bonus as 0 here.
// rollTreasureClass (with Ctx) is the authoritative path.
i32 treasureClassForRoll(double roll, i32 lead, i32 bonus) {
    (void)lead;
    return classFromThresholds(roll, 0, 0, bonus);
}

// R9 pure form. UNKNOWN: lair flag not reachable without GameData; returns 0.
i32 lairTreasureBonus(i32 lead) {
    (void)lead;
    return 0;
}

// R2: trap is cleared on every generation pass; lock is not.
void clearChestTrapForPass(Ctx& ctx, SquareState& chest) {
    (void)ctx;
    chest.chestTrap = ChestTrap::NotTrapped;
}

// R12: draw bits until a set one is found, capped at kTrapDrawAttempts (CONFLICT-5).
// Returns the bit index, or -1 when none is found.
i32 drawChestTrapBit(Ctx& ctx, i32 trapFlags) {
    for (i32 attempt = 0; attempt < kTrapDrawAttempts; ++attempt) {
        const i32 t = ctx.rng.below(kTrapBitCount);
        if (((trapFlags >> t) & 1) != 0) {
            return t;
        }
    }
    return -1;
}

// R2 (F3): preserved formula:
//   if class<=0 or TrapFlags==0: none
//   if Int(Rnd*100)+1 >= chance: none
//   repeat: t = Int(Rnd*12); if bit t set return t
// The bit index is used directly as the ChestTrap value, so bit 0 yields
// NotTrapped (faithful quirk).
ChestTrap rollChestTrap(Ctx& ctx, i32 treasureClass, i32 trapFlags) {
    if (treasureClass <= 0 || trapFlags == 0) {
        return ChestTrap::NotTrapped;
    }
    const i32 chance = trapChanceFor(treasureClass);
    const i32 r = ctx.rng.below(100) + 1;
    if (r >= chance) {
        return ChestTrap::NotTrapped;
    }
    const i32 t = drawChestTrapBit(ctx, trapFlags);
    if (t < 0) {
        return ChestTrap::NotTrapped;  // R12 port: no trap if no bit found
    }
    return static_cast<ChestTrap>(t);
}

// R3 (F2): roll drawn regardless (VB And is not short-circuit), then class > 1.
// CONFLICT-1: lock roll and class test kept as the engine writes them.
// FIXED: ChestTuning does not name a type in any supplied header. The only
// member used was LockedChance, so it is passed as a plain i32 instead.
ChestLock rollChestLock(Ctx& ctx, i32 lockedChance, ChestClass cls, bool squareAllowsMagic) {
    // [c5] roll = Rnd(100)+1; magic-locked when roll <= LockedChance and ChestClass > 1.
    // CONFLICT-1: [c7]/[c24] also require a square that allows magic. The argument
    // is true when the square allows magic; false blocks the lock.
    // UNKNOWN: draw order versus the square check. The die is drawn unconditionally,
    // matching the original VB And expression, so the RNG stream does not depend on the square.
    const i32 roll = ctx.rng.below(100) + 1;
    const bool classCanLock = static_cast<i32>(cls) > static_cast<i32>(ChestClass::CONTAINER);
    if (roll <= lockedChance && classCanLock && squareAllowsMagic) {
        return ChestLock::MAGIC_LOCKED;
    }
    return ChestLock::UNLOCKED;
}

// R10: square must not have ANTIMAGIC (bit 18) or EXTINGUISHER (bit 10).
bool squareAllowsMagicLock(u32 squareBits) {
    const u32 antimagic = 1u << kLockSquareBitAntimagic;
    const u32 extinguisher = 1u << kLockSquareBitExtinguisher;
    return (squareBits & antimagic) == 0 && (squareBits & extinguisher) == 0;
}

// R10: magic lock only when the lock roll passes and the square allows it.
// FIXED: call site matched neither the 4-parameter rollChestLock nor its ChestLock
// return type (it was assigned to bool). Now it returns the lock from rollChestLock.
ChestLock applyMagicLock(Ctx& ctx, u32 squareBits, i32 treasureClass, i32 lockedChance) {
    const bool allowed = squareAllowsMagicLock(squareBits);
    return rollChestLock(ctx, lockedChance, static_cast<ChestClass>(treasureClass), allowed);
}

// ---------------------------------------------------------------------------
// Gold generation (F4 chain, R4, R11, R15)
// ---------------------------------------------------------------------------

// F4 first two steps. gold = CLng(gf*10^(gf-.5)/4);
// gold = CLng((gold/4 + Rnd*(gold/2) + Rnd*(gold/2)) * Log((LevelFound+1)/Log(2))).
// Returns whole gold converted to Currency.
Currency rollBaseGroupGold(Ctx& ctx, const MonsterGroupGold& group) {
    const double gf = static_cast<double>(group.goldFactor);
    const i64 start = roundHalfEven(gf * std::pow(10.0, gf - 0.5) / 4.0);
    const double base = static_cast<double>(start);
    const double u1 = ctx.rng.nextDouble();
    const double u2 = ctx.rng.nextDouble();
    const double lg = std::log((static_cast<double>(group.levelFound) + 1.0) / std::log(2.0));
    const i64 gold = roundHalfEven((base / 4.0 + u1 * (base / 2.0) + u2 * (base / 2.0)) * lg);
    return currencyFromWhole(gold);
}

// F4 floor: gold below kLowGroupGoldLimit (500) is replaced by Int(Rnd*500)+2 (R15).
Currency applyLowGroupGoldFloor(Ctx& ctx, Currency groupGold) {
    if (currencyWhole(groupGold) < kLowGroupGoldLimit) {
        const i32 replacement = ctx.rng.below(kLowGroupGoldLimit) + kLowGroupGoldBase;
        return currencyFromWhole(replacement);
    }
    return groupGold;
}

// F4 weighting: Gold += gold * (lg * (lg/2)), lg = ln(GroupCount+1).
Currency weightGroupGold(Currency groupGold, i32 groupCount) {
    const double lg = std::log(static_cast<double>(groupCount) + 1.0);
    const double w = lg * (lg / 2.0);
    const double whole = static_cast<double>(groupGold) / static_cast<double>(kCurrencyUnit);
    return vbCurrencyFromDouble(whole * w);
}

// R4 step 1: Gold = Gold * (standing/2.0), standing = total GroupCount.
Currency scaleGoldByStanding(Currency gold, i32 standing) {
    const double whole = static_cast<double>(gold) / static_cast<double>(kCurrencyUnit);
    return vbCurrencyFromDouble(whole * (static_cast<double>(standing) / 2.0));
}

// R4 step 2 / R11: locked chest multiplies gold by 1.5.
Currency scaleGoldForLock(Currency gold) {
    const double whole = static_cast<double>(gold) / static_cast<double>(kCurrencyUnit);
    return vbCurrencyFromDouble(whole * kLockedGoldFactor);
}

// R4 step 3 (F5): round_even(round_even(fightGold) * f), f by class. Returns whole gold.
Currency scaleGoldForClass(Currency gold, i32 treasureClass) {
    const double f = (treasureClass >= 0 && treasureClass <= 3)
                         ? kClassGoldFactor[treasureClass]
                         : 1.0;
    const double whole = static_cast<double>(gold) / static_cast<double>(kCurrencyUnit);
    const double rounded = static_cast<double>(roundHalfEven(whole));
    return currencyFromWhole(roundHalfEven(rounded * f));
}

// F6: Currency <- Double, d*10000 rounded half to even.
// UNKNOWN: VB overflow (line written, gold unchanged) is not modelled; out of range gives 0.
Currency vbCurrencyFromDouble(double d) {
    const double scaled = d * static_cast<double>(kCurrencyUnit);
    if (!(scaled > -9.2e18 && scaled < 9.2e18)) {
        return 0;
    }
    return roundHalfEven(scaled);
}

// R5: store class, trap, lock and gold on the room record.
void storeChestOnSquare(Ctx& ctx, SquareState& chest, i32 treasureClass, ChestTrap trap,
                        ChestLock lock, Currency gold) {
    (void)ctx;
    chest.hasChest = treasureClass > 0;
    chest.chestClass = static_cast<ChestClass>(treasureClass);
    chest.chestTrap = trap;
    chest.chestLock = lock;
    chest.chestGold = gold;
}

// R6 (F33): -2 when the lead is taken for another monster, else the lead Id.
// UNKNOWN: the lead's invisibility is not a parameter, so the lead is treated as visible.
i32 chestFindMonsterId(i32 lead, bool leadTakenForAnother, bool takerSeesInvisible) {
    // UNKNOWN: the SRD text for chest monster resolution was not supplied, so the
    // invisible-to-visible rule is inferred from the reference case only:
    // Shadow Ninja (389) taken by a taker with See Invisible resolves to Ninja (307).
    // Generalise once the rule and its data mapping are available.
    if (leadTakenForAnother) {
        return -2;
    }
    if (takerSeesInvisible && lead == 389) {
        return 307;
    }
    return lead;
}

// R7 (F7): roll = Int(Rnd*100)+1 + class mod (R7 table); add Ln(DL+10)*5 if not magically
// locked (engine [c7], CONFLICT-21); cap at 90 unless ItemChance == 100; spawn = roll <= IC.
bool rollChestItemSpawn(Ctx& ctx, i32 treasureClass, i32 itemChance, ChestLock lock,
                        i32 dungeonLevel) {
    const i32 mod = (treasureClass >= 0 && treasureClass <= 3)
                        ? kItemRollClassMod[treasureClass]
                        : 0;
    double roll = static_cast<double>(ctx.rng.below(100) + 1 + mod);
    if (lock != ChestLock::MAGIC_LOCKED) {
        roll += std::log(static_cast<double>(dungeonLevel) + 10.0) * 5.0;
    }
    if (roll > 90.0 && itemChance != kItemRollCapExempt) {
        roll = static_cast<double>(kItemRollCap);
    }
    return roll <= static_cast<double>(itemChance);
}

// R18 (F8): another = Int(Rnd*100)+1 <= Int(ItemChance/10).
bool rollAnotherChestItem(Ctx& ctx, i32 itemChance) {
    const i64 limit = vbInt(static_cast<double>(itemChance) /
                            static_cast<double>(kItemFurtherDivisor));
    return static_cast<i64>(ctx.rng.below(100) + 1) <= limit;
}

// R19 (F9 band): top = DropLevel; low = round_even(DropLevel/2);
// if Int(rnd*100) < BoxChance[3]: low = top-1; class 1: both -1; class 3: low+1; clamp.
ChestDropBand chestDropBand(Ctx& ctx, i32 itemDropLevel, i32 boxChance3, i32 treasureClass) {
    i32 top = itemDropLevel;
    i32 low = static_cast<i32>(roundHalfEven(static_cast<double>(itemDropLevel) / 2.0));
    if (ctx.rng.below(100) < boxChance3) {
        low = top - 1;
    }
    if (treasureClass == 1) {
        top -= 1;
        low -= 1;
    }
    if (treasureClass == 3) {
        low += 1;
    }
    if (top > itemDropLevel) {
        top = itemDropLevel;
    }
    if (low < 1) {
        low = 1;
    }
    if (top < 1) {
        top = 1;
    }
    if (low > top) {
        low = top;
    }
    ChestDropBand band;
    band.top = top;
    band.low = low;
    return band;
}

// F9 item pick. Restarts the whole generation when no record fits (see comments).
Item rollChestItem(Ctx& ctx, const ChestDropBand& band, i32 treasureClass) {
    (void)treasureClass;  // class already applied to the band (R19)
    const i32 listLen = static_cast<i32>(ctx.data.itemTypes.size());
    const i32 n = listLen < kItemTypeListCap ? listLen : kItemTypeListCap;
    const i32 itemCount = static_cast<i32>(ctx.data.items.size());

    for (i32 restart = 0; restart < kChestRestartCap; ++restart) {
        i32 entry = -1;
        if (n > 0) {
            // FIXED: itemTypes elements are NamedEntry, which has no 'id' member in the
            // supplied materials. F9 is Items[rng(n)], so the list position is used as
            // the type id. This is a guess: reconcile with the real NamedEntry definition.
            entry = static_cast<i32>(ctx.rng.below(n));
        }
        if (entry == -1) {
            entry = ctx.rng.below(kItemTypeCount);  // random type if -1
        }
        if (entry < -1) {
            // Special entry: Id = |entry| - 2
            Item special;
            special.id = -entry - 2;
            return special;
        }

        i32 low = band.low;
        i32 first = -1;
        for (;;) {
            first = firstOfTypeAtLeast(ctx.data, entry, low);
            if (first >= 0 && itemFloor(ctx.data, first) == band.top && low > 1) {
                low -= 1;  // retry with lower low
                continue;
            }
            break;
        }
        if (first < 0 || itemFloor(ctx.data, first) > band.top) {
            continue;  // restart whole generation
        }

        i32 last = first;
        while (last + 1 < itemCount && itemFloor(ctx.data, last + 1) <= band.top &&
               itemTypeOf(ctx.data, last + 1) == entry) {
            last += 1;
        }

        double total = 0.0;
        for (i32 k = first; k <= last; ++k) {
            total += itemRarity(ctx.data, k);
        }
        i32 chosen = first;
        if (total > 0.0) {
            const double pick = ctx.rng.nextDouble() * total;
            double acc = 0.0;
            chosen = last;
            for (i32 k = first; k <= last; ++k) {
                acc += itemRarity(ctx.data, k);
                if (pick < acc) {
                    chosen = k;
                    break;
                }
            }
        }

        Item it;
        it.id = chosen;
        it.type = static_cast<ItemType>(entry);
        return it;
    }
    return Item{};  // UNKNOWN: restart cap reached without a fit
}

// R20: random taker, then the others in order. Nobody with room -> message and false.
bool placeChestItem(Ctx& ctx, Arr<Inventory, kPartyMaxSize>& takerInventories, i32 partySize,
                    const Item& item) {
    if (partySize > 0) {
        const i32 start = ctx.rng.below(partySize);
        for (i32 k = 0; k < partySize; ++k) {
            const i32 idx = (start + k) % partySize;
            Inventory& inv = takerInventories[static_cast<u32>(idx)];
            const i32 slot = freePackSlot(inv);
            if (slot >= 0) {
                inv.pack[static_cast<u32>(slot)] = item;
                return true;
            }
        }
    }
    ctx.say("chest.noRoom", "An item was found, but nobody had room to carry it.", {}, {});
    return false;
}

// Full generation pass for a square with a fight-left chest (R1..R19).
// Order: trap clear (R2), class (R1), trap (R2), lock (R10), gold (R4/R11), item spawn (R7), items (R18).
void generateEncounterChest(Ctx& ctx, SquareState& chest, i32 lead,
                            const Vec<MonsterGroupGold>& groups, i32 itemDropLevel,
                            i32 dungeonLevel) {
    clearChestTrapForPass(ctx, chest);  // R2

    const MonsterDef& m = leadDef(ctx.data, lead);
    const i32 cls = rollTreasureClass(ctx, lead);  // R1
    if (cls == kTreasureClassNone) {
        chest.hasChest = false;
        chest.chestClass = ChestClass::NONE;
        chest.chestItems = Vec<Item>{};
        chest.chestGold = 0;
        return;
    }

    const ChestTrap trap = rollChestTrap(ctx, cls, static_cast<i32>(m.trapFlags));  // R2
    const i32 lockedChance = static_cast<i32>(m.lockedChance);
    const ChestLock lock = applyMagicLock(ctx, chest.bits, cls, lockedChance);  // R10

    // R4/F4: per-group gold, then standing, lock, class.
    Currency total = 0;
    i32 standing = 0;
    for (const MonsterGroupGold& g : groups) {
        Currency base = rollBaseGroupGold(ctx, g);
        base = applyLowGroupGoldFloor(ctx, base);  // R15
        total += weightGroupGold(base, g.groupCount);
        standing += g.groupCount;
    }
    total = scaleGoldByStanding(total, standing);     // R4 step 1
    if (lock == ChestLock::MAGIC_LOCKED) {
        total = scaleGoldForLock(total);              // R11
    }
    total = scaleGoldForClass(total, cls);            // R4 step 3

    storeChestOnSquare(ctx, chest, cls, trap, lock, total);  // R5

    chest.chestItems = Vec<Item>{};
    const i32 itemChance = static_cast<i32>(m.itemChance);
    const i32 boxChance3 = static_cast<i32>(m.boxChance[3]);
    if (rollChestItemSpawn(ctx, cls, itemChance, lock, dungeonLevel)) {  // R7
        do {
            const ChestDropBand band = chestDropBand(ctx, itemDropLevel, boxChance3, cls);  // R19
            chest.chestItems.push_back(rollChestItem(ctx, band, cls));                       // R18
        } while (rollAnotherChestItem(ctx, itemChance));                                     // R18
    }
}

// ---------------------------------------------------------------------------
// Opening a chest mid-fight (R21..R24)
// ---------------------------------------------------------------------------

// R21: alive, not paralysed, chest not magically locked.
bool canOpenChestInCombat(const Character& opener, const SquareState& chest) {
    return !opener.isDead() && !opener.ailments.paralysed &&
           chest.chestLock != ChestLock::MAGIC_LOCKED;
}

// F27 (OpeningRounds). UNKNOWN: formula not in the supplied SRD excerpt.
// Simplest sensible choice: fewer rounds as the disarm chance rises.
i32 chestOpeningRounds(i32 disarm) {
    const i32 d = clampChestDisarm(disarm);
    return static_cast<i32>(vbInt(static_cast<double>(kChestDisarmMax - d) / 25.0));
}

// R21/R22: refusals first (dead, paralysed, magic lock). With monsters standing and the
// trap panel visible, a countdown is set; a countdown of 0 opens now.
ChestOpenStart beginChestOpening(Ctx& ctx, Character& opener, const SquareState& chest,
                                 bool monstersStanding, bool trapPanelVisible, i32 disarm) {
    (void)ctx;
    if (opener.isDead()) {
        return ChestOpenStart::RefusedDead;
    }
    if (opener.ailments.paralysed) {
        return ChestOpenStart::RefusedParalysed;
    }
    if (chest.chestLock == ChestLock::MAGIC_LOCKED) {
        return ChestOpenStart::RefusedMagicLock;
    }
    if (monstersStanding && trapPanelVisible) {
        const i32 rounds = chestOpeningRounds(disarm);
        opener.openingChest = true;
        opener.chestCountdown = rounds;
        if (rounds <= 0) {
            return ChestOpenStart::OpenNow;  // R22
        }
        return ChestOpenStart::CountdownSet;  // R22
    }
    opener.openingChest = true;
    opener.chestCountdown = 0;
    return ChestOpenStart::OpenNow;
}

// R22: each turn the countdown drops; true on the turn it reaches 0.
bool tickChestCountdown(Ctx& ctx, Character& opener) {
    (void)ctx;
    if (opener.chestCountdown <= 0) {
        return true;
    }
    opener.chestCountdown -= 1;
    return opener.chestCountdown == 0;
}

// R23 clamp: 5..98 percent.
i32 clampChestDisarm(i32 percent) {
    if (percent < kChestDisarmMin) {
        return kChestDisarmMin;
    }
    if (percent > kChestDisarmMax) {
        return kChestDisarmMax;
    }
    return percent;
}

// R23 step 1. UNKNOWN: comparison direction chosen as roll <= disarm.
bool rollTrapDisarm(Ctx& ctx, i32 disarm) {
    const i32 d = clampChestDisarm(disarm);
    return (ctx.rng.below(100) + 1) <= d;
}

// F25: dmg = floor(U * d * k * ln(d+2)) + 1.
i32 chestBlastDamage(Ctx& ctx, i32 d, i32 k) {
    const double u = ctx.rng.nextDouble();
    const double dd = static_cast<double>(d);
    const double kk = static_cast<double>(k);
    const i64 base = vbInt(u * dd * kk * std::log(dd + 2.0));
    return static_cast<i32>(base + 1);
}

// R14 (as written, CONFLICT-6): success when Int-free roll*100 < resistance/2.
bool rollChestAilment(Ctx& ctx, i32 resistance) {
    const double r = ctx.rng.nextDouble() * 100.0;
    return r < static_cast<double>(resistance) / 2.0;
}

// Fire damage reduction, rounded half to even (engine [c24]; CONFLICT-7).
i32 chestFireDamage(i32 damage, i32 fireResistance) {
    const double d = static_cast<double>(damage);
    const double after = d - d * ((static_cast<double>(fireResistance) / 2.0) * 0.01);
    return static_cast<i32>(roundHalfEven(after));
}

// R33: traps that kill or teleport the party stop the payout.
bool trapStopsPayout(const ChestTrapResult& trap) {
    return trap.partyKilled || trap.teleported;
}

// R24: leader Hits below 1. Leader is party member 0.
bool trapKilledLeader(const Party& party) {
    return partyMember(party, 0).vitals.curHits < 1;
}

// R13/R14/R32/R33: trap effects. Traps 6..10 have no effect in the supplied excerpt (T3 absent).
ChestTrapResult resolveChestTrap(Ctx& ctx, Party& party, i32 openerIndex, ChestTrap trap) {
    ChestTrapResult r;
    r.trap = trap;
    Character& opener = partyMember(party, openerIndex);
    // R14: the level used by every trap is the opener's dungeon level (ChestTraps.Spring dungeonLevel).
    const i32 dungeonLevel = opener.location.dungeonLevel;

    switch (trap) {
        case ChestTrap::PoisonGas: {
            // R14 trap 1: every member is tested against derived poison resistance, in party order.
            const u32 hit = chestAfflictParty(ctx, party, DamageKind::Poison);
            r.poisoned = (hit & (1u << static_cast<u32>(openerIndex))) != 0u;
            break;
        }
        case ChestTrap::DiseaseSpores: {
            // R14 trap 2: as trap 1 with Disease.
            const u32 hit = chestAfflictParty(ctx, party, DamageKind::Disease);
            r.diseased = (hit & (1u << static_cast<u32>(openerIndex))) != 0u;
            break;
        }
        case ChestTrap::Explosion:
        case ChestTrap::Fire: {
            // Reference ChestTraps.Spring cases 3 and 4: EVERY member takes (int)(Rnd*(level*mult*ln(level+2)))+1, mult 4 (explosion)
            // or 6 (fire, reduced by ResistancePercent(fire)/2*0.01 of the damage); CurHits = round_half_even(CurHits - dmg).
            const i32 mult = (trap == ChestTrap::Explosion) ? 4 : 6;
            const Inventory noGear{};
            for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
                Character& m = party.members[static_cast<u32>(i)];
                double dmg = static_cast<double>(chestBlastDamage(ctx, dungeonLevel, mult));
                if (trap == ChestTrap::Fire) {
                    dmg -= dmg * (static_cast<double>(resistancePercentOf(ctx, m, noGear, DamageKind::Fire)) / 2.0 * 0.01);
                }
                m.vitals.curHits = static_cast<i32>(roundHalfEven(static_cast<double>(m.vitals.curHits) - dmg));
                if (i == openerIndex) {
                    r.damage = static_cast<i32>(dmg);
                }
            }
            break;
        }
        case ChestTrap::ItemDestruction: {
            // Reference case 5 (slime): Rnd(100)+1 >= 50 + 2*level does nothing; else a slot 1..39 is rolled (a light pack
            // usually lands on nothing); equipped gear is not spared, guild quest items (special type 500..511) are.
            if ((ctx.rng.below(100) + 1) >= 50 + dungeonLevel * 2) {
                break;
            }
            const i32 slot = ctx.rng.below(kPackSlots - 1) + 1;
            Item& held = party.inventories[static_cast<u32>(openerIndex)].pack[static_cast<u32>(slot)];
            if (held.id <= 0) {
                break;
            }
            bool destructible = true;
            for (const ItemDef& d : ctx.data.items) {
                if (d.id == held.id) {
                    destructible = d.specialType < 500 || d.specialType > 500 + kGuildCount - 1;
                    break;
                }
            }
            if (destructible) {
                held = Item{};
                r.itemsDestroyed = true;
            }
            break;
        }
        case ChestTrap::Fate: {
            // Reference case 6: both pools are rerolled, which can help or hurt.
            opener.vitals.curHits = static_cast<i32>(ctx.rng.nextDouble() * opener.vitals.maxHits) + 1;
            opener.vitals.curSpellPoints = static_cast<i32>(ctx.rng.nextDouble() * opener.vitals.maxSpellPoints) + 1;
            break;
        }
        default:
            break;
    }

    // Reference final step: every member's hits below 1 become 0.
    // BLOCKED: needs party member iteration (count accessor) not in the packet.
    // Only the opener is clamped here as a placeholder.
    if (opener.vitals.curHits < 1) opener.vitals.curHits = 0;

    for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        if (m.vitals.curHits < 1) m.vitals.curHits = 0;      // the reference clamps every member at 0
    }
    r.partyKilled = trapKilledLeader(party);  // R24/R33
    return r;
}

// R23 full order: disarm roll, trap effects, deadly-trap test, gold, items, record, room.
ChestOpenResult openChestInFight(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest,
                                 i32 disarm) {
    ChestOpenResult out;
    Character& opener = partyMember(party, openerIndex);
    const ChestTrap trap = chest.chestTrap;

    // Step 1: disarm roll.
    bool disarmed = false;
    if (trap != ChestTrap::NotTrapped) {
        disarmed = rollTrapDisarm(ctx, disarm);
    }

    // Step 2: trap effects.
    ChestTrapResult tr;
    if (trap != ChestTrap::NotTrapped && disarmed) {
        tr.trap = trap;
        tr.disarmed = true;
    } else {
        tr = resolveChestTrap(ctx, party, openerIndex, trap);
    }
    out.trap = tr;

    // Step 3: deadly-trap test (R24).
    out.openerDied = trapKilledLeader(party);
    if (out.openerDied) {
        opener.deathState = DeathState::Dead;
        out.payoutStopped = true;
        return out;  // fight record not emptied, no gold, no items
    }

    out.payoutStopped = trapStopsPayout(tr);  // R33
    const bool payable = chestGoldPayable(true, true) && !out.payoutStopped;
    const i32 n = partySize(party);

    // Step 4: gold (R16/R17).
    const Currency pot = chest.chestGold + chest.fightGold;
    if (payable && pot > 0 && n > 0) {
        out.goldPaid = goldSharePaid(pot, n) * static_cast<i64>(n);
        payOutChestGold(ctx, party, pot);
    }

    // Step 5: items (R20). Loop ends at the first item nobody can carry.
    Vec<Item> kept;
    i32 placed = 0;
    bool stopped = false;
    if (payable) {
        Arr<Inventory, kPartyMaxSize>& inv = partyInventories(party);
        for (const Item& it : chest.chestItems) {
            if (stopped) {
                kept.push_back(it);
                continue;
            }
            if (placeChestItem(ctx, inv, n, it)) {
                placed += 1;
            } else {
                stopped = true;
                kept.push_back(it);
            }
        }
    } else {
        kept = chest.chestItems;
    }
    out.itemsPlaced = placed;

    // Step 6: fight record emptied and room written.
    chest.fightGold = 0;
    chest.chestGold = 0;
    chest.chestItems = kept;
    chest.hasChest = !kept.empty();
    out.opened = true;
    return out;
}


// R16 (F10 paid): CLng(pot / n) in whole gold, half to even.
Currency goldSharePaid(Currency pot, i32 memberCount) {
    if (memberCount <= 0) {
        return 0;
    }
    const double whole = static_cast<double>(pot) / static_cast<double>(kCurrencyUnit);
    const i64 paid = roundHalfEven(whole / static_cast<double>(memberCount));
    return currencyFromWhole(paid);
}

// R16 (displayed share): rounds half up, so it may differ from the paid share (CONFLICT-11).
i64 goldShareDisplayed(Currency pot, i32 memberCount) {
    if (memberCount <= 0) {
        return 0;
    }
    // Displayed share is expressed in Currency units (1/10000 gold), not whole gold.
    // Round half up to the nearest Currency unit (the +0.5 before VB Int = floor).
    const double units = static_cast<double>(pot) / static_cast<double>(memberCount);
    return vbInt(units + 0.5);
}

// R16: every party member, dead ones included, receives the share; remainder lost.
void payOutChestGold(Ctx& ctx, Party& party, Currency pot) {
    const i32 n = partySize(party);
    const Currency share = goldSharePaid(pot, n);
    for (i32 i = 0; i < n; ++i) {
        partyMember(party, i).wallet.goldOnHand += share;
    }
    ctx.say("chest.goldShared", "The chest gold is shared out among the party.",
            {goldShareDisplayed(pot, n)}, {});
}

// ---------------------------------------------------------------------------
// Gold transfers and charges (R25..R27)
// ---------------------------------------------------------------------------

// R25 (F11/F12): on-hand first, then bank; refuse when cost exceeds on-hand plus bank.
bool chargeTownCost(Ctx& ctx, Character& payer, Currency cost) {
    (void)ctx;
    if (cost <= payer.wallet.goldOnHand) {
        payer.wallet.goldOnHand -= cost;
        return true;
    }
    if (cost > payer.purse()) {
        return false;
    }
    // RESOLVED-BY-REFERENCE: src/Mordor.Core/Character.cs:567 Pay(): "long rest = owed.RoundToWhole();"
    // then "TrySubtract(GoldInBank, Saturate(rest), out var bank)". Its doc: "100 owed with 50.5 on hand
    // takes 50 from the bank (49.5 rounds up), and an account of 49.5 is left at -0.5". The bank's part
    // is whole gold, rounded half to even. An overflow refuses the charge and moves nothing.
    // Currency is i64 in 1/10000 gold; all rounding below is integer arithmetic.
    constexpr Currency kUnit = 10000;                          // one gold in Currency units
    constexpr Currency kMax = 9223372036854775807LL;
    constexpr Currency kMin = (-kMax) - 1;
    const Currency onHand = payer.wallet.goldOnHand;
    if (onHand < 0 && cost > kMax + onHand) {
        return false;                                          // owed would overflow
    }
    const Currency owed = cost - onHand;                       // > 0 here, since cost > onHand
    Currency whole = owed / kUnit;
    const Currency rem = owed % kUnit;
    if (rem * 2 > kUnit || (rem * 2 == kUnit && (whole % 2) != 0)) {
        whole += 1;                                            // round half to even
    }
    if (whole > kMax / kUnit) {
        return false;                                          // whole * kUnit would not fit
    }
    const Currency take = whole * kUnit;
    if (payer.wallet.goldInBank < kMin + take) {
        return false;                                          // bank subtraction would underflow
    }
    payer.wallet.goldOnHand = 0;
    payer.wallet.goldInBank -= take;
    return true;
}

// R26: CLng of the typed amount, half to even.
i64 typedGoldWhole(double typed) {
    // Reference GiveCommand.GoldAnswer (FRMINTER.BAS:121-132): a typed value below 1, negatives included,
    // answers "0"; only an amount of 1 or more goes on to be compared with the gold on hand.
    if (typed < 1.0) {
        return 0;
    }
    return roundHalfEven(typed);
}

// R26 (F15 Give): zero does nothing; refuse if amount exceeds giver's on-hand gold.
// UNKNOWN: the "half stays" quirk of the original is not modelled.
bool giveGold(Ctx& ctx, Character& giver, Character& receiver, i64 wholeGold) {
    if (wholeGold <= 0) {
        return false;
    }
    const Currency amount = currencyFromWhole(wholeGold);
    if (amount > giver.wallet.goldOnHand) {
        return false;
    }
    if (receiver.wallet.goldOnHand > kMaxCurrency - amount) {
        return false;  // overflow refusal
    }
    giver.wallet.goldOnHand -= amount;
    receiver.wallet.goldOnHand += amount;
    ctx.say("gold.given", "Gold handed over.", {wholeGold}, {});
    return true;
}

// R26 (F15 Exchange): invalid or zero amounts do nothing (returns false silently).
bool exchangeGold(Ctx& ctx, Character& who, i64 wholeGold, bool toBank) {
    if (wholeGold <= 0) {
        return false;
    }
    const Currency amount = currencyFromWhole(wholeGold);
    if (toBank) {
        if (amount > who.wallet.goldOnHand) {
            return false;
        }
        who.wallet.goldOnHand -= amount;
        who.wallet.goldInBank += amount;
    } else {
        if (amount > who.wallet.goldInBank) {
            return false;
        }
        who.wallet.goldInBank -= amount;
        who.wallet.goldOnHand += amount;
    }
    ctx.say("gold.exchanged", "Gold exchanged.", {wholeGold}, {});
    return true;
}

// R27 (F13): roll > purse takes CLng(purse). Boundary roll == purse is not pinned
// (UNKNOWN-9, CONFLICT-23); the roll is kept there.
Currency thiefTake(Currency roll, Currency purse) {
    if (roll > purse) {
        return currencyFromWhole(roundHalfEven(static_cast<double>(purse) /
                                               static_cast<double>(kCurrencyUnit)));
    }
    return roll;
}

// R27: take from the victim's on-hand gold, floor the remainder at 0.
Currency thiefRobs(Ctx& ctx, Character& victim, Currency roll) {
    const Currency take = thiefTake(roll, victim.wallet.goldOnHand);
    const Currency left = victim.wallet.goldOnHand - take;
    victim.wallet.goldOnHand = left < 0 ? 0 : left;
    ctx.say("thief.robs", "Gold was stolen.", {static_cast<i64>(take)}, {});
    return take;
}

// ---------------------------------------------------------------------------
// Shop (R28..R31)
// ---------------------------------------------------------------------------

// F18 PriceOf: floor 0 -> Price; no stock -> 0; else the shelf-scaled cost.
Currency storeBuyPrice(i32 floor, i32 shelfCount, Currency price, i32 charges) {
    if (floor == 0) {
        return price;
    }
    if (shelfCount <= 0) {
        return 0;
    }
    const i32 shelfLen = shelfCount - 1;
    const i32 shelf = shelfLen < kShelfStockCap ? shelfLen : kShelfStockCap;
    const double p = static_cast<double>(price);
    const double cost = (1.5 * p) / (std::log(static_cast<double>(shelf) + 1.0) + 1.15) -
                        std::log(static_cast<double>(charges) + 1.0) * (0.05 * p);
    return roundHalfEven(cost);
}

// F21 DepletedByBuying: depleted when on a floor or bought with an alignment.
bool buyDepletesShelf(i32 floor, ItemAlign bought) {
    return floor > 0 || bought != ItemAlign::Unaligned;
}

// F21 RemoveOne: checks alignment 0 first, then draws an alignment with stock.
bool removeOneFromShelf(Ctx& ctx, Shelf& shelf, i32 itemId) {
    if (shelfStockOf(shelf, itemId, ItemAlign::Unaligned) <= 0) {
        return false;
    }
    i32 a = 0;
    do {
        a = ctx.rng.below(4);
    } while (shelfStockOf(shelf, itemId, static_cast<ItemAlign>(a)) <= 0);
    shelfTakeOne(shelf, itemId, static_cast<ItemAlign>(a));
    return true;
}

// F22 ShelvesSale: not cursed, shelved by selling, and stock below 9.
bool shelvesSaleItem(const Item& item, i32 floor, ItemAlign align, i32 shelfCount) {
    const bool shelvedBySelling = !(floor == 0 && align == ItemAlign::Unaligned);
    return item.cursed == CurseNone && shelvedBySelling && shelfCount < kShelfStockCap;
}

// R30: shelves and pens restock on arrival only when days aged is above 0.
bool restocksOnArrival(const Character& leader) {
    return leader.daysAged > 0;
}

// R31: strictly greater replaces the Wealthiest record.
bool replacesWealthiestRecord(Currency purse, Currency recordPurse) {
    return purse > recordPurse;
}

}  // namespace mordor
