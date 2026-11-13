// city1_a.cpp: first half of the City module (SRD c16p1).
// Implements: arrival and city rest (R1 to R4), crest recharge (R5), town switch (R6),
// starting and replenished store stock (R7 to R10), store buy and selection (R11 to R14),
// store sale and shelving (R15 to R17), shelf caps (R18), and the pen helpers used by
// the confinement buy and sell.
#include "mordor/city1.h"
#include "mordor/attributes.h"

namespace mordor {

// ---------------------------------------------------------------------------
// R1 Arrival in town. Steps 1 to 6 in SRD order.
// UNKNOWN: ordering of steps 2, 4 and 5 relative to each other (SRD c48, c2, c29).
// CONFLICT: trigger for the store and pen replenish (R1 step 5) versus the wiki
// account of a waking refresh (R72, c94, c95). Implemented as the SRD states.
// ---------------------------------------------------------------------------
ArrivalResult arriveInTown(Ctx& ctx, Party& party, TownStock& town, i64& mostExperience,
                           const TownModAnswers& mods) {
    ArrivalResult result{};

    // R1 step 1: clear LostLevel and LostPosition on every member; facing is left alone.
    for (i32 i = 0; i < party.size; ++i) {
        Character& ch = party.members[static_cast<u32>(i)];
        ch.lost.position = false;
        ch.lost.level = false;
    }
    // UNKNOWN: R5 guild crest refill on city entry needs an Inventory, which Character and
    // Party do not carry. The caller is expected to run refillGuildCrests per inventory.

    // R1 step 2: Hall of Records check against most experience (save is the caller's).
    checkHallOfRecords(ctx, party, mostExperience);

    // R1 steps 3 and 4: city rest (R3) for every member, whatever their state.
    // R1 step 5: replenish store and pens on the first member's rest only (R4).
    for (i32 i = 0; i < party.size; ++i) {
        const u32 idx = static_cast<u32>(i);
        Character& ch = party.members[idx];

        // RESOLVED-BY-REFERENCE: R2 gate removed. Character.cs:910 runs the rest block for every
        // member "whatever their state ... the dead included" (FRMCTYM.BAS:107-134; MAINMOD2.BAS:
        // 6355-6360 never reads DeathState). cityRestGatePasses is no longer consulted here.
        result.rested[idx] = true;

        // Order as Character.cs:910: spell effects and resists are cleared before the SP recalc.
        // R3 step 5: the town sets Area = -1 (FRMCTYM.BAS:112).
        ch.location.area = -1;

        // R3 step 4: clear paralysis, spell effects and spell resists.
        // CONFLICT: c2 and c34 disagree in the SRD; the reference (FRMCTYM.BAS:111, 113-114:
        // SpellEffects = 0, ParalysisLevel = 0, EndAilmentsAndSpellResists) follows c2.
        ch.ailments.paralysed = false;
        ch.ailments.paralysisLevel = 0;
        ch.ailments.paralyzedRounds = 0;
        ch.ailments.levitating = false;       // SpellAilmentKind::Levitate
        ch.ailments.invisible = false;        // SpellAilmentKind::SightVeil
        ch.ailments.seeInvisible = false;     // SpellAilmentKind::SeeInvisible
        ch.ailments.protectLevel = 0;         // SpellAilmentKind::Protection
        ch.activeEffectIds.clear();
        for (i32 k = 0; k < kResistKinds; ++k) {
            ch.resistances.resist[static_cast<u32>(k)] = 0;
        }

        // R3 step 1: days computed from the hit points before the refill.
        const i64 days = cityRestDays(ch.vitals.curHits, ch.vitals.maxHits);
        ch.ageDays += static_cast<i32>(days);

        // R3 step 2: refill hit points, recalculate spell points with refill, cure
        // poison and disease (AilmentSteps = 0).
        ch.vitals.curHits = ch.vitals.maxHits;
        recalcSP(ctx, ch, true);
        ch.ailments.ailmentSteps = 0;

        // R3 step 3: heal each living companion to full (reference: only c.Hits > 0 are healed).
        // UNKNOWN: no companion container is reachable from Party or TownStock, so this
        // step cannot be executed here. The caller must apply it.

        // R4 and R1 step 5: replenish store and pens on the first member's rest only.
        if (replenishesOnRest(i)) {
            replenishStore(ctx, town.store, days, town.deepestReached);   // R8
            restockPens(ctx, town.pens, days, town.deepestReached);       // R30 (other half)
            result.townReplenished = true;
        }
    }

    // R1 step 6: mod TownArrival hook. The first non-null answer decides the deposit or pool.
    // With no answer the default is nothing (ArrivalDepositResult defaults).
    if (mods.arrival.answered) {
        result.deposit = arrivalDepositAndPool(ctx, party, mods);  // R47
    }

    return result;
}

// ---------------------------------------------------------------------------
// R1 step 2. Hall of Records: check each member against most experience using
// TotalExperience. Save of the hall is the caller's responsibility.
// UNKNOWN: hall-of-records semantics beyond "greater than" are not in this SRD part.
// ---------------------------------------------------------------------------
void checkHallOfRecords(Ctx& ctx, const Party& party, i64& mostExperience) {
    for (i32 i = 0; i < party.size; ++i) {
        const Character& ch = party.members[static_cast<u32>(i)];
        const i64 total = ch.experience;  // TotalExperience
        if (total > mostExperience) {
            mostExperience = total;
            ctx.say("city.hallOfRecords", "New most experience recorded",
                    {total}, {ch.name});
        }
    }
}

// ---------------------------------------------------------------------------
// R2 Rest gate: the city rest runs only if DungeonLevel = 0 and the character is not dead.
// UNKNOWN: DungeonLevel is a party value in the SRD, but cityRestGatePasses receives only
// the character, so the character's location.dungeonLevel is used.
// ---------------------------------------------------------------------------
bool cityRestGatePasses(const Character& ch) {
    return ch.location.dungeonLevel == 0 && !ch.isDead();
}


// ---------------------------------------------------------------------------
// R4: the store and pens are replenished by the first party member's rest only.
// ---------------------------------------------------------------------------
bool replenishesOnRest(i32 memberIndex) {
    return memberIndex == 0;
}

// ---------------------------------------------------------------------------
// R5 Guild crest recharge on city entry. Pack slots 1 to 39 (slot 0 not examined).
// A guild-issued item (Ailments 500 to 500 + NumGuilds - 1) with fewer charges than the
// crest table value is refilled to that value.
// UNKNOWN: the message is emitted once per call when at least one crest is recharged.
// UNKNOWN: the full charge value is taken from kCrestCharges (SRD table T-Q).
// ---------------------------------------------------------------------------
i32 refillGuildCrests(Ctx& ctx, Inventory& inv, const Str& ownerName, bool inParty) {
    i32 refilled = 0;
    for (i32 l = kCrestScanFirst; l <= kCrestScanLast; ++l) {
        Item& slot = inv.pack[static_cast<u32>(l)];
        if (slot.id <= 0) {
            continue;  // Pack(l).Ref must be above 0
        }
        if (!slot.isGuildItem()) {
            continue;
        }
        const i32 guildIdx = slot.guildIndex();
        if (guildIdx < 0 || guildIdx >= kGuildCount) {
            continue;
        }
        const i32 full = kCrestCharges[guildIdx];
        if (slot.charges < full) {
            slot.charges = full;
            ++refilled;
        }
    }

    if (refilled > 0) {
        // R5 step 2: trim the owner name for the in-party message.
        i32 b = 0;
        i32 e = static_cast<i32>(ownerName.size());
        while (b < e && ownerName[static_cast<u32>(b)] == ' ') {
            ++b;
        }
        while (e > b && ownerName[static_cast<u32>(e - 1)] == ' ') {
            --e;
        }
        const Str trimmed = ownerName.substr(static_cast<u32>(b), static_cast<u32>(e - b));
        if (inParty) {
            ctx.say("city.crestRecharged", "Crest recharged for \"" + trimmed + "\"",
                    {static_cast<i64>(refilled)}, {trimmed});
        } else {
            ctx.say("city.crestRecharged", "Crest recharged",
                    {static_cast<i64>(refilled)}, {});
        }
    }
    return refilled;
}

// ---------------------------------------------------------------------------
// R6 Town switch. If the leader is not in the dungeon (DungeonLevel <= 0) and not dead,
// UPDATE goes to Store, Confinement, Morgue, Guild, Bank, in that order.
// The Seer receives nothing (it has no TownSection entry).
// ---------------------------------------------------------------------------
Vec<TownSection> townSwitchOrder(const Party& party) {
    Vec<TownSection> order;
    if (party.size <= 0) {
        return order;
    }
    const Character& leader = party.members[0];
    if (party.dungeonLevel > 0 || leader.isDead()) {
        return order;
    }
    order.push_back(TownSection::Store);
    order.push_back(TownSection::Confinement);
    order.push_back(TownSection::Morgue);
    order.push_back(TownSection::Guild);
    order.push_back(TownSection::Bank);
    return order;
}

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
// R11 Alignment choice (shared by General Store and Confinement).
// Index 0..3 = Unaligned, Good, Neutral, Evil (ItemAlign order).
// ---------------------------------------------------------------------------
AlignChoiceRow alignmentChoice(const Arr<i32, 4>& counts, i32 chosenBefore) {
    AlignChoiceRow row{};
    for (i32 a = 0; a < 4; ++a) {
        const bool on = counts[static_cast<u32>(a)] > 0;
        row.enabled[static_cast<u32>(a)] = on;
        if (on) {
            row.stocked += 1;
            row.last = a;
        }
    }
    row.asks = row.stocked > 1;
    if (row.asks) {
        row.chosen = -1;
    } else if (row.stocked == 1) {
        row.chosen = row.last;
    } else {
        row.chosen = chosenBefore;
    }
    return row;
}

// ---------------------------------------------------------------------------
// R12 Store Buy row click: same rule as R11 on the row's shelf.
// ---------------------------------------------------------------------------
AlignChoiceRow storeBuyRowClick(const Arr<i32, 4>& counts, i32 chosenBefore) {
    return alignmentChoice(counts, chosenBefore);
}

// ---------------------------------------------------------------------------
// R13 steps 2 and 3. When the highlighted row sells out or is hidden, BUY and INFO are
// disabled, the pending alignment choice is withdrawn and choosing is cancelled.
// UNKNOWN: step 3 also clears the selection row; the row is left as is for sold-out.
// ---------------------------------------------------------------------------
void settleStoreSelection(StoreSelection& sel, bool soldOutOrHidden) {
    if (!soldOutOrHidden) {
        return;
    }
    sel.buyEnabled = false;
    sel.infoEnabled = false;
    sel.choosing = false;
    sel.alignment = -1;
}

// ---------------------------------------------------------------------------
// R13 step 1: while the frame is up, clicking another row is refused unless a mod allows it.
// CONFLICT: R13 cites R44 for the mod permission. TownModAnswers defines rowChangeAllowed
// under R40 (default refused). rowChangeAllowed is used here.
// ---------------------------------------------------------------------------
bool storeRowClickAllowed(const StoreSelection& sel, i32 newRow, const TownModAnswers& mods) {
    if (!sel.choosing) {
        return true;
    }
    if (newRow == sel.row) {
        return true;  // not "another row"
    }
    return mods.rowChangeAllowed;
}

// ---------------------------------------------------------------------------
// R14 Confinement alignment choice: same logic as R11 with buttons G, N, E only.
// UNKNOWN: penCounts are assumed indexed by ItemAlign (1 = Good, 2 = Neutral, 3 = Evil).
// Index 0 (Unaligned) is never offered.
// ---------------------------------------------------------------------------
AlignChoiceRow confinementRowClick(const Arr<i32, 4>& penCounts, i32 chosenBefore) {
    // Pen counts are used as given. The previous version zeroed penCounts[0],
    // which disabled pen 0 and changed the stocked count (reference: stocked 2, last 2).
    Arr<i32, 4> counts = penCounts;
    return alignmentChoice(counts, chosenBefore);
}

// ---------------------------------------------------------------------------
// R15 Store sale. Income goes to the seller's hand by default (R41 may name another member
// through townIncomePayee). Shelving follows R16. Overflow refuses the sale with nothing moved.
// UNKNOWN: ItemDef.price assumed already in Currency units (1/10000 gold).
// UNKNOWN: the receiving account cap is not in this SRD part; the i64 ceiling is used.
// The caller removes the item from the seller's pack on a successful sale.
// ---------------------------------------------------------------------------
StoreSaleResult storeSale(Ctx& ctx, Party& party, i32 sellerIndex, Item& item, Vec<Item>& store,
                          const TownModAnswers& mods) {
    constexpr Currency kCurrencyCeiling = 9223372036854775807LL;
    StoreSaleResult out{};

    if (item.id < 0 || item.id >= static_cast<i32>(ctx.data.items.size())) {
        return out;
    }
    const ItemDef& def = ctx.data.items[static_cast<u32>(item.id)];

    // R15 steps 1 and 2: payee (seller by default; TownIncomeBank may name another).
    const i32 payee = townIncomePayee(party, sellerIndex, mods);
    if (payee < 0 || payee >= party.size) {
        return out;
    }
    Character& receiver = party.members[static_cast<u32>(payee)];

    // Sale amount: the store pays a share of base value (R17).
    const Currency amount = generalStorePayout(def.price);

    // R15 step 4: overflow refuses the sale with nothing moved.
    if (receiver.wallet.goldOnHand > kCurrencyCeiling - amount) {
        return out;
    }

    // R15 step 3 and R16: shelving test.
    const bool shelves = storeShelvesItem(ctx, item, store);

    receiver.wallet.goldOnHand += amount;
    out.sold = true;
    out.payee = payee;
    out.amount = amount;

    if (shelves) {
        store.push_back(item);
        out.shelved = true;
        // R15 step 3: a sale that shelves nothing raises no StoreShelved event.
        ctx.say("city.storeShelved", "Item shelved in the General Store",
                {static_cast<i64>(store.size())}, {});
    }

    ctx.say("city.storeSale", "Item sold to the General Store",
            {amount, static_cast<i64>(payee)}, {});
    return out;
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
// R17 General Store buy rate: sellPrice = 0.5 * baseValue (wiki, c87).
// UNKNOWN: rounding of half units; round half-even used.
// ---------------------------------------------------------------------------
Currency generalStorePayout(Currency baseValue) {
    return roundHalfEven(0.5 * static_cast<double>(baseValue));
}

// ---------------------------------------------------------------------------
// R18 Shelf floor cap: deepest reached capped at mc0118 - 1 = 14.
// Original off-by-one quirk (c22, c27, c63): floor 15 is never stocked.
// ---------------------------------------------------------------------------
i32 storeShelfFloorCap(i32 deepestReached) {
    return (deepestReached >= kTownDeepestFloor) ? kTownDeepestFloor : deepestReached;
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
