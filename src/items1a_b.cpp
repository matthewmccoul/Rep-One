// items1a_b.cpp: second half of module items1a (rules R19-R44 and the
// generation, use, combat-use, drop and pack-swap helpers listed in the
// module assignment). Other functions of items1a.h live in items1a.cpp /
// items1a_2.cpp and are not defined here.

#include "mordor/items1a.h"
#include "mordor/items1b.h"   // applyStatMod (defined elsewhere, called here)
#include "mordor/math_util.h"

namespace mordor {

// R23 / R89: charges are rolled with RollCharges.
// R89 gives the spread as Int(rng x charges / 2) (kFoundChargesDivisor).
// UNKNOWN: the exact roll around the spread is not in the supplied SRD.
// Chosen behaviour: roll uniformly in [charges - spread, charges + spread].
i32 rollCharges(Ctx& ctx, i32 charges) {
    if (charges <= 0) return charges;  // UNKNOWN: no roll for non-positive charges
    const i32 spread = static_cast<i32>(
        vbInt(ctx.rng.nextDouble() * charges / kFoundChargesDivisor));
    const i32 low = charges - spread;
    return low + static_cast<i32>(ctx.rng.below(2 * spread + 1));
}

// R25: identification on pickup runs through the reading formula
// (see Identification section).
// UNKNOWN: the reading formula is not in the supplied SRD extract.
// Chosen behaviour: the reader cannot raise identification beyond the
// item's current level, so the current level is returned.
IdLevel readableLevel(Ctx& ctx, const Character& reader, const Item& item) {
    (void)ctx;
    (void)reader;
    return item.idLevel;  // UNKNOWN: reading formula not supplied
}

// R25: identification on pickup. Identification only goes up.
void readOnPickup(Ctx& ctx, const Character& reader, Item& item) {
    const IdLevel level = readableLevel(ctx, reader, item);
    if (level > item.idLevel) {
        item.idLevel = level;
    }
}

// R29: incapacitated characters refuse item use silently.
// UNKNOWN: the incapacitated condition is not defined in the supplied SRD.
// Chosen behaviour: dead or paralysed characters are incapacitated.
bool isIncapacitated(const Character& c) {
    return c.isDead() || c.ailments.paralysed;  // UNKNOWN
}

// R33 / Table of MustBeEquipped types.
// Original quirk: type 0 is missing from the equip-required list.
// UNKNOWN: the list of types is not in the supplied SRD. Chosen behaviour:
// types 1..8 (Scroll through Other) must be worn; type 0 (Potion) need not.
bool isMustBeEquippedType(i32 itemTypeCode) {
    // R81: MustBeEquipped covers types 1-22, 29, 32, 33, 34.
    return (itemTypeCode >= 1 && itemTypeCode <= 22)
        || itemTypeCode == 29
        || itemTypeCode == 32
        || itemTypeCode == 33
        || itemTypeCode == 34;
}


// R28-R39: use runs in this order: incapacitated, negative SpecialType,
// charges, must-be-equipped, spell alignment, charge spend, non-spell
// alignment, special effects.
// UNKNOWN: the position of the dungeon-only refusal (R34) is not given
// by R28. Chosen behaviour: checked after must-be-equipped, before the
// spell alignment check.
UseOutcome useItem(Ctx& ctx, Character& user, Inventory& inv, i32 slot, bool inDungeon) {
    // R29: incapacitated -> silent refusal.
    if (isIncapacitated(user)) return UseOutcome::Silent;
    if (slot < 0 || slot >= kPackSlots) return UseOutcome::Silent;  // UNKNOWN: bounds guard

    // R30: negative SpecialType -> silent refusal.
    // UNKNOWN: ItemDef field name for SpecialType is not in the supplied
    // header; assumed to be `specialType`, indexed by item.id.
    {
        i32 specialType = 0;
        const Item& probe = inv.pack[static_cast<u32>(slot)];
        if (probe.id >= 0 && probe.id < static_cast<i32>(ctx.data.items.size())) {
            specialType = ctx.data.items[static_cast<u32>(probe.id)].specialType;
        }
        if (specialType < 0) return UseOutcome::Silent;
    }

    Item& item = inv.pack[static_cast<u32>(slot)];

    // RESOLVED-BY-REFERENCE: ItemUse.cs Problem: "if (it.Charges <= 0) return ...doesn't appear to do
    // much of anything." The test is on the item kind's charges, mapped here to maxCharges, and is <= 0.
    if (item.maxCharges <= 0) {
        ctx.say("use.noEffect", "The Item doesn't appear to do much of anything.", {}, {});
        return UseOutcome::ItemNoEffect;
    }

    // R32: slot charges 0 -> "out of charges".
    if (item.charges <= 0) {
        ctx.say("use.outOfCharges", "The Item is out of charges!", {}, {});
        return UseOutcome::SlotOutOfCharges;
    }

    // R34: dungeon-only use (position UNKNOWN, see function comment).
    if (!inDungeon) {
        ctx.say("use.notInDungeon", "Must be in Dungeon to Use items.", {}, {});
        return UseOutcome::NotInDungeon;
    }

    // R35 / R33: spell items only. RESOLVED-BY-REFERENCE: ItemUse.cs Problem checks the spell
    // alignment first and the must-be-equipped test second, both inside "if (it.SpellIndex > 0)".
    // A non-spell item is not refused for being unworn. Only "Nothing Happens." spends no charge.
    if (item.spellId > 0) {
        if (!spellFiresOnUse(item, user.alignment)) {
            ctx.say("use.nothingHappens", "Nothing Happens.", {}, {});
            return UseOutcome::NothingHappens;
        }
        if (isMustBeEquippedType(static_cast<i32>(item.type)) && !item.equipped) {
            ctx.say("use.mustBeEquipped", "Item must be Equipped to use!", {}, {});
            return UseOutcome::MustBeEquipped;
        }
    }
    // Spell firing itself is performed by the casting module; not done here.

    // R19 / Table T-5: fit of the item relative to the user. Computed before
    // the charge spend (the item may be destroyed by the spend).
    FitClass fit = FitClass::Full;
    if (item.alignment != ItemAlign::Unaligned) {
        const i32 itemStep = static_cast<i32>(item.alignment) - 1;  // Good=0, Neutral=1, Evil=2
        const i32 userStep = static_cast<i32>(user.alignment);      // Good=0, Neutral=1, Evil=2
        const i32 steps = itemStep > userStep ? itemStep - userStep : userStep - itemStep;
        if (steps == 1) {
            fit = FitClass::Half;
        } else if (steps == 2) {
            fit = FitClass::Subtract;
        }
    }

    // R36: charge spend.
    const bool destroyed = subtractItemUse(ctx, inv, slot);
    if (destroyed) {
        resetEmptiedBuffers(user, slot);  // R66 / R76
    }

    // R37: one alignment step away -> "Nothing Happens." (charge already spent).
    if (fit == FitClass::Half) {
        ctx.say("use.nothingHappens", "Nothing Happens.", {}, {});
        return UseOutcome::NothingHappens;
    }

    // R37 / R38: non-spell fit, then special effects.
    // Two steps away reverses positive modifiers (reversed = true).
    applyNonSpellFit(ctx, user, item, fit);
    applySpecialEffects(ctx, user, inv, slot, fit == FitClass::Subtract);

    return destroyed ? UseOutcome::UsedAndDestroyed : UseOutcome::Used;
}

// R35: the spell fires only if the item is unaligned or its alignment
// matches the user's alignment exactly. No half-strength case.
bool spellFiresOnUse(const Item& item, Align userAlign) {
    if (item.alignment == ItemAlign::Unaligned) return true;
    switch (userAlign) {
        case Align::Good:    return item.alignment == ItemAlign::Good;
        case Align::Neutral: return item.alignment == ItemAlign::Neutral;
        case Align::Evil:    return item.alignment == ItemAlign::Evil;
    }
    return false;
}

// R36: spend one charge. On the last charge, consumed types are destroyed.
// Other items stay at zero charges (including guild crests; original quirk).
// Returns true if the item was destroyed (slot emptied).
// The caller must reset spell buffers for an emptied slot (resetEmptiedBuffers).
bool subtractItemUse(Ctx& ctx, Inventory& inv, i32 slot) {
    (void)ctx;
    if (slot < 0 || slot >= kPackSlots) return false;  // UNKNOWN: bounds guard
    Item& item = inv.pack[static_cast<u32>(slot)];
    if (item.charges > 0) {
        item.charges -= 1;  // R36
    }
    if (item.charges == 0 && isConsumedType(static_cast<i32>(item.type))) {
        item = Item{};  // R36: destroyed. UNKNOWN: empty-slot representation
        return true;
    }
    return false;
}

// R37 / Table T-5: non-spell effects by fit.
// Half (one step away): "Nothing Happens." is printed by useItem; no effect.
// Subtract (two steps away): positive modifiers are reversed. The reversal
// is carried by the reversed flag passed to applySpecialEffects.
// Full: normal effects.
// UNKNOWN: magnitude scaling (round_half_even / floor / negate) for the
// special effects is not in the supplied SRD extract.
void applyNonSpellFit(Ctx& ctx, Character& user, Item& item, FitClass fit) {
    (void)ctx;
    (void)user;
    (void)item;
    (void)fit;
    // Full: no adjustment. Half: no effect (R37). Subtract: handled by caller.
}

// R38: the spell fires first (casting module), then SpecialType effects
// (Table T-4). When reversed, positive modifiers are reversed (R37).
// UNKNOWN: Table T-4 (SpecialType -> effect mapping) is not in the supplied
// SRD extract, so no SpecialType effect is applied here.
// NOTE: for a consumable whose last charge was spent, the slot is already
// empty when this runs; the SpecialType read is lost. Resolve with T-4.
void applySpecialEffects(Ctx& ctx, Character& user, Inventory& inv, i32 slot, bool reversed) {
    (void)ctx;
    (void)user;
    (void)inv;
    (void)slot;
    (void)reversed;
}

// Years of youth removed from the character's age.
// UNKNOWN: the aging rule is not in the supplied SRD extract. Chosen
// behaviour: 365 days per year (kAgeStartDays = 16 x 365), age floored at 0.
void applyYounger(Ctx& ctx, Character& user, i32 years) {
    const i32 floorDays = 16 * 365;  // R82: minimum age 16 * 365
    user.ageDays -= years * 365;     // R82: age -= years * 365
    if (user.ageDays < floorDays) {
        user.ageDays = floorDays;
        ctx.say("items.younger.limit", "You cannot get any younger.", {}, {});
    }
}

// Stat modification (reversed = positive modifiers are reversed).
// Delegates to the shared stat-modification rule in items1b (applyStatMod).
void applyStatModifier(Ctx& ctx, Character& user, StatIndex stat, i32 mod, bool reversed) {
    (void)applyStatMod(ctx, user, stat, mod, reversed);
}

// R40: a combat item becomes the character's order. Each round it casts its
// spell and takes one charge. At zero charges the order drops.
// Points are charged only for an item of level 0 (cost UNKNOWN: not in the
// supplied SRD extract, so no points are deducted here).
// UNKNOWN: the CombatOrderKind used for an item order is not stated.
// Chosen behaviour: Buffer.
void combatItemRound(Ctx& ctx, Character& who, Inventory& inv, i32 slot) {
    if (slot < 0 || slot >= kPackSlots) return;  // UNKNOWN: bounds guard

    Item& item = inv.pack[static_cast<u32>(slot)];
    if (item.charges <= 0) {
        who.order = CombatOrder{};  // R40: order drops at zero charges
        return;
    }

    // R40: the combat item becomes the order.
    who.order.kind = CombatOrderKind::Buffer;  // UNKNOWN: kind
    who.order.spellId = item.spellId;

    // R40: the spell is cast this round (casting module performs the cast).
    who.castThisRound = true;

    // R40: one charge per round.
    const bool destroyed = subtractItemUse(ctx, inv, slot);
    if (destroyed) {
        resetEmptiedBuffers(who, slot);  // R66 / R76
    }

    // R40: at zero charges the order drops (also when the item was destroyed).
    if (inv.pack[static_cast<u32>(slot)].charges <= 0) {
        who.order = CombatOrder{};
    }
}

// R41-R44: drop an item from the pack.
// R41: guild-issued items refuse ("Cannot drop Guild-Issued Items!").
//      Slot 0 (hands) cannot be dropped.
// R42: an equipped cursed item is refused after the confirmation prompt.
//      UNKNOWN: "cursed" is read as the slot curse flag (slotCursed).
// R43: an unworn cursed item drops normally.
// R44: any other item is destroyed. Nothing is left on the floor.
// UNKNOWN: an invalid slot index is reported as HandsSlot.
DropOutcome dropItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot, bool confirmed) {
    if (slot < 0 || slot >= kPackSlots) return DropOutcome::HandsSlot;  // UNKNOWN

    Item& item = inv.pack[static_cast<u32>(slot)];

    // R41: guild-issued items.
    if (item.guildIssued) {
        ctx.say("drop.guildIssued", "Cannot drop Guild-Issued Items!", {}, {});
        return DropOutcome::GuildIssued;
    }

    // R41: slot 0 cannot be dropped.
    if (isHandsSlot(slot)) return DropOutcome::HandsSlot;

    // R42: equipped cursed item: prompt first, then refuse.
    if (item.equipped && item.slotCursed) {
        if (!confirmed) return DropOutcome::NeedsConfirm;
        return DropOutcome::CursedEquipped;
    }

    // R43 / R44: unworn cursed items and all other items are destroyed.
    inv.pack[static_cast<u32>(slot)] = Item{};
    resetEmptiedBuffers(c, slot);  // R66 / R76
    return DropOutcome::Destroyed;
}

// Swap within pack, addressed by player slot numbers.
// Player number = internal index + 1 (kPlayerSlotOffset).
// UNKNOWN: the rule for typed swaps is not in the supplied SRD extract
// (section "Swap within pack" is empty). Chosen behaviour: both slots must
// be item slots (kFirstItemSlot..kLastItemSlot) and different; then swap.
// Returns false when the swap is not made. Buffers are not touched here
// (no Character argument); the dragged variant handles them.
bool swapPackTyped(Ctx& ctx, Inventory& inv, i32 playerSlotA, i32 playerSlotB) {
    (void)ctx;
    const i32 a = playerSlotA - kPlayerSlotOffset;
    const i32 b = playerSlotB - kPlayerSlotOffset;
    if (a < kFirstItemSlot || a > kLastItemSlot) return false;  // UNKNOWN
    if (b < kFirstItemSlot || b > kLastItemSlot) return false;  // UNKNOWN
    if (a == b) return false;                                    // UNKNOWN
    const Item tmp = inv.pack[static_cast<u32>(a)];
    inv.pack[static_cast<u32>(a)] = inv.pack[static_cast<u32>(b)];
    inv.pack[static_cast<u32>(b)] = tmp;
    return true;
}

// Swap within pack, addressed by internal slot indices (drag).
// UNKNOWN: the rule for dragged swaps is not in the supplied SRD extract.
// Chosen behaviour: both slots must be item slots and different; swap the
// items and their spell-buffer references.
void swapPackDragged(Ctx& ctx, Character& c, Inventory& inv, i32 slotA, i32 slotB) {
    (void)ctx;
    if (slotA < kFirstItemSlot || slotA > kLastItemSlot) return;  // UNKNOWN
    if (slotB < kFirstItemSlot || slotB > kLastItemSlot) return;  // UNKNOWN
    if (slotA == slotB) return;                                    // UNKNOWN
    const Item tmp = inv.pack[static_cast<u32>(slotA)];
    inv.pack[static_cast<u32>(slotA)] = inv.pack[static_cast<u32>(slotB)];
    inv.pack[static_cast<u32>(slotB)] = tmp;
    swapBufferSlots(c, slotA, slotB);
}

// R66 / R76: when a slot is emptied, every spell buffer that refers to it
// is reset to kSlotEmptyBuffer (-1).
void resetEmptiedBuffers(Character& c, i32 slot) {
    for (i32 i = 0; i < kSpellBuffers; ++i) {
        const u32 idx = static_cast<u32>(i);
        if (c.order.spellBuffers[idx] == slot) {
            c.order.spellBuffers[idx] = kSlotEmptyBuffer;
        }
    }
}

}  // namespace mordor
