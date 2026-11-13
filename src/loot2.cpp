// loot2.cpp: SRD c14p2 chests, traps, locks, chest gold, thief take, bank,
// guild quest, item alignment, shelf and identify-level helpers.
//
// BLOCKER (not resolved in this file): ChestOpenResult and ChestOpenOutcome are not
// declared in any listed header. openChest() returns a plain i32 code; see
// the FIXED comment at openChest. loot2.h must declare openChest with the same
// i32 return type.
//
// UNKNOWN stubs (SRD part c14p2 gives no formula, only notes):
//   rollChestItems, rollChestGold, rollMonsterDrops, assignGuildQuest,
//   replenishShelf, and several trap effects in applyChestTrap.
#include "mordor/loot2.h"
#include "mordor/loot1a.h"
#include "mordor/rng.h"
#include "mordor/combat2a.h"  // FIXED: resolveChestTrap is declared in combat2a.h

#include <cmath>

namespace mordor {
namespace {

// ln(max(1, v)) guard used by the trap identify formula (CONFLICT-24).
i32 atLeastOne(i32 v) {
    return v < 1 ? 1 : v;
}

// Lock rule gate: ChestClass > 1, i.e. BOX, CHEST or ASMODEUS (CONFLICT-1 / [c5]).
bool classAllowsLock(ChestClass cls) {
    return static_cast<i32>(cls) > 1;
}

}  // namespace

// ---------------------------------------------------------------------------
// Chest class and lock
// ---------------------------------------------------------------------------

ChestClass rollChestClass(Ctx& ctx, const ChestTuning& tuning, bool lair) {
    // UNKNOWN: class thresholds not given in the provided SRD text. Cumulative
    // BoxChance[0..2] assumed (CONTAINER, BOX, CHEST); otherwise NONE.
    // CONFLICT-2 / CONFLICT-3: lair adds kLairBoxBonus to BoxChance[2] when > 0.
    i32 b2 = tuning.boxChance[2];
    if (lair && b2 > 0) {
        b2 += kLairBoxBonus;
    }
    i32 t0 = tuning.boxChance[0];
    i32 t1 = t0 + tuning.boxChance[1];
    i32 t2 = t1 + b2;
    i32 r = static_cast<i32>(ctx.rng.below(100)) + 1;  // Rnd(100)+1
    if (r <= t0) return ChestClass::CONTAINER;
    if (r <= t1) return ChestClass::BOX;
    if (r <= t2) return ChestClass::CHEST;
    return ChestClass::NONE;
}

ChestLock rollChestLock(Ctx& ctx, const ChestTuning& tuning, ChestClass cls, bool squareAllowsMagic) {
    // [c5][c7] R10: lock only when rng+1 <= LockedChance, ChestClass > 1, and the square
    // allows magic (not ANTIMAGIC / EXTINGUISHER). The rng draw happens first, as in the
    // engine expression order. The square check is applied after the roll, so the draw is
    // consumed whether or not the square allows magic.
    // BoxChance[3] is the item band (ch14 legend), not a lock threshold, so it is not used here.
    const i32 r = static_cast<i32>(ctx.rng.below(100)) + 1;
    if (!(r <= tuning.lockedChance && classAllowsLock(cls))) {
        return ChestLock::UNLOCKED;
    }
    if (!squareAllowsMagic) {
        return ChestLock::UNLOCKED;  // UNKNOWN: non-magic lock state not named in ChestLock
    }
    return ChestLock::MAGIC_LOCKED;
}

// ---------------------------------------------------------------------------
// Trap bits
// ---------------------------------------------------------------------------

i32 drawTrapBit(Ctx& ctx, i32 trapFlags) {
    // CONFLICT-5: loop until a set bit, capped at kChestTrapTries (port addition);
    // returns 0 on exhaustion. Bits 0..11 per CONFLICT-29 engine order.
    for (i32 tries = 0; tries < kChestTrapTries; ++tries) {
        i32 bit = static_cast<i32>(ctx.rng.below(12));
        if (((trapFlags >> bit) & 1) != 0) {
            return bit;
        }
    }
    return 0;
}

ChestTrap trapForBit(i32 bit) {
    // CONFLICT-29 / CONFLICT-34: engine index order (Fate 6, Fear 9, Withering 10).
    // Bits 5 and 11 have no name: UNKNOWN-2, mapped to NotTrapped.
    switch (bit) {
        case 0: return ChestTrap::PoisonGas;
        case 1: return ChestTrap::DiseaseSpores;
        case 2: return ChestTrap::Explosion;
        case 3: return ChestTrap::Fire;
        case 4: return ChestTrap::ItemDestruction;
        case 6: return ChestTrap::Fate;
        case 7: return ChestTrap::Trap7;
        case 8: return ChestTrap::Blackout;
        case 9: return ChestTrap::Trap9;    // Fear (CONFLICT-29)
        case 10: return ChestTrap::Trap10;  // Withering (CONFLICT-29)
        default: return ChestTrap::NotTrapped;
    }
}

// ---------------------------------------------------------------------------
// Chest contents
// ---------------------------------------------------------------------------

bool rollChestHasItems(Ctx& ctx, const ChestTuning& tuning, ChestLock lock, i32 dungeonLevel) {
    // UNKNOWN: item-spawn formula only partly given (archive F7 unverified).
    // CONFLICT-4: roll above kItemRollCapThreshold is capped unless ItemChance == 100.
    // CONFLICT-21: LN floor bonus applied when lock is CHARM_OPENED (lockState -1).
    // Order (cap then bonus) is an assumption.
    i32 r = static_cast<i32>(ctx.rng.below(100)) + 1;
    if (r > kItemRollCapThreshold && tuning.itemChance != 100) {
        r = kItemRollCapThreshold;
    }
    if (lock == ChestLock::CHARM_OPENED) {
        r += static_cast<i32>(vbInt(std::log(static_cast<double>(dungeonLevel + 10)) * 5.0));
    }
    return r <= tuning.itemChance;
}

void rollChestItems(Ctx& ctx, const GameData& data, i32 dungeonLevel, ChestClass cls, Vec<Item>& out) {
    // UNKNOWN-4: item band selection and per-class counts are not in the provided
    // SRD text. Placeholder: appends nothing. Fill when the band table is supplied.
    (void)ctx;
    (void)data;
    (void)dungeonLevel;
    (void)cls;
    (void)out;
}

Currency rollChestGold(Ctx& ctx, ChestClass cls, i32 dungeonLevel) {
    // UNKNOWN: base chest gold formula (Rules.ChestExp / gold band) not in the
    // provided SRD text (UNKNOWN-3). Placeholder returns 0.
    (void)ctx;
    (void)cls;
    (void)dungeonLevel;
    return 0;
}

Currency scaleChestGold(Currency base, ChestClass cls, bool magicLocked) {
    // [c5][c7] magic lock multiplies by kLockGoldNum/kLockGoldDen (3/2).
    // UNKNOWN-15 / CONFLICT-19: rounding placement; integer truncation used.
    // Class scale is archive-only and unverified (CONFLICT-27): not applied.
    (void)cls;
    if (magicLocked) {
        return base * kLockGoldNum / kLockGoldDen;
    }
    return base;
}

void createChest(Ctx& ctx, const ChestTuning& tuning, SquareState& sq, i32 dungeonLevel,
                 bool lair, bool squareAllowsMagic) {
    // Creation order: class, lock, item-spawn roll, gold.
    // UNKNOWN-25: trap draw ordering on creation is not given; no trap is set here.
    ChestClass cls = rollChestClass(ctx, tuning, lair);
    sq.chestClass = cls;
    sq.hasChest = (cls != ChestClass::NONE);
    sq.chestCount = kChestCountStart;     // [c65] CONFLICT-22
    sq.chestTeleporterTrap = false;
    sq.chestTrap = ChestTrap::NotTrapped;
    sq.chestGold = 0;
    sq.chestItems = Vec<Item>();

    if (!sq.hasChest) {
        sq.chestLock = ChestLock::UNLOCKED;
        return;
    }

    ChestLock lock = rollChestLock(ctx, tuning, cls, squareAllowsMagic);
    sq.chestLock = lock;
    if (rollChestHasItems(ctx, tuning, lock, dungeonLevel)) {
        rollChestItems(ctx, ctx.data, dungeonLevel, cls, sq.chestItems);
    }
    Currency base = rollChestGold(ctx, cls, dungeonLevel);
    sq.chestGold = scaleChestGold(base, cls, lock == ChestLock::MAGIC_LOCKED);
}

// ---------------------------------------------------------------------------
// Trap effects
// ---------------------------------------------------------------------------

bool chestPoisonTakes(Ctx& ctx, i32 poisonResistance) {
    // CONFLICT-6: port form as written: U*100 < resistance/2 (measured data matches "<").
    double u = ctx.rng.nextDouble();
    return u * 100.0 < static_cast<double>(poisonResistance) / 2.0;
}

bool chestDiseaseTakes(Ctx& ctx, i32 resistance) {
    // UNKNOWN-5: exact disease test not given separately; same form as poison assumed.
    double u = ctx.rng.nextDouble();
    return u * 100.0 < static_cast<double>(resistance) / 2.0;
}

i32 fireTrapRemainingHits(i32 hits, i32 damage) {
    // CONFLICT-7: rounding unresolved. Integer arithmetic (port truncation); clamped at 0 (UNKNOWN).
    i32 rem = hits - damage;
    return rem < 0 ? 0 : rem;
}

void applyBlackoutTrap(Ctx& ctx, Party& party, i32 openerIndex) {
    // CONFLICT-26: engine behaviour removes spell-granted effects. Manual detail unverified.
    (void)openerIndex;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[i];
        c.activeEffectIds = Vec<i32>();
        c.ailments.levitating = false;
        c.ailments.invisible = false;
        c.ailments.protectLevel = 0;
    }
    ctx.say("chest.trap.blackout", "The lights go out and spell effects fade.", {}, {});
}

void applyChestTrap(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest, ChestTrap trap, i32 d) {
    // One implementation of the trap effects exists: loot1a.cpp resolveChestTrap (reference ChestTraps.Spring: derived resistances,
    // damage from the dungeon level, item destruction, teleport). The earlier body here was a weaker copy (explosion did no damage).
    (void)chest;
    (void)d;
    if (trap == ChestTrap::NotTrapped) return;
    ctx.say("chest.trap", "The chest was trapped!", {static_cast<i64>(trap)}, {});
    ChestTrapResult r = resolveChestTrap(ctx, party, openerIndex, trap);
    (void)r;
}

// ---------------------------------------------------------------------------
// Trap identify and disarm
// ---------------------------------------------------------------------------

i32 trapIdentifyChance(i32 naturalWis, i32 gearWis, i32 monGuildLevel, i32 monDex,
                       i32 thiefGuildLevel, double disarmRating) {
    // CONFLICT-24: engine form (F24).
    // conf = naturalWis + gearWis/4.0 - ln(max(1,monGuildLvl))*ln(max(1,monDex))*2.5
    // clamp conf to [kTrapIdentifyMin, kTrapIdentifyMax] (UNKNOWN-27: clamp applies to conf).
    // result = floor(conf + disarm/1.2) + ln(max(1,thievGuildLvl))^2
    double conf = static_cast<double>(naturalWis) + static_cast<double>(gearWis) / 4.0
                - std::log(static_cast<double>(atLeastOne(monGuildLevel)))
                  * std::log(static_cast<double>(atLeastOne(monDex))) * 2.5;
    if (conf < static_cast<double>(kTrapIdentifyMin)) conf = static_cast<double>(kTrapIdentifyMin);
    if (conf > static_cast<double>(kTrapIdentifyMax)) conf = static_cast<double>(kTrapIdentifyMax);
    double lnThief = std::log(static_cast<double>(atLeastOne(thiefGuildLevel)));
    double total = std::floor(conf + disarmRating / 1.2) + lnThief * lnThief;
    return static_cast<i32>(vbInt(total));
}

bool trapDisarmRoll(Ctx& ctx, i32 thievingSkill) {
    // CONFLICT-20: engine form Rnd(100)+1 <= thieving. Proposed formula not used.
    i32 r = static_cast<i32>(ctx.rng.below(100)) + 1;
    return r <= thievingSkill;
}

DisarmResult attemptChestDisarm(Ctx& ctx, const Party& party, i32 thiefIndex, SquareState& chest,
                                i32 thievingSkill, i32 identifyChance) {
    DisarmResult res{};
    res.identifyChance = identifyChance;
    if (thiefIndex < 0 || thiefIndex >= party.size || !chest.hasChest) {
        res.outcome = DisarmOutcome::Failed;
        return res;
    }
    // UNKNOWN-24: disarm and identify attempt order not confirmed; disarm roll only here.
    if (trapDisarmRoll(ctx, thievingSkill)) {
        chest.chestTrap = ChestTrap::NotTrapped;
        chest.chestTeleporterTrap = false;
        chest.trapFlags = 0;
        res.outcome = DisarmOutcome::Disarmed;
        return res;
    }
    // UNKNOWN: failed disarm on a trapped chest reported as Triggered (caller applies trap).
    res.outcome = (chest.chestTrap != ChestTrap::NotTrapped) ? DisarmOutcome::Triggered
                                                             : DisarmOutcome::Failed;
    return res;
}

// ---------------------------------------------------------------------------
// Opening
// ---------------------------------------------------------------------------

bool tickChestCountdown(Ctx& ctx, SquareState& chest) {
    // CONFLICT-22: engine ChestTick. Count 2 -> 1 -> 0 -> -1; opens when it reaches -1.
    // UNKNOWN: the engine drops the count when there is no gold; not modelled.
    (void)ctx;
    if (!chest.hasChest) return false;
    chest.chestCount -= 1;
    return chest.chestCount < 0;
}

// Forward declaration of the file-local helper defined further below.
// FIXED: it was used in openChest before its definition.
static bool placeInFreePackSlot(Party& party, i32 member, const Item& it);

// FIXED: placeChestItem is not declared in any listed header. Declared static here and
// defined further below (interim implementation, see its comment).
static bool placeChestItem(Ctx& ctx, Party& party, const Item& it);

// FIXED (UNRESOLVED IN THIS FILE): the compiler reports "ambiguating new declaration"
// because loot2.h (not provided) already declares openChest with the same parameters
// but a different return type. A function cannot be overloaded on return type, and
// renaming it would break callers, so this file alone cannot compile. The return type
// here is a plain i32 code. loot2.h must be changed to declare:
//   i32 openChest(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest);
// The header's existing return type is not visible in the API index or the state digest,
// so it must be checked directly.
//
// Opens a chest (R-chest). Returns:
//   0 = still being opened (countdown)
//   1 = refused (no chest, or bad opener index)
//   2 = opened and paid out
//   3 = opened with no payout (opener killed, the trap killed the leader, the trap moved
//       the party, or nobody is left standing)
// Once the countdown is over the chest is removed. On return chestItems holds only the items that
// did not fit into a pack; those are lost.
//
// FIXED: combat2a.h declares resolveChestTrap(ctx, party, trap) as void, with no opener index
// and no outcome. The trap is sprung with that call, and the "trap killed the leader" test is
// taken from members[0] after the trap. The R33 "trap stops payout" outcome cannot be read
// from the API and is therefore not applied.
i32 openChest(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest) {
    if (!chest.hasChest || openerIndex < 0 || openerIndex >= party.size) {
        return 1;  // Refused
    }
    if (!tickChestCountdown(ctx, chest)) {
        return 0;  // Waiting
    }

    const i32 x0 = party.x;
    const i32 y0 = party.y;
    const i32 l0 = party.dungeonLevel;

    // Trap first (R14). UNKNOWN: no disarm roll is made; the disarm skill is not in the packet,
    // so no trap is disarmed.
    if (chest.chestTrap != ChestTrap::NotTrapped) {
        resolveChestTrap(ctx, party, openerIndex, chest.chestTrap);   // loot1a: the one implementation of the trap effects
    }
    chest.hasChest = false;

    // R24: a member whose hits fell below 1 is dead. Set the death state so isDead() agrees.
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[static_cast<size_t>(i)];
        if (c.vitals.curHits < 1 && c.deathState == DeathState::Alive) {
            c.deathState = DeathState::Dead;
        }
    }

    const bool moved = (party.x != x0 || party.y != y0 || party.dungeonLevel != l0);
    const bool openerKilled = party.members[static_cast<size_t>(openerIndex)].isDead();
    // FIXED: the leader (members[0]) stands for the trap-killed-the-leader outcome (R24).
    const bool partyKilled = party.members[0].isDead();
    i32 living = 0;
    for (i32 i = 0; i < party.size; ++i) {
        if (!party.members[static_cast<size_t>(i)].isDead()) ++living;
    }

    // A killed opener, a trap that kills the leader, a trap that moves the party,
    // or an empty party stops the payout.
    // UNKNOWN: SRD silent on the contents of a chest whose trap moves the party; treated as lost.
    if (openerKilled || partyKilled || moved || living == 0) {
        chest.chestGold = 0;
        chest.fightGold = 0;
        chest.chestItems = Vec<Item>();
        return 3;
    }

    // Gold (R16/R17): the chest gold and any fight gold are shared among the living members as a
    // whole-gold share (integer division), the same to each member. The rounding remainder stays unpaid.
    // CONFLICT: the reference PayGold shares to the dead too; the task specifies the living only,
    // so the living are paid here.
    const Currency pot = chest.chestGold + chest.fightGold;
    if (pot > 0) {
        // living > 0 here (returned above otherwise).
        const Currency share = pot / living;
        for (i32 i = 0; i < party.size; ++i) {
            const size_t m = static_cast<size_t>(i);
            if (!party.members[m].isDead()) {
                party.members[m].wallet.goldOnHand += share;
            }
        }
    }
    chest.chestGold = 0;
    chest.fightGold = 0;

    // Items (R20): placeChestItem tries the members' packs from a random start (one rng draw per
    // item). Items that do not fit are lost.
    // UNKNOWN: SRD silent on overflow; the packet says lost with a message.
    Vec<Item> items = chest.chestItems;
    chest.chestItems = Vec<Item>();
    Vec<Item> unplaced;
    for (const Item& it : items) {
        if (!placeChestItem(ctx, party, it)) {
            unplaced.push_back(it);
        }
    }
    chest.chestItems = unplaced;
    if (!unplaced.empty()) {
        ctx.say("chest.itemsLost", "Some chest items did not fit in any pack and are lost.",
                {static_cast<i64>(unplaced.size())}, {});
    }
    return 2;  // Opened and paid
}

// ---------------------------------------------------------------------------
// Gold distribution
// ---------------------------------------------------------------------------

i32 chestReceiverIndex(Ctx& ctx, i32 partySize) {
    // CONFLICT-9: engine CInt(Rnd*(PartySize-1)); VB CInt rounds half to even.
    double u = ctx.rng.nextDouble();
    if (partySize <= 1) return 0;
    i32 idx = static_cast<i32>(roundHalfEven(u * static_cast<double>(partySize - 1)));
    if (idx < 0) idx = 0;
    if (idx > partySize - 1) idx = partySize - 1;
    return idx;
}

void distributeChestGold(Ctx& ctx, Party& party, Currency gold, i32 openerIndex, bool openerKilled) {
    // CONFLICT-8: an opener killed by the trap receives nothing.
    // UNKNOWN-16: whether a killed non-leader opener stops payout is not confirmed.
    // Assumption: gold is split equally among living members; remainder to the drawn receiver.
    (void)openerIndex;
    if (openerKilled || gold <= 0 || party.size <= 0) return;

    i32 alive = 0;
    for (i32 i = 0; i < party.size; ++i) {
        if (!party.members[i].isDead()) ++alive;
    }
    if (alive <= 0) return;

    Currency share = splitChestGold(gold, alive);
    Currency paid = 0;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[i];
        if (c.isDead()) continue;
        c.wallet.goldOnHand += share;
        paid += share;
    }
    Currency remainder = gold - paid;
    if (remainder > 0) {
        i32 receiver = chestReceiverIndex(ctx, party.size);
        if (party.members[receiver].isDead()) {
            // UNKNOWN: remainder given to first living member if receiver is dead.
            for (i32 i = 0; i < party.size; ++i) {
                if (!party.members[i].isDead()) {
                    receiver = i;
                    break;
                }
            }
        }
        party.members[receiver].wallet.goldOnHand += remainder;
    }
    ctx.say("chest.gold", "The party shares the chest's gold.", {}, {});
}

Currency splitChestGold(Currency pot, i32 shareCount) {
    // CONFLICT-11: integer shares (formatNumber has no decimals).
    if (shareCount <= 0) return 0;
    return pot / shareCount;
}

// ---------------------------------------------------------------------------
// Items, thieves, purse and bank
// ---------------------------------------------------------------------------

bool slotCursedFromItem(const Item& item) {
    // CONFLICT-32: SlotCursed = (Items.Cursed != 0); kind 1 vs 2 is not kept.
    return item.cursed != CurseNone;
}

Currency rollThiefTake(Ctx& ctx, Currency purse) {
    // UNKNOWN-9 / CONFLICT-23: roll domain and boundary not in provided text.
    // Sampled uniformly in [0, purse); if roll >= purse the whole purse is taken.
    if (purse <= 0) return 0;
    Currency roll = static_cast<Currency>(vbInt(ctx.rng.nextDouble() * static_cast<double>(purse)));
    return roll < purse ? roll : purse;
}

bool canPayFromPurse(const Purse& wallet, Currency price) {
    // UNKNOWN-10: wording says "purse short"; check implemented is Gold + BankGold >= price.
    return wallet.goldOnHand + wallet.goldInBank >= price;
}

bool bankDepositGold(Purse& wallet, Currency amount) {
    // CONFLICT-10: original refuses deposit that is <= 0 or greater than carried gold.
    if (amount <= 0 || amount > wallet.goldOnHand) return false;
    wallet.goldOnHand -= amount;
    wallet.goldInBank += amount;
    return true;
}

bool bankWithdrawGold(Purse& wallet, Currency amount) {
    // CONFLICT-10 port rule: refuse only if amount exceeds bank balance; negative accepted.
    if (amount > wallet.goldInBank) return false;
    wallet.goldInBank -= amount;
    wallet.goldOnHand += amount;
    return true;
}

// ---------------------------------------------------------------------------
// Guild quest, monster drops
// ---------------------------------------------------------------------------

void assignGuildQuest(Ctx& ctx, const GameData& data, GuildMembership& membership, i32 guildLevel) {
    // UNKNOWN-19 / CONFLICT-33 / F37: quest kinds, targets and counts are not in the
    // provided SRD text. Quest left as kind None with the guild level recorded.
    (void)ctx;
    (void)data;
    membership.quest = GuildQuest{};
    membership.quest.level = guildLevel;
}

void rollMonsterDrops(Ctx& ctx, const GameData& data, i32 monsterIndex, bool laired, Vec<Item>& out) {
    // UNKNOWN: monster drop tables and laired bonus not in provided SRD text.
    // Community-only named laired drops (T14) not applied. Placeholder: no drops.
    (void)ctx;
    (void)data;
    (void)monsterIndex;
    (void)laired;
    (void)out;
}

// ---------------------------------------------------------------------------
// Item alignment, shelves, identify level
// ---------------------------------------------------------------------------

ItemAlign rollItemAlignment(Ctx& ctx) {
    // UNKNOWN-14: four draw values 0..3. 0 = Unaligned assumed; 1..3 = Good/Neutral/Evil.
    return static_cast<ItemAlign>(static_cast<i32>(ctx.rng.below(4)));
}

bool shelvedBySelling(i32 floor, ItemAlign align) {
    // CONFLICT-14: engine test is not (Floor == 0 and align == Unaligned).
    return !(floor == 0 && align == Unaligned);
}

IdLevel knownToIdLevel(i32 known) {
    // CONFLICT-15: engine form id = kKnownScaleMax - Known; id <= 0 is fully identified.
    i32 k = known;
    if (k < 0) k = 0;
    if (k > kKnownScaleMax) k = kKnownScaleMax;
    i32 id = kKnownScaleMax - k;
    if (id <= 0) return All;
    return static_cast<IdLevel>(id);
}

bool buyOutRemoveOne(Ctx& ctx, Vec<Item>& stock, i32 maxDraws) {
    // CONFLICT-13: kBuyOutDrawCap (100) quirk; maxDraws < 0 means unbounded.
    // UNKNOWN: acceptance test for a drawn item not given; first draw accepted.
    i32 n = static_cast<i32>(stock.size());
    if (n <= 0) return false;
    i32 draws = 0;
    for (;;) {
        if (maxDraws >= 0 && draws >= maxDraws) return false;
        ++draws;
        i32 idx = static_cast<i32>(ctx.rng.below(n));
        bool accept = true;  // UNKNOWN: acceptance test not in provided text
        if (accept) {
            stock.erase(stock.begin() + idx);
            return true;
        }
    }
}

void replenishShelf(Ctx& ctx, const GameData& data, Vec<Item>& shelf) {
    // UNKNOWN-8: Replenish mechanics beyond the call signature are not in provided text.
    // Placeholder: no change.
    (void)ctx;
    (void)data;
    (void)shelf;
}

// Interim slot search in one member's pack.
static bool placeInFreePackSlot(Party& party, i32 member, const Item& it) {
    auto& pack = party.inventories[static_cast<size_t>(member)].pack;
    for (size_t s = 0; s < static_cast<size_t>(kPackSlots); ++s) {
        if (pack[s].id <= 0) {
            pack[s] = it;
            pack[s].equipped = false;
            return true;
        }
    }
    return false;
}

// NEED: placeChestItem (loot1a.h). Interim implementation until its signature is available:
// tries the members' packs from a random start (one rng draw per item), as described in
// openChest. Reports no message on a full party.
static bool placeChestItem(Ctx& ctx, Party& party, const Item& it) {
    if (party.size <= 0) return false;
    const i32 start = static_cast<i32>(ctx.rng.below(party.size));
    for (i32 k = 0; k < party.size; ++k) {
        const i32 m = (start + k) % party.size;
        if (placeInFreePackSlot(party, m, it)) return true;
    }
    return false;
}

}  // namespace mordor
