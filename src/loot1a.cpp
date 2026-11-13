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
#include "mordor/shared_helpers.h"

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

Character& memberAt(Party& party, i32 index) {
    return party.members[static_cast<u32>(index)];
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

// F25: dmg = floor(U * d * k * ln(d+2)) + 1.
i32 chestBlastDamage(Ctx& ctx, i32 d, i32 k) {
    const double u = ctx.rng.nextDouble();
    const double dd = static_cast<double>(d);
    const double kk = static_cast<double>(k);
    const i64 base = vbInt(u * dd * kk * std::log(dd + 2.0));
    return static_cast<i32>(base + 1);
}

// R24: leader Hits below 1. Leader is party member 0.
bool trapKilledLeader(const Party& party) {
    return memberAt(party, 0).vitals.curHits < 1;
}

// R13/R14/R32/R33: trap effects. Traps 6..10 have no effect in the supplied excerpt (T3 absent).
ChestTrapResult resolveChestTrap(Ctx& ctx, Party& party, i32 openerIndex, ChestTrap trap) {
    ChestTrapResult r;
    r.trap = trap;
    Character& opener = memberAt(party, openerIndex);
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

// ---------------------------------------------------------------------------
// Gold transfers and charges (R25..R27)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Shop (R28..R31)
// ---------------------------------------------------------------------------

}  // namespace mordor
