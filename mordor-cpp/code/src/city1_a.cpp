// city1_a.cpp: first half of the City module (SRD c16p1).
// Implements: arrival and city rest (R1 to R4), crest recharge (R5), town switch (R6),
// starting and replenished store stock (R7 to R10), store buy and selection (R11 to R14),
// store sale and shelving (R15 to R17), shelf caps (R18), and the pen helpers used by
// the confinement buy and sell.
#include "mordor/city1.h"
#include "mordor/attributes.h"

namespace mordor {

// ---------------------------------------------------------------------------
// R7 Starting store stock (CreateInitial).
// UNKNOWN: ItemDef field names (id, floor, price, cursed, alignmentFlags, type, rarity) are
// not given in the foundation; assumed to match the data file names used here.
// UNKNOWN: store representation is one Item entry per stocked unit (count copies).
// ---------------------------------------------------------------------------
void createInitialStore(Ctx& ctx, Vec<Item>& store) {
    store.clear();
    const i32 n = static_cast<i32>(ctx.data.items.size());
    for (i32 i = 0; i < n; ++i) {
        const ItemDef& def = ctx.data.items[static_cast<u32>(i)];
        const i32 id = def.id;

        // R7 artifact branch: artifacts 356..361 with fixed alignment and count.
        i32 artIdx = -1;
        for (i32 k = 0; k < 6; ++k) {
            if (kStartingArtifacts[k].itemId == id) {
                artIdx = k;
            }
        }
        if (artIdx >= 0) {
            const StartingArtifact& art = kStartingArtifacts[artIdx];
            Item unit{};
            unit.id = id;
            unit.alignment = art.alignment;
            unit.aligned = art.alignment != ItemAlign::Unaligned;
            for (i32 c = 0; c < art.count; ++c) {
                store.push_back(unit);
            }
            continue;
        }

        // R7 skip: Floor > 1, Price > 200000, Price == 0, or Cursed.
        if (def.floor > 1 || def.price > 200000 || def.price == 0 || def.cursed) {
            continue;
        }

        Item unit{};
        unit.id = id;
        if (def.alignmentFlags > 1) {
            // R7: AlignmentFlags > 1 is a numeric test (quirk kept). One each of G, N, E.
            const ItemAlign aligns[3] = {ItemAlign::Good, ItemAlign::Neutral, ItemAlign::Evil};
            for (i32 a = 0; a < 3; ++a) {
                unit.alignment = aligns[a];
                unit.aligned = true;
                store.push_back(unit);
            }
        } else {
            // R7: Unaligned = (Floor == 0) ? 99 : 5.
            unit.alignment = ItemAlign::Unaligned;
            unit.aligned = false;
            const i32 count = (def.floor == 0) ? 99 : 5;
            for (i32 c = 0; c < count; ++c) {
                store.push_back(unit);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// R8 Store replenish (Replenish, sub071B).
// UNKNOWN: days / 100 * 100 taken as the mathematical value (= days). Integer division
// would give 0 below 100 days (SRD note).
// UNKNOWN: sell-in and buy-out details beyond "sell-in loops until drawn; buy-out tries at
// most 100 draws" are not in this SRD part. The sell-in loop has no cap (as stated).
// UNKNOWN: the alignment of a sell-in item is not stated; left Unaligned.
// ---------------------------------------------------------------------------
void replenishStore(Ctx& ctx, Vec<Item>& store, i64 daysAged, i32 deepestReached) {
    i32 deepest = deepestReached;
    if (deepest < 1) {
        deepest = 1;
    }
    if (deepest > kTownDeepestFloor) {
        deepest = kTownDeepestFloor;
    }

    i64 chance = daysAged;
    if (chance < kRestockChanceMin) {
        chance = kRestockChanceMin;
    }
    if (chance > kRestockChanceMax) {
        chance = kRestockChanceMax;
    }

    while (true) {
        if (ctx.rng.nextDouble() * 100.0 > static_cast<double>(chance)) {
            break;
        }
        const bool adding = ctx.rng.nextDouble() * 100.0 < static_cast<double>(kAddProbabilityPct);
        const i32 roll = static_cast<i32>(vbInt(ctx.rng.nextDouble() * 100.0)) + 1;

        i32 pickFloor = 0;
        for (i32 f = 1; f <= deepest; ++f) {
            const double threshold =
                100.0 - (static_cast<double>(f) / static_cast<double>(deepest + 1)) * 100.0;
            if (static_cast<double>(roll) < threshold) {
                pickFloor = f;
            } else {
                break;
            }
        }
        if (pickFloor < 1) {
            pickFloor = 1;
        }
        if (pickFloor > deepest) {
            pickFloor = deepest;
        }

        if (adding) {
            // Reference StoreStock.Replenish / AddOne: draw until an uncursed floor>0 item; Unaligned unless its alignment flags
            // exceed 1, then a random allowed alignment (bits 0-3); one copy is shelved unless that alignment already holds 9.
            for (i32 guard = 0; guard < 100000; ++guard) {
                const i32 idx = drawItemIndex(ctx, pickFloor);
                if (idx <= 0) continue;
                const ItemDef& d = ctx.data.items[static_cast<u32>(idx)];
                if (d.cursed != 0 || d.floor <= 0) continue;
                ItemAlign al = Unaligned;
                if (d.alignmentFlags > 1) {
                    if ((d.alignmentFlags & 0xF) == 0) break;      // the original would spin here; nothing is shelved
                    do {
                        al = static_cast<ItemAlign>(static_cast<i32>(vbInt(ctx.rng.nextDouble() * 4.0)));
                    } while ((d.alignmentFlags & (1 << static_cast<i32>(al))) == 0);
                }
                i32 same = 0;
                for (const Item& o : store) {
                    if (o.id == d.id && o.alignment == al) ++same;
                }
                if (same < 9) {                                       // ShelfCap
                    Item unit{};
                    unit.id = d.id;                                   // the item's id, not its table index
                    unit.alignment = al;
                    unit.level = pickFloor;
                    store.push_back(unit);
                }
                break;
            }
        } else {
            // Reference RemoveOne: up to 100 tries for an uncursed floor>0 item with Unaligned stock (the original only looks at the
            // Unaligned count), then one copy leaves from a random alignment that has stock.
            for (i32 t = 0; t < 100; ++t) {
                const i32 idx = drawItemIndex(ctx, pickFloor);
                if (idx <= 0) continue;
                const ItemDef& d = ctx.data.items[static_cast<u32>(idx)];
                if (d.cursed != 0 || d.floor <= 0) continue;
                auto countOf = [&](ItemAlign a2) { i32 n = 0; for (const Item& o : store) if (o.id == d.id && o.alignment == a2) ++n; return n; };
                if (countOf(Unaligned) <= 0) continue;
                ItemAlign al = Unaligned;
                do {
                    al = static_cast<ItemAlign>(static_cast<i32>(vbInt(ctx.rng.nextDouble() * 4.0)));
                } while (countOf(al) <= 0);
                for (u32 j = 0; j < store.size(); ++j) {
                    if (store[j].id == d.id && store[j].alignment == al) {
                        store.erase(store.begin() + static_cast<i64>(j));
                        break;
                    }
                }
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// R9 Item draw (DrawOne). Kind drawn by RoundedIndex over the item kinds.
// Kinds = 9 (ItemType: Potion..Other).
// UNKNOWN: kind count confirmed against the ItemType enum, not data.
// CONFLICT: the original keeps a stale "last" across passes when the run reaches the end
// of the file (c22). A stateless function cannot carry that value, so the end of the file
// is used.
// UNKNOWN: rarity field is assumed numeric; zero total returns 0 (original would fail).
// ---------------------------------------------------------------------------
i32 drawItemIndex(Ctx& ctx, i32 floor) {
    // The reference draws the kind over NumItemTypes (the item-type table, 35 entries in the shipped data), not 9.
    const i32 kItemKindCount = ctx.data.itemTypes.empty() ? 9 : static_cast<i32>(ctx.data.itemTypes.size());
    const i32 count = static_cast<i32>(ctx.data.items.size());
    const i32 kind = roundedIndex(ctx, kItemKindCount);

    // first = first i with Type == kind and Floor <= floor.
    // Index 0 is not found, whether nothing matched or item 0 matched
    // (StoreStock.cs:423 Reachable: "if (first == 0) continue;", "an index of 0 is not found").
    i32 first = -1;
    for (i32 i = 0; i < count; ++i) {
        const ItemDef& d = ctx.data.items[static_cast<u32>(i)];
        if (static_cast<i32>(d.type) == kind && d.floor <= floor) {
            first = i;
            break;
        }
    }
    if (first <= 0) {
        return 0;
    }

    // last = (first i from first with Floor > floor or Type != kind) - 1
    i32 last = count - 1;  // CONFLICT: stale-value quirk, see note above
    for (i32 i = first; i < count; ++i) {
        const ItemDef& d = ctx.data.items[static_cast<u32>(i)];
        if (d.floor > floor || static_cast<i32>(d.type) != kind) {
            last = i - 1;
            break;
        }
    }
    if (last < first) {
        last = first;
    }

    double total = 0.0;
    for (i32 i = first; i <= last; ++i) {
        total += static_cast<double>(ctx.data.items[static_cast<u32>(i)].rarity);
    }
    if (total <= 0.0) {
        return 0;  // UNKNOWN: zero total; original would divide by zero
    }

    const double pick = ctx.rng.nextDouble() * 100.0;
    i64 acc = 0;
    for (i32 i = first; i <= last; ++i) {
        const double rarity =
            static_cast<double>(ctx.data.items[static_cast<u32>(i)].rarity);
        const i64 weight = (rarity > 0.0) ? roundHalfEven(rarity / total * 100.0) : 0;
        acc += weight;
        if (pick <= static_cast<double>(acc)) {
            return i;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// R10 Rounded index: index = round_half_even(Rnd * n) (port, c19).
// Index n matches nothing and is retried.
// ---------------------------------------------------------------------------
i32 roundedIndex(Ctx& ctx, i32 n) {
    if (n <= 0) {
        return 0;  // UNKNOWN: guard for empty choice set
    }
    while (true) {
        const i64 k = roundHalfEven(ctx.rng.nextDouble() * static_cast<double>(n));
        if (k < n) {
            return static_cast<i32>(k);
        }
        // index n matches nothing: retry
    }
}

// ---------------------------------------------------------------------------
// R16 Shelving test:
// shelves = !cursed AND !(unaligned AND floor == 0) AND row_count(alignment) < 9.
// UNKNOWN: the floor is the item definition's floor. Row count is per alignment.
// Source FRMCTYS.BAS:264-267.
// ---------------------------------------------------------------------------
bool storeShelvesItem(const Ctx& ctx, const Item& item, const Vec<Item>& store) {
    if (item.id < 0 || item.id >= static_cast<i32>(ctx.data.items.size())) {
        return false;
    }
    const ItemDef& def = ctx.data.items[static_cast<u32>(item.id)];
    const bool cursed = item.cursed != CurseNone;
    const bool unaligned = item.alignment == ItemAlign::Unaligned;
    const bool floorZero = def.floor == 0;
    if (cursed) {
        return false;
    }
    if (unaligned && floorZero) {
        return false;
    }
    return storeRowCount(store, item.alignment) < kShelfRowCap;
}

// ---------------------------------------------------------------------------
// R16 row count: entries on the store shelf whose alignment matches.
// ---------------------------------------------------------------------------
i32 storeRowCount(const Vec<Item>& store, ItemAlign align) {
    i32 n = 0;
    for (const Item& it : store) {
        if (it.alignment == align) {
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// Confinement buy: take one penned companion of monsterRef for the payer.
// UNKNOWN: the R-series for confinement buy (R21 to R25) is in the other half of the SRD.
// Assumed flow: check the pen allows the take, price it with the creature's base worth and
// the penned count, charge the payer, then release one pen slot.
// UNKNOWN: monsterRef indexes ctx.data.monsters; base worth uses the payer's active level.
// ---------------------------------------------------------------------------
bool confinementBuy(Ctx& ctx, Party& party, i32 payerIndex, PenBook& pens, i32 monsterRef,
                    i32 align, const TownModAnswers& mods) {
    if (payerIndex < 0 || payerIndex >= party.size) {
        return false;
    }
    if (monsterRef < 0 || monsterRef >= static_cast<i32>(ctx.data.monsters.size())) {
        return false;
    }
    if (align < 0 || align >= kPenAlignCount) {
        return false;
    }
    if (!penTakeAllowed(pens, monsterRef, align)) {
        return false;
    }

    i32 penned = 0;
    if (static_cast<u32>(monsterRef) < pens.byRef.size()) {
        penned = pens.byRef[static_cast<u32>(monsterRef)][static_cast<u32>(align)];
    }

    const Character& payer = party.members[static_cast<u32>(payerIndex)];
    const Currency base = creatureBaseWorth(
        ctx.data.monsters[static_cast<u32>(monsterRef)], payer.activeLevel());
    const Currency cost = penBuyPrice(base, penned);

    const ChargeResult paid = townCharge(ctx, party, payerIndex, cost, mods);
    if (!paid.paid) {
        return false;
    }
    releaseOne(pens, monsterRef, align);
    ctx.say("city.penBought", "Companion released from confinement",
            {cost, static_cast<i64>(monsterRef)}, {});
    return true;
}

// ---------------------------------------------------------------------------
// Confinement sell: pen one companion and pay the seller.
// UNKNOWN: the R-series for confinement sell is in the other half of the SRD.
// Assumed flow: map the companion's alignment to a pen index, check the pen cap, price the
// sale with the pre-sale penned count, then pay the income payee.
// ---------------------------------------------------------------------------
bool confinementSell(Ctx& ctx, Party& party, i32 sellerIndex, PenBook& pens,
                     const Companion& companion, const TownModAnswers& mods) {
    if (sellerIndex < 0 || sellerIndex >= party.size) {
        return false;
    }
    // CompanionAlignment Good=1, Neutral=2, Evil=3 matches ItemAlign values.
    const ItemAlign penAlign = static_cast<ItemAlign>(static_cast<i32>(companion.alignment));
    const i32 align = penIndexOfItemAlign(penAlign);
    if (align < 0 || align >= kPenAlignCount) {
        return false;
    }
    const i32 ref = companion.ref;
    if (ref < 0 || companion.monsterId < 0 ||
        companion.monsterId >= static_cast<i32>(ctx.data.monsters.size())) {
        return false;
    }

    i32 before = 0;
    bool anyPenned = false;
    if (static_cast<u32>(ref) < pens.byRef.size()) {
        const PenCounts& c = pens.byRef[static_cast<u32>(ref)];
        before = c[static_cast<u32>(align)];
        for (i32 k = 0; k < kPenAlignCount; ++k) {
            if (c[static_cast<u32>(k)] > 0) {
                anyPenned = true;
            }
        }
    }

    const i32 payee = townIncomePayee(party, sellerIndex, mods);
    if (payee < 0 || payee >= party.size) {
        return false;
    }

    if (!penOne(pens, ref, align, companion.level)) {
        return false;
    }

    const Character& seller = party.members[static_cast<u32>(sellerIndex)];
    const Currency base = creatureBaseWorth(
        ctx.data.monsters[static_cast<u32>(companion.monsterId)], seller.activeLevel());
    const Currency price = penSellPrice(base, before, anyPenned,
                                        static_cast<i32>(companion.idLevel));

    party.members[static_cast<u32>(payee)].wallet.goldOnHand += price;
    ctx.say("city.penSold", "Companion penned for sale",
            {price, static_cast<i64>(ref)}, {});
    return true;
}

// ---------------------------------------------------------------------------
// Pen one companion of ref in pen align. Cap of 9 per alignment (R21, R24, R25).
// UNKNOWN: levelFound is not used by the SRD part given here (its gate is in the other half).
// ---------------------------------------------------------------------------
bool penOne(PenBook& pens, i32 ref, i32 align, i32 levelFound) {
    (void)levelFound;
    if (ref < 0 || align < 0 || align >= kPenAlignCount) {
        return false;
    }
    const u32 r = static_cast<u32>(ref);
    while (pens.byRef.size() <= r) {
        PenCounts zero{};
        pens.byRef.push_back(zero);
    }
    PenCounts& c = pens.byRef[r];
    const u32 a = static_cast<u32>(align);
    if (c[a] >= kPenShelfCap) {
        return false;
    }
    c[a] += 1;
    return true;
}

// ---------------------------------------------------------------------------
// Release one penned companion of ref from pen align (floor at zero).
// ---------------------------------------------------------------------------
void releaseOne(PenBook& pens, i32 ref, i32 align) {
    if (ref < 0 || align < 0 || align >= kPenAlignCount) {
        return;
    }
    const u32 r = static_cast<u32>(ref);
    if (r >= pens.byRef.size()) {
        return;
    }
    PenCounts& c = pens.byRef[r];
    const u32 a = static_cast<u32>(align);
    if (c[a] > 0) {
        c[a] -= 1;
    }
}

}  // namespace mordor
