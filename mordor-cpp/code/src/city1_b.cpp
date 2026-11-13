// city1_b.cpp: pens, vault, town charges and shortfall cover, store row and mod hooks, arrival deposit.
// Assumed MonsterDef field names (game_data.h not provided; verify against the generated header):
//   att, def, hits, stats (Arr<i32,6>), specialAttackFlags (u32), spellFlags (u32),
//   resistances (Arr<i32,12>, indexed directly), levelFound (i32), chance (i32),
//   alignmentValue (Align-compatible i32 0..2).
// FIXED: the compiler reported no member named `align` or `alignment` on MonsterDef. The field is
// `alignmentValue`. The compiler also reported no member `resist` on `resistances`, which is
// itself the Arr<i32,12>, so it is indexed directly. Verify these names against game_data.h;
// they are not confirmed.
// UNKNOWN: flagged where the SRD does not give enough detail.

#include "mordor/city1.h"
#include "mordor/monsters1a.h"

#include <cmath>

namespace mordor {

// R21 Take is only called by the buy handler when the pen count is above 0 (FRMCITYC.BAS:140-142).
bool penTakeAllowed(const PenBook& pens, i32 ref, i32 align) {
    if (ref < 0 || align < 0 || align >= kPenAlignCount) {
        return false;
    }
    if (ref >= static_cast<i32>(pens.byRef.size())) {
        return false;
    }
    return pens.byRef[static_cast<u32>(ref)][static_cast<u32>(align)] > 0;
}

// R22 Pen buy price, engine formula.
Currency penBuyPrice(Currency baseValue, i32 pennedCount) {
    // CONFLICT: R22 says Log(0) gives price 0 for an empty pen (the original error trap), while the
    // quirk note says the port uses Max(1, ...). Following the explicit formula and the stated
    // "buying from an empty pen gives price 0" result: an empty pen gives 0.
    if (pennedCount <= 0) {
        return 0;
    }
    Currency value = maxv(baseValue, static_cast<Currency>(10));  // value = max(BaseValue, 10)
    i32 stock = pennedCount - 1;                                  // stock = penned - 1
    if (stock > 9) {                                              // stock = min(stock, 9)
        stock = 9;
    }
    double denom = std::log(static_cast<double>(stock) + 1.0) + 1.15;
    double price = (1.5 * static_cast<double>(value)) / denom;
    if (!std::isfinite(price)) {
        return 0;
    }
    return roundHalfEven(price);  // Long, round half to even
}

// R23 Pen buy price, Set 2 (port-pinned factors 1.5 and 1.15, no ln term).
Currency penBuyPriceSet2(Currency baseValue, i32 pennedCount) {
    if (pennedCount <= 0) {
        return 0;
    }
    return roundHalfEven((1.5 * static_cast<double>(baseValue)) / 1.15);
}

// R24 Pen sell price.
Currency penSellPrice(Currency baseValue, i32 pennedCount, bool anyAlignmentPenned, i32 idLevel) {
    i32 stock = pennedCount;
    if (stock == 0 && anyAlignmentPenned) {
        stock = 1;
    }
    if (stock > 9) {
        stock = 9;
    }
    double price = ((0.9 * static_cast<double>(baseValue)) / (static_cast<double>(stock) + 1.15)) /
                   static_cast<double>(idLevel + 1);
    i64 r = roundHalfEven(price);  // Long
    return r < 0 ? 0 : r;          // floor 0
}

// R25 Pen identify price. Applies only when idLevel > 0 (step 3 of R20).
Currency penIdentifyPrice(Currency baseValue, i32 pennedCount, i32 idLevel) {
    // UNKNOWN: idLevel <= 0 is not an identify case (R20 step 3); returning 0 avoids divide by zero.
    if (idLevel <= 0) {
        return 0;
    }
    i32 stock = pennedCount;
    if (stock > 9) {
        stock = 9;
    }
    // As written: the "+ 1" is applied after the division by idLevel (R25 note).
    double v = ((0.13 * static_cast<double>(baseValue)) /
                ((static_cast<double>(stock) / 2.0 + 1.0) * 0.9)) /
                   static_cast<double>(idLevel) +
               1.0;
    i64 r = roundHalfEven(v);
    return r < 0 ? 0 : r;  // floor 0
}

// R27 Starting companion stock (OpenDejenol Confinement.Fresh, MAINMOD.BAS:522-559): ten creatures, two of each,
// at the creature's own alignment. The picks are named by monster kind: the first table index of kinds 0, 7, 8 and 14
// and the index after each, and kinds 1 and 9 alone. A kind the table lacks gives -1, and -1 + 1 picks index 0, as in the reference.
void seedStartingPens(Ctx& ctx, PenBook& pens) {
    const Vec<ClassRange> first = buildClassRanges(ctx);
    const auto start = [&first](i32 kind) { return kind < static_cast<i32>(first.size()) ? first[static_cast<u32>(kind)].first : -1; };
    const i32 picked[10] = {start(0), start(0) + 1, start(7), start(7) + 1, start(8), start(8) + 1,
                            start(14), start(14) + 1, start(1), start(9)};
    const auto& monsters = ctx.data.monsters;
    const i32 n = static_cast<i32>(monsters.size());
    for (i32 i = 0; i < n; ++i) {
        bool hit = false;
        for (i32 k = 0; k < 10; ++k) hit = hit || picked[k] == i;
        if (!hit) continue;
        const i32 align = static_cast<i32>(monsters[static_cast<u32>(i)].alignmentValue);
        if (align < 0 || align >= kPenAlignCount) continue;
        while (static_cast<i32>(pens.byRef.size()) <= i) pens.byRef.push_back(PenCounts{});
        pens.byRef[static_cast<u32>(i)][static_cast<u32>(align)] = 2;   // R27: pen(i, AlignmentOf(monster i)) = 2 (set, not add)
    }
}

// R28 Creature restock on rest.
void restockPens(Ctx& ctx, PenBook& pens, i64 daysAged, i32 deepestReached) {
    const auto& monsters = ctx.data.monsters;
    i32 n = static_cast<i32>(monsters.size());
    if (n <= 0) {
        return;
    }

    // chance = clamp(days, 1, 50)
    i64 chance = daysAged;
    if (chance < kRestockChanceMin) {
        chance = kRestockChanceMin;
    }
    if (chance > kRestockChanceMax) {
        chance = kRestockChanceMax;
    }

    // UNKNOWN: deepest < 1 is not covered by the SRD; clamp to the 1..14 range.
    i32 deepest = deepestReached;
    if (deepest < 1) {
        deepest = 1;
    }
    if (deepest > kTownDeepestFloor) {
        deepest = kTownDeepestFloor;
    }

    // l0288: the one pen index both paths share, 0 (Good) at the start of each call (Confinement.cs:409-447).
    // It is not reset per pass.
    i32 l0288 = 0;

    i64 passes = 0;
    // while rnd*100 <= chance and passes < 500 (left to right, rnd drawn first)
    while (ctx.rng.nextDouble() * 100.0 <= static_cast<double>(chance) && passes < kReplenishPassCap) {
        ++passes;
        bool adding = ctx.rng.nextDouble() * 100.0 < static_cast<double>(kAddProbabilityPct);

        // roll = rnd(100)+1; floor = #f in 1..deepest with roll < 100 - (f/(deepest+1))*100
        i32 roll = static_cast<i32>(ctx.rng.below(100)) + 1;
        i32 floorNo = 0;
        for (i32 f = 1; f <= deepest; ++f) {
            double threshold = 100.0 - (static_cast<double>(f) / static_cast<double>(deepest + 1)) * 100.0;
            if (static_cast<double>(roll) < threshold) {
                ++floorNo;
            }
        }
        if (floorNo < 1) {
            floorNo = 1;
        }
        if (floorNo > deepest) {
            floorNo = deepest;
        }

        // retry up to 100 tries: draw kind (RoundedIndex), draw creature with LevelFound <= floor, LevelFound 0 excluded
        // UNKNOWN: "weighted by Chance" is implemented as acceptance with probability chance/100.
        i32 pick = -1;
        for (i32 t = 0; t < 100 && pick < 0; ++t) {
            i32 kind = roundedIndex(ctx, n);
            if (kind < 0 || kind >= n) {
                continue;
            }
            const auto& cand = monsters[static_cast<u32>(kind)];
            if (cand.levelFound <= 0 || cand.levelFound > floorNo) {
                continue;
            }
            i32 weight = cand.chance;
            if (weight < 0) {
                weight = 0;
            }
            if (weight > 100) {
                weight = 100;
            }
            if (static_cast<i32>(ctx.rng.below(100)) < weight) {
                pick = kind;
            }
        }
        if (pick < 0) {
            continue;
        }

        const auto& chosen = monsters[static_cast<u32>(pick)];
        i32 align = static_cast<i32>(chosen.alignmentValue);  // FIXED: was .alignment
        if (align < 0 || align >= kPenAlignCount) {
            continue;
        }
        if (adding) {
            // PenOne (Confinement.cs:452-460): the cap is tested on pen l0288, but the increment
            // lands on the creature's own alignment. A failed test still ends the pass.
            const u32 r = static_cast<u32>(pick);
            while (pens.byRef.size() <= r) {
                PenCounts zero{};
                pens.byRef.push_back(zero);
            }
            if (pens.byRef[r][static_cast<u32>(l0288)] >= kPenShelfCap) {
                continue;
            }
            pens.byRef[r][static_cast<u32>(align)] += 1;
        } else {
            // UNKNOWN: the reference ReleaseOne(chosen, rng, ref l0288) body is not in the packet, so
            // a release does not move l0288 here. The shared index stays at 0.
            releaseOne(pens, pick, align);                 // ReleaseOne
        }
    }
}

// R29 Base worth of a creature. Units: gold in double, returned as Currency (1/10000 gold).
Currency creatureBaseWorth(const MonsterDef& def, i32 guildLevel) {
    double v = 500.0 * static_cast<double>(def.att) + 500.0 * static_cast<double>(def.def) +
               250.0 * static_cast<double>(def.hits);

    i32 statSum = def.stats[0] + def.stats[5];  // Stats[0] + Stats[5] (R22 quirk note on Stats(0))
    if (statSum < 1) {
        statSum = 1;                            // max(1, ...)
    }
    v = v * std::log(static_cast<double>(statSum));
    v = static_cast<double>(roundHalfEven(v * 10000.0)) / 10000.0;  // Currency rounding, 4 dp

    v = v * std::log(static_cast<double>(guildLevel) + 1.0);

    // Special attack bits: each set bit adds rate*GuildLevel + 1 (the +1 is a flat 1 gold).
    // UNKNOWN: rate units are taken to be the same units as v (gold).
    for (const BitRate& r : kSpecialAttackRates) {
        if (((def.specialAttackFlags >> r.bit) & 1u) != 0u) {
            v += static_cast<double>(r.rate) * static_cast<double>(guildLevel) + 1.0;
        }
    }

    // Spell bits, only if SpellFlags > 0.
    if (def.spellFlags > 0u) {
        for (const BitRate& r : kSpellRates) {
            if (((def.spellFlags >> r.bit) & 1u) != 0u) {
                v += static_cast<double>(r.rate) * static_cast<double>(guildLevel) + 1.0;
            }
        }
    }

    // Resistances 0..10: only positive values count (quirk note).
    for (i32 r = 0; r < kResistKinds; ++r) {
        i32 value = def.resistances[static_cast<u32>(r)];  // FIXED: was def.resistances.resist, which is the Arr itself
        if (value > 0) {
            v += 500.0 * static_cast<double>(value);
        }
    }

    if (def.hits == 1) {
        v = 1.0;
    }

    return roundHalfEven(v * 10000.0);
}

// R30 Item alignment scheme (1-based: Good 1, Neutral 2, Evil 3) to pen index (0-based).
i32 penIndexOfItemAlign(ItemAlign align) {
    return static_cast<i32>(align) - 1;
}

// R31 Deposit into the bank vault.
VaultDeposit depositToVault(Ctx& ctx, Inventory& inv, i32 packSlot) {
    (void)ctx;
    // Slot 0 and slots 40 and above are refused.
    if (packSlot <= 0 || packSlot >= kPackSlots) {
        return VaultDeposit::Refused;
    }
    Item& src = inv.pack[static_cast<u32>(packSlot)];
    // Equipped items are refused.
    if (src.equipped) {
        return VaultDeposit::Refused;
    }

    // hole = first vault index with ItemId <= 0
    i32 hole = -1;
    i32 used = 0;
    for (i32 i = 0; i < kBankSlots; ++i) {
        if (inv.bank[static_cast<u32>(i)].id <= 0) {
            if (hole < 0) {
                hole = i;
            }
        } else {
            ++used;
        }
    }
    // A full vault (40 slots in use, no hole) is refused.
    if (hole < 0 && used >= kBankSlots) {
        return VaultDeposit::Refused;
    }
    // An empty pack slot succeeds without change.
    if (src.id <= 0) {
        return VaultDeposit::EmptySlot;
    }
    if (hole < 0) {
        return VaultDeposit::Refused;  // unreachable: a non-full vault always has a hole
    }
    inv.bank[static_cast<u32>(hole)] = src;
    src = Item{};
    return VaultDeposit::Stored;
}

// R32 Withdraw selected vault lines into the first free pack slots.
i32 withdrawFromVault(Ctx& ctx, Inventory& inv, Party& party, const Vec<i32>& bankSlots) {
    (void)ctx;
    i32 moved = 0;
    for (u32 k = 0; k < static_cast<u32>(bankSlots.size()); ++k) {
        i32 b = bankSlots[k];
        if (b < 0 || b >= kBankSlots) {
            continue;
        }
        Item& src = inv.bank[static_cast<u32>(b)];
        if (src.id <= 0) {
            continue;
        }
        // UNKNOWN: pack slot 0 is refused for deposit, so the free-slot search starts at 1.
        i32 freeSlot = -1;
        for (i32 p = 1; p < kPackSlots; ++p) {
            if (inv.pack[static_cast<u32>(p)].id <= 0) {
                freeSlot = p;
                break;
            }
        }
        if (freeSlot < 0) {
            break;  // R32.2: a full pack stops a multiple withdrawal; the rest stays in the vault
        }
        // UNKNOWN: "the item is re-identified" has no defined effect in the SRD; left unchanged.
        inv.pack[static_cast<u32>(freeSlot)] = src;
        src = Item{};
        // Buffers naming that slot are let go.
        // UNKNOWN: ItemOrder.slot is taken to be the pack slot named by a buffer.
        for (u32 m = 0; m < static_cast<u32>(kPartyMaxSize); ++m) {
            if (party.itemOrders[m].slot == freeSlot) {
                party.itemOrders[m] = ItemOrder{};
            }
        }
        ++moved;
    }
    return moved;
}

// R33 Free vault slots.
i32 freeVaultSlots(const Inventory& inv) {
    i32 used = 0;
    for (i32 i = 0; i < kBankSlots; ++i) {
        if (inv.bank[static_cast<u32>(i)].id > 0) {
            ++used;
        }
    }
    return kBankSlots - used;
}

// R34 Town charge and purse check.
ChargeResult townCharge(Ctx& ctx, Party& party, i32 payerIndex, Currency cost, const TownModAnswers& mods) {
    ChargeResult res;
    if (payerIndex < 0 || payerIndex >= party.size) {
        return res;
    }
    Currency due = cost < 0 ? 0 : cost;
    Character& payer = party.members[static_cast<u32>(payerIndex)];

    // R34.2 and R34.3: short purse; the CoverShortfall hook may cover it first (R35).
    if (due > payer.purse()) {
        ShortfallCover cover = coverShortfall(ctx, party, payerIndex, due, mods);
        if (!cover.covered) {
            ctx.say("city.notEnoughGold", "Not enough gold!", {}, {});
            return res;
        }
    }

    // R34.1: gold on hand first, then the bank.
    Currency fromHand = payer.wallet.goldOnHand < due ? payer.wallet.goldOnHand : due;
    if (fromHand < 0) {
        fromHand = 0;
    }
    Currency fromBank = due - fromHand;
    payer.wallet.goldOnHand -= fromHand;
    payer.wallet.goldInBank -= fromBank;
    res.paid = true;
    res.fromHand = fromHand;
    res.fromBank = fromBank;
    return res;
}

// R35 Shortfall cover: plans the gifts and, if complete, moves them and reports one line per giver.
ShortfallCover coverShortfall(Ctx& ctx, Party& party, i32 payerIndex, Currency cost, const TownModAnswers& mods) {
    ShortfallCover plan = wouldCoverShortfall(party, payerIndex, cost, mods);
    if (!plan.covered) {
        return plan;  // nothing moves; the original refusal follows
    }
    Character& payer = party.members[static_cast<u32>(payerIndex)];
    for (u32 k = 0; k < static_cast<u32>(plan.gifts.size()); ++k) {
        const ShortfallGift& g = plan.gifts[k];
        Character& giver = party.members[static_cast<u32>(g.giver)];
        giver.wallet.goldInBank -= g.amount;
        payer.wallet.goldOnHand += g.amount;
        // R35.6: one Messages line per giver.
        ctx.say("city.coverGift", "A party member covered part of the shortfall.",
                {g.amount}, {giver.name, payer.name});
    }
    return plan;
}

// R36 WouldCoverShortfall: plans only, moves nothing.
ShortfallCover wouldCoverShortfall(const Party& party, i32 payerIndex, Currency cost, const TownModAnswers& mods) {
    ShortfallCover out;
    if (payerIndex < 0 || payerIndex >= party.size) {
        return out;
    }
    const Character& payer = party.members[static_cast<u32>(payerIndex)];
    Currency need = cost - payer.purse();
    if (need <= 0) {
        // R35.1: the test applies only when cost > purse; otherwise nothing is needed.
        out.covered = true;
        return out;
    }
    // R35.1 conditions: mod hooks live, party size >= 2, payer in the city.
    // "in the city" is the payer's dungeon level being 0 (Party.cs PlanShortfall: payer.DungeonLevel != 0 -> null).
    if (mods.liveMods == 0) {
        return out;
    }
    if (party.size < 2) {
        return out;
    }
    if (payer.location.dungeonLevel != 0) {
        return out;
    }

    // Givers in the named order or in party order (R35.2). Per-giver amount already planned.
    Arr<Currency, kPartyMaxSize> taken{};
    // Distinct() (Party.cs PlanShortfall): each giver is asked once, so a repeat in a named order is skipped.
    Arr<bool, kPartyMaxSize> seen{};
    bool named = !mods.coverOrder.empty();
    i32 steps = named ? static_cast<i32>(mods.coverOrder.size()) : party.size;
    for (i32 s = 0; s < steps && need > 0; ++s) {
        i32 g = named ? mods.coverOrder[static_cast<u32>(s)] : s;
        if (g == payerIndex || g < 0 || g >= party.size) {
            continue;  // payer and non-members are skipped
        }
        const u32 gu = static_cast<u32>(g);
        if (seen[gu]) {
            continue;  // repeated name: already considered
        }
        seen[gu] = true;
        const Character& giver = party.members[gu];
        if (giver.location.dungeonLevel != 0 || giver.isDead()) {
            continue;  // not in the city, or dead
        }
        // R35.3: part = min(max(0, bank), need), minus what this plan already took from this giver.
        Currency avail = giver.wallet.goldInBank - taken[gu];
        if (avail < 0) {
            avail = 0;
        }
        Currency part = avail < need ? avail : need;
        if (part > 0) {
            ShortfallGift gift;
            gift.giver = g;
            gift.amount = part;
            out.gifts.push_back(gift);
            taken[gu] += part;
            need -= part;
        }
    }

    // R35.4: if the full shortfall cannot be covered, nothing moves.
    if (need > 0) {
        out.gifts = Vec<ShortfallGift>{};
        out.covered = false;
        return out;
    }
    out.covered = true;
    return out;
}

// CityAltP: ALT-p in town with a party of two or more pools to the character in command.
AltPoolOutcome chooseAltPPool(const Party& party, bool inTown, const TownModAnswers& mods) {
    // UNKNOWN: the SRD says the dungeon never asks; no pooling there is assumed (Refused).
    if (!inTown) {
        return AltPoolOutcome::Refused;
    }
    // A party of one is refused first and is not asked.
    if (party.size < kAltPoolMinParty) {
        return AltPoolOutcome::Refused;
    }
    // Default PoolOnHand; a mod may choose PoolAndDeposit.
    if (mods.altPoolAnswered && mods.altPoolDeposit) {
        return AltPoolOutcome::PoolAndDeposit;
    }
    return AltPoolOutcome::PoolOnHand;
}

// R37.2 and R38: a row is shown unless a StoreRowVisible mod answered false for it.
bool storeRowVisible(const TownModAnswers& mods, i32 row) {
    if (mods.liveMods == 0) {
        return true;
    }
    for (u32 i = 0; i < static_cast<u32>(mods.hiddenRows.size()); ++i) {
        if (mods.hiddenRows[i] == row) {
            return false;
        }
    }
    return true;
}

// R39: the row order answer must have the same count and the same multiset of row values.
bool storeRowOrderAccepted(const Vec<i32>& defaultRows, const Vec<i32>& modRows) {
    if (defaultRows.size() != modRows.size()) {
        return false;
    }
    for (u32 i = 0; i < static_cast<u32>(defaultRows.size()); ++i) {
        i32 v = defaultRows[i];
        u32 countDefault = 0;
        u32 countMod = 0;
        for (u32 j = 0; j < static_cast<u32>(defaultRows.size()); ++j) {
            if (defaultRows[j] == v) {
                ++countDefault;
            }
        }
        for (u32 j = 0; j < static_cast<u32>(modRows.size()); ++j) {
            if (modRows[j] == v) {
                ++countMod;
            }
        }
        if (countDefault != countMod) {
            return false;
        }
    }
    return true;
}

// R40: refused unless some mod answered true (a throwing mod is counted as false).
bool storeRowChangeAllowed(const TownModAnswers& mods) {
    return mods.rowChangeAllowed;
}

// R41: the TownIncomeBank answer, if it names a party member; otherwise the seller's hand.
i32 townIncomePayee(const Party& party, i32 sellerIndex, const TownModAnswers& mods) {
    // R41: a non-member answer counts as null, so the payment goes to the seller's hand.
    // UNKNOWN: the seller's hand is returned as the seller's member index.
    if (mods.incomeAnswered) {
        i32 who = mods.incomePayee;
        if (who >= 0 && who < party.size) {
            return who;
        }
    }
    return sellerIndex;
}

// R42: kept = max(1, first non-null mod answer), else 20.
i32 messageWindowLines(const TownModAnswers& mods) {
    if (mods.messagesKept <= 0) {
        return kDefaultMessageLines;
    }
    return mods.messagesKept < 1 ? 1 : mods.messagesKept;
}

// R43: default refused.
bool storeAlignChoiceUnlocked(const TownModAnswers& mods) {
    return mods.alignChoiceUnlocked;
}

// R39 and R44: a mismatch disables that mod's row order for the session.
void disableRowOrderForSession(TownModAnswers& mods) {
    mods.rowOrderMatches = false;
    mods.rowOrderDisabled = true;
}

// R47: arrival deposit and pool (mod hook).
ArrivalDepositResult arrivalDepositAndPool(Ctx& ctx, Party& party, const TownModAnswers& mods) {
    (void)ctx;
    ArrivalDepositResult res;  // deposited 0, wealthiestCheckDue true (R47 step 5 runs either way)
    // R47 step 1: no live mods means nothing moves.
    if (mods.liveMods == 0) {
        return res;
    }
    if (!mods.arrival.answered) {
        return res;
    }
    if (mods.arrival.action == ArrivalAction::PartyDeposit) {
        res.deposited = partyDepositOnHand(party);
    } else if (mods.arrival.action == ArrivalAction::PartyPool) {
        res.deposited = partyPoolOnHandToBank(party, mods.arrival.poolTo);
    }
    // R47 step 4: a line is requested only when deposited > 0 and the message is not blank.
    if (res.deposited > 0 && mods.arrival.hasMessage) {
        bool blank = true;
        const Str& msg = mods.arrival.message;
        for (u32 i = 0; i < static_cast<u32>(msg.size()); ++i) {
            char c = msg[i];
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                blank = false;
                break;
            }
        }
        if (!blank) {
            res.messageDue = true;
            res.line = msg;
        }
    }
    return res;
}

// R47 PartyDeposit: each member's gold on hand goes into their own account.
Currency partyDepositOnHand(Party& party) {
    Currency total = 0;
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        Currency amt = m.wallet.goldOnHand;
        if (amt <= 0) {
            continue;
        }
        m.wallet.goldOnHand = 0;
        m.wallet.goldInBank += amt;
        total += amt;
    }
    return total;
}

// R47 PartyPool: all members' gold on hand goes into the named member's account.
// All or nothing on overflow of the target account.
Currency partyPoolOnHandToBank(Party& party, i32 toMember) {
    if (toMember < 0 || toMember >= party.size) {
        return 0;
    }
    constexpr Currency kBankCeiling = 9223372036854775807LL;
    Currency sum = 0;
    for (i32 i = 0; i < party.size; ++i) {
        Currency amt = party.members[static_cast<u32>(i)].wallet.goldOnHand;
        if (amt > 0) {
            sum += amt;
        }
    }
    Character& target = party.members[static_cast<u32>(toMember)];
    if (sum > kBankCeiling - target.wallet.goldInBank) {
        return 0;  // overflow: nothing moves
    }
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        if (m.wallet.goldOnHand > 0) {
            m.wallet.goldOnHand = 0;
        }
    }
    target.wallet.goldInBank += sum;
    return sum;
}

}  // namespace mordor
