// items1a_c.cpp: implementation of the SRD c13p1 (half c) item rules.
// Functions defined here: swapBufferSlots, giveRead, lootReadOnReceive,
// storeCombine, storeBuy, realignCost, realignMultiplier, uncurseItem,
// isGuildCrest, sellQuote, parseTypedAmount, bankDepositAllowed,
// bankWithdrawAllowed, depositItem, retrieveFromBank,
// bankRetrievalReidentifies, slimeRollReaches, pickItemSlot, isDestructibleItem.

#include "mordor/items1a.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {
// UNKNOWN: VB Integer limit (32767) assumed as the charge overflow point (R56).
constexpr i64 kChargeSumLimit = 32767;
}  // namespace

// R46 dragged swap: buffers follow the item they name.
// UNKNOWN: whether a non-zero swap always remaps buffers is unresolved in the SRD;
// this implementation always remaps.
void swapBufferSlots(Character& c, i32 slotA, i32 slotB) {
    for (i32 b = 0; b < kSpellBuffers; ++b) {
        i32& buf = c.order.spellBuffers[static_cast<u32>(b)];
        if (buf == slotA) {
            buf = slotB;
        } else if (buf == slotB) {
            buf = slotA;
        }
    }
}

// R53 give path (Party.ReadGiven).
void giveRead(Ctx& ctx, Character& receiver, Item& slot) {
    if (slot.idLevel == All) {
        return;
    }
    IdLevel reading = readableLevel(ctx, receiver, slot);
    if (reading <= slot.idLevel) {
        return;
    }
    slot.idLevel = reading;
    // UNKNOWN: "file the item in the library" has no library API in this SRD half;
    // the filing step is not performed here.
    ctx.say("items.give_identified",
            "<0> identified the given item as <1>.",
            {},
            {receiver.name, itemDisplayName(ctx, slot, reading)});
}

// R52 exchange path (Loot.OnReceived).
// Original quirk: the second reading uses Items[LevelScale] instead of the item.
void lootReadOnReceive(Ctx& ctx, const Character& receiver, Item& slot) {
    if (slot.idLevel == All) {
        return;
    }
    if (readableLevel(ctx, receiver, slot) <= slot.idLevel) {
        return;
    }
    // UNKNOWN: ItemDef field name for LevelScale is assumed to be `levelScale`.
    // Items[LevelScale] ?? it: if the index is not a valid table entry, use the item itself.
    Item scaled = slot;
    if (slot.id >= 0 && slot.id < static_cast<i32>(ctx.data.items.size())) {
        i32 scale = ctx.data.items[static_cast<u32>(slot.id)].levelScale;
        if (scale >= 0 && scale < static_cast<i32>(ctx.data.items.size())) {
            scaled.id = scale;
        }
    }
    slot.idLevel = readableLevel(ctx, receiver, scaled);
}

// R54 - R57 Store Combine.
CombineResult storeCombine(Ctx& ctx, Inventory& storeInv, const Vec<i32>& slots) {
    CombineResult result;

    // R54 refusal is checked first.
    // UNKNOWN: refusal conditions are not itemised in the SRD; implemented as
    // "a listed slot is outside the pack item range".
    for (i32 s : slots) {
        if (s < kFirstItemSlot || s > kLastItemSlot) {
            result.outcome = CombineOutcome::Refused;
            return result;
        }
    }

    // R54 a list of one item does nothing.
    if (slots.size() < 2) {
        result.outcome = CombineOutcome::NothingToCombine;
        return result;
    }

    const Item& first = storeInv.pack[static_cast<u32>(slots[0])];

    // R54 the first item's type must be combinable.
    // UNKNOWN: combinable types are not listed in the SRD; isConsumedType is used as the test.
    if (!isConsumedType(static_cast<i32>(first.type))) {
        result.outcome = CombineOutcome::FirstNotCombinable;
        return result;
    }

    // R54 every item must be fully identified; each later item is then checked in order:
    // type, item id, alignment.
    const u32 count = static_cast<u32>(slots.size());
    for (u32 k = 0; k < count; ++k) {
        const Item& it = storeInv.pack[static_cast<u32>(slots[k])];
        if (it.idLevel != All) {
            result.outcome = CombineOutcome::NotIdentified;
            return result;
        }
        if (k == 0) {
            continue;
        }
        if (it.type != first.type) {
            // Original quirk: the "not of the same type" message is swapped in the original.
            result.outcome = CombineOutcome::TypeMismatch;
            return result;
        }
        if (it.id != first.id) {
            result.outcome = CombineOutcome::ItemIdMismatch;
            return result;
        }
        if (it.alignment != first.alignment) {
            result.outcome = CombineOutcome::AlignMismatch;
            return result;
        }
    }

    // R55 / R56 merge pass. Validation completes before any change is made.
    // UNKNOWN: the SRD does not say whether validation is per-item or all-first;
    // all-first is used.
    Item& head = storeInv.pack[static_cast<u32>(slots[0])];
    for (u32 k = 1; k < count; ++k) {
        const i32 s = slots[k];
        Item& it = storeInv.pack[static_cast<u32>(s)];
        const i64 total = static_cast<i64>(head.charges) + static_cast<i64>(it.charges);
        if (total > kChargeSumLimit) {
            // R56 overflow: the item's message shows but its charges are not added.
            // Original quirk: VB run-time error 6 is untrapped in the original (program ends).
            // UNKNOWN: slot is left unchanged on overflow.
            result.overflowed += 1;
            ctx.say("items.combine.overflow",
                    "Charges could not be combined from this item.",
                    {s},
                    {});
            continue;
        }
        // R55 charges added to the first item, passing item's slot emptied.
        head.charges = static_cast<i32>(total);
        it = Item{};
        result.merged += 1;
        ctx.say("items.combine.merged",
                "Charges combined into the first item.",
                {s},
                {});
    }
    result.outcome = CombineOutcome::Combined;
    return result;
}

// R58 store buy.
BuyOutcome storeBuy(Ctx& ctx, Character& buyer, Inventory& inv, const Item& stock, Currency price) {
    // R58 scan slots 1 to 39 (internal indices; hands slot 0 excluded) for a blank.
    i32 freeSlot = -1;
    for (i32 s = kFirstItemSlot; s <= kLastItemSlot; ++s) {
        if (slotIsEmpty(inv.pack[static_cast<u32>(s)])) {
            freeSlot = s;
            break;
        }
    }
    if (freeSlot < 0) {
        ctx.say("items.buy.no_free_slot", "No free slots!", {}, {});
        return BuyOutcome::NoFreeSlot;
    }
    if (buyer.wallet.goldOnHand < price) {
        ctx.say("items.buy.not_enough_gold", "Not enough gold!", {}, {});
        return BuyOutcome::NotEnoughGold;
    }
    buyer.wallet.goldOnHand -= price;
    inv.pack[static_cast<u32>(freeSlot)] = stock;
    // UNKNOWN: slot number shown as player numbering (internal index + 1).
    const i32 shown = freeSlot + kPlayerSlotOffset;
    ctx.say("items.buy.success",
            "Slot #<0> now contains your <1>.",
            {shown},
            {itemDisplayName(ctx, stock, stock.idLevel)});
    return BuyOutcome::Bought;
}

// R60 realign cost = round_half_even(0.72 x Price x mult).
Currency realignCost(Currency price, i32 multiplier) {
    return roundHalfEven(0.72 * static_cast<double>(price) * static_cast<double>(multiplier));
}

// R60 multiplier: 1 for a neighbouring step, 2 for the full spectrum (Table T-11).
// Original quirk: uses Abs on the raw alignment index, not a true spectrum distance.
i32 realignMultiplier(ItemAlign from, ItemAlign to) {
    i32 diff = static_cast<i32>(from) - static_cast<i32>(to);
    if (diff < 0) {
        diff = -diff;
    }
    // UNKNOWN: realigning to the same alignment is not defined in the SRD; cost 0 is used.
    if (diff == 0) {
        return 0;
    }
    if (diff == 1) {
        return 1;
    }
    return 2;
}

// R61 uncurse.
UncurseOutcome uncurseItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    (void)c;
    Item& item = inv.pack[static_cast<u32>(slot)];
    if (!item.slotCursed) {
        ctx.say("items.uncurse.not_cursed", "Item is not cursed!", {}, {});
        return UncurseOutcome::NotCursed;
    }
    if (!item.equipped) {
        ctx.say("items.uncurse.not_equipped", "Item isn't equipped!", {}, {});
        return UncurseOutcome::NotEquipped;
    }
    // R61 otherwise the uncurse proceeds.
    // UNKNOWN: the SRD half does not state what the uncurse changes; the slot flag is cleared.
    item.slotCursed = false;
    ctx.say("items.uncurse.done", "The curse is lifted.", {}, {});
    return UncurseOutcome::Uncursed;
}

// R62 guild crest test.
// UNKNOWN: crest identification rule is not in this SRD half. Crests are assumed to be
// items whose id lies in (kCrestItemIdBase, kCrestItemIdBase + kGuildCount].
bool isGuildCrest(const Item& item) {
    return item.id > kCrestItemIdBase && item.id <= kCrestItemIdBase + kGuildCount;
}

// R62 sell quote: crests are refused before a price is quoted.
bool sellQuote(Ctx& ctx, const Item& item, Currency& quote) {
    if (isGuildCrest(item)) {
        return false;
    }
    quote = sellPrice(ctx, item);
    return true;
}

// R63 typed amount: empty, non-numeric or zero is ignored (returns false).
// Accepts optional leading spaces, an optional sign and an optional decimal part (up to
// 4 places). Returns the amount in 1/10000 gold.
// UNKNOWN: the SRD does not specify the accepted syntax; this is the simplest sensible choice.
bool parseTypedAmount(const Str& text, Currency& amount) {
    using SizeT = decltype(text.size());
    const SizeT n = text.size();
    SizeT i = 0;
    while (i < n && text[i] == ' ') {
        ++i;
    }
    bool negative = false;
    if (i < n && (text[i] == '-' || text[i] == '+')) {
        negative = (text[i] == '-');
        ++i;
    }
    // UNKNOWN: upper cap on whole gold to keep the value within i64.
    const i64 kMaxWhole = 1000000000000LL;
    i64 whole = 0;
    bool sawDigit = false;
    while (i < n && text[i] >= '0' && text[i] <= '9') {
        sawDigit = true;
        whole = whole * 10 + static_cast<i64>(text[i] - '0');
        if (whole > kMaxWhole) {
            return false;
        }
        ++i;
    }
    i64 frac = 0;
    i32 fracDigits = 0;
    if (i < n && text[i] == '.') {
        ++i;
        while (i < n && text[i] >= '0' && text[i] <= '9') {
            sawDigit = true;
            if (fracDigits < 4) {
                frac = frac * 10 + static_cast<i64>(text[i] - '0');
                ++fracDigits;
            }
            ++i;
        }
    }
    while (fracDigits < 4) {
        frac *= 10;
        ++fracDigits;
    }
    while (i < n && text[i] == ' ') {
        ++i;
    }
    if (i != n || !sawDigit) {
        return false;
    }
    const i64 value = whole * 10000 + frac;
    if (value == 0) {
        return false;
    }
    amount = negative ? -value : value;
    return true;
}

// R63 deposit beeps if amount > hand or amount <= 0.
bool bankDepositAllowed(Currency amount, Currency goldOnHand) {
    return amount > 0 && amount <= goldOnHand;
}

// R63 withdrawal beeps if amount > bank or bank < 0.
// Original quirk: a negative amount is not refused here (see R63 overflow quirk).
bool bankWithdrawAllowed(Currency amount, Currency bankGold) {
    return !(amount > bankGold || bankGold < 0);
}

// R65 deposit an item from the pack into the first free bank slot.
// UNKNOWN: the SRD does not state the bank-full message or buffer behaviour on deposit;
// buffers are left unchanged and a generic message is used.
bool depositItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    (void)c;
    Item& item = inv.pack[static_cast<u32>(slot)];
    if (item.equipped) {
        ctx.say("items.bank.equipped", "Cannot put equipped items in the bank!", {}, {});
        return false;
    }
    if (slotIsEmpty(item)) {
        return false;
    }
    i32 bankFree = -1;
    for (i32 b = 0; b < kBankSlots; ++b) {
        if (slotIsEmpty(inv.bank[static_cast<u32>(b)])) {
            bankFree = b;
            break;
        }
    }
    if (bankFree < 0) {
        ctx.say("items.bank.full", "No free bank slots!", {}, {});
        return false;
    }
    inv.bank[static_cast<u32>(bankFree)] = item;
    item = Item{};
    ctx.say("items.bank.deposited", "Item deposited in the bank.", {}, {});
    return true;
}

// R66 retrieve an item from the bank into the first free pack slot.
// UNKNOWN: "buffers pointing at the slot" is taken as the destination pack slot.
bool retrieveFromBank(Ctx& ctx, Character& c, Inventory& inv, i32 bankSlot) {
    Item& stored = inv.bank[static_cast<u32>(bankSlot)];
    if (slotIsEmpty(stored)) {
        return false;
    }
    const i32 dest = firstFreePackSlot(inv);
    if (dest < kFirstItemSlot || dest > kLastItemSlot) {
        ctx.say("items.retrieve.no_free_slot", "No free slots!", {}, {});
        return false;
    }
    Item moved = stored;
    if (bankRetrievalReidentifies(ctx, c, moved)) {
        // UNKNOWN: re-identified level taken as the reading, never lowering the known level.
        const IdLevel level = readableLevel(ctx, c, moved);
        if (level > moved.idLevel) {
            moved.idLevel = level;
        }
        ctx.say("items.retrieve.reidentified",
                "You have re-identified the retrieved item as: <0>",
                {},
                {itemDisplayName(ctx, moved, moved.idLevel)});
    }
    inv.pack[static_cast<u32>(dest)] = moved;
    stored = Item{};
    // R66 original quirk: buffers are set to -1, not remapped.
    for (i32 b = 0; b < kSpellBuffers; ++b) {
        i32& buf = c.order.spellBuffers[static_cast<u32>(b)];
        if (buf == dest) {
            buf = kSlotEmptyBuffer;
        }
    }
    return true;
}

// R66 Bank retrieval test (see Formulas).
// UNKNOWN: the formula is in the Formulas section, which is not part of this SRD half.
// Simplest sensible choice: no re-identification.
bool bankRetrievalReidentifies(Ctx& ctx, const Character& c, const Item& item) {
    (void)ctx;
    (void)c;
    (void)item;
    return false;
}

// R68 slime roll: roll 1..100, threshold = 50 + 2 x dungeonLevel.
// UNKNOWN: the comparison operator is not given in this SRD half; "roll <= threshold" is used.
bool slimeRollReaches(Ctx& ctx, i32 dungeonLevel) {
    const i32 roll = static_cast<i32>(ctx.rng.below(kSlimeRollMax)) + 1;
    const i32 threshold = kSlimeRollBase + kSlimeRollPerLevel * dungeonLevel;
    return roll <= threshold;
}

// UNKNOWN: the rule for pickItemSlot is not in this SRD half. The R72 formula
// Int(Rnd x 39) + 2 (player numbering) is used and converted to an internal index.
i32 pickItemSlot(Ctx& ctx) {
    // Formulas (slime): slot = rng.Next(39) + 1, slots 1..39 (R69).
    return static_cast<i32>(ctx.rng.below(39)) + 1;
}

// R70 destructible special types.
// UNKNOWN: ItemDef field name is assumed to be `specialType`; the lower bound is assumed to be 1.
bool isDestructibleItem(const ItemDef& def) {
    return def.specialType >= 1 && def.specialType <= kDestructibleSpecialMax;
}

}  // namespace mordor
