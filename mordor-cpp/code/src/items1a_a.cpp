// items1a_a.cpp: gate, curse and alignment helpers for module items1a.
// Source: SRD c13p1 Part A, rules R1-R18 and Table T-2. The Formulas section
// (slot emptiness, alignment fit detail, asmodeus band, worth halving rounding)
// is not in this half of the SRD, so those points are marked UNKNOWN.
// Item-data field names (ItemDef / MDATA) are not in the supplied headers, so
// the per-item lookups are isolated in the helpers below with UNKNOWN fallbacks.
#include "mordor/items1a.h"
#include "mordor/guilds3.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// FIXED: kHandsSlot is not declared in any supplied header. R2 says Hands is
// internal index 0, so it is defined locally.
constexpr i32 kHandsSlot = 0;

// UNKNOWN: hand width comes from item data (not in supplied headers). Fallback 1.
i32 itemHandWidth(const Ctx& ctx, const Item& item) {
    (void)ctx;
    (void)item;
    return 1;
}

// Table T-2 wear class cap for the item: -1 no cap, 0 unwearable, >0 maximum.
// UNKNOWN: wear class is not in supplied item headers. Fallback is no cap,
// which makes the class-cap gate pass until the data field is wired.
i32 wearClassCap(const Ctx& ctx, const Item& item) {
    (void)ctx;
    (void)item;
    return kWearClassNoCap;
}

// Wear class identity used to count worn items of the same class (R7).
// UNKNOWN: fallback is kWearClassNoCap; never counted while cap is no cap.
i32 wearClassOf(const Ctx& ctx, const Item& item) {
    (void)ctx;
    (void)item;
    return kWearClassNoCap;
}

// Stat requirement for one stat (R10). UNKNOWN: item-data field not supplied.
// Fallback 0 means no requirement.
i32 itemStatRequirement(const Ctx& ctx, const Item& item, i32 statIndex) {
    (void)ctx;
    (void)item;
    (void)statIndex;
    return 0;
}

bool isItemSlot(i32 slot) {
    return slot >= kFirstItemSlot && slot <= kLastItemSlot;
}

bool anyGuildJoined(const Character& c) {
    for (i32 g = 0; g < kGuildCount; ++g) {
        if (c.guilds[static_cast<u32>(g)].joined) {
            return true;
        }
    }
    return false;
}

// UNKNOWN: refusal texts other than R12 and the three quoted in R5-R7 are not
// given in this half of the SRD. The texts below are placeholders; codes are firm.
void sayEquipRefusal(Ctx& ctx, EquipOutcome outcome) {
    switch (outcome) {
        case EquipOutcome::NoFreeHands:
            ctx.say("items.equip.no_free_hands", "No Free Hands!", {}, {});  // R6
            break;
        case EquipOutcome::TooManyWorn:
            ctx.say("items.equip.too_many_worn", "Using too Many!", {}, {});  // R7
            break;
        case EquipOutcome::ClassRestricted:
            // RESOLVED-BY-REFERENCE: Character.cs EquipProblem returns "Your current guild cannot use this item!" when it.ClassRestricted != 0 (R8)
            ctx.say("items.equip.class_restricted",
                    "Your current guild cannot use this item!", {}, {});  // R8
            break;
        case EquipOutcome::GuildRestricted:
            // RESOLVED-BY-REFERENCE: Character.cs EquipProblem returns "Your currently joined guilds cannot use this item!" when it.ClassRestricted == 0 (R8)
            ctx.say("items.equip.guild_restricted",
                    "Your currently joined guilds cannot use this item!", {}, {});  // R8
            break;
        case EquipOutcome::GuildLevelTooLow:
            // UNKNOWN: the reference has two texts, "Your guild level is too low to use item!" (ClassRestricted != 0)
            // and "All guild levels too low to use item!" (otherwise). The choice needs the item's flag, which this
            // function does not receive, and the signature must stay fixed.
            ctx.say("items.equip.guild_level",
                    "UNKNOWN: guild level message text.", {}, {});  // R9
            break;
        case EquipOutcome::StatsTooLow:
            // RESOLVED-BY-REFERENCE: Character.cs EquipProblem returns "One or more stats are too low!" (no stat named) (R10)
            ctx.say("items.equip.stats_too_low",
                    "One or more stats are too low!", {}, {});  // R10
            break;
        case EquipOutcome::CursedStatsTooLow:
            ctx.say("items.equip.cursed_stats",
                    "Your stats are too low for the item to properly attach!", {}, {});  // R12
            break;
        case EquipOutcome::SilentRefusal:
        case EquipOutcome::Ok:
            break;
    }
}

}  // namespace

// R2: Hands is internal index 0.
// CONFLICT: API INDEX lists items1b.h with an inline isHandsSlot definition.
// Both must not be included in the same TU with differing linkage.
bool isHandsSlot(i32 internalIndex) {
    return internalIndex == kHandsSlot;
}

// R3: first free slot scanning from internal index 1.
i32 firstFreePackSlot(const Inventory& inv) {
    for (i32 i = kFirstItemSlot; i <= kLastItemSlot; ++i) {
        if (slotIsEmpty(inv.pack[static_cast<u32>(i)])) {
            return i;
        }
    }
    return -1;  // UNKNOWN: no-free-slot sentinel value not given; -1 chosen.
}

// R4: slot emptiness is defined in the Formulas section (not supplied).
// UNKNOWN: simplest sensible rule, an item with id 0 is an empty slot.
bool slotIsEmpty(const Item& item) {
    return item.id == 0;
}

// R6: hands in use plus the item's hand width must not exceed 2.
// Hands in use = kHandsTotal - inv.freeHands (freeHands is the free-hand count).
bool handsCheckPasses(const Inventory& inv, i32 handWidth) {
    const i32 inUse = kHandsTotal - inv.freeHands;
    return inUse + handWidth <= kHandsTotal;
}

// R7 and Table T-2.
// cap 0: unwearable, reported as a silent refusal by equipCheck.
// cap -1: no cap.
// cap > 0: refuse when the number of equipped items of this class is >= cap.
// The equipped set is read from pack[].equipped (the authority scanned by R16),
// because inv.worn is not maintained by this module. UNKNOWN: confirm that worn
// is meant to be a derived list.
bool classCapPasses(const Ctx& ctx, const Inventory& inv, const Item& item) {
    const i32 cap = wearClassCap(ctx, item);
    if (cap == kWearClassUnwearable) {
        return false;  // R7 silent exit
    }
    if (cap > 0) {
        const i32 cls = wearClassOf(ctx, item);
        i32 worn = 0;
        for (i32 i = kFirstItemSlot; i <= kLastItemSlot; ++i) {
            const Item& other = inv.pack[static_cast<u32>(i)];
            if (other.equipped && wearClassOf(ctx, other) == cls) {
                ++worn;
            }
        }
        if (worn >= cap) {
            return false;  // R7 "Using too Many!"
        }
    }
    return true;  // Table T-2: -1 no cap, or count below maximum
}

// R10: refuse if any stat is below its requirement.
// UNKNOWN: which value is compared (current stats vs stat maximum) is in the
// SRD Unknowns. This uses the current stats, abilities.stats.
bool statsRequirementMet(const Ctx& ctx, const Character& c, const Inventory& inv, const Item& item) {
    (void)inv;
    for (i32 s = 0; s < kStatCount; ++s) {
        if (c.abilities.stats[static_cast<u32>(s)] < itemStatRequirement(ctx, item, s)) {
            return false;
        }
    }
    return true;
}

// R5: five checks in order: hands, class cap, guild, guild level, stats.
// storeLike (store, guild hall, editor) skips the hands and class checks.
// R8 guild check uses CanUse from guilds3.h. The refusal message choice
// (class-restricted vs joined-guilds) is UNKNOWN; this picks joined-guilds
// when the character has joined any guild.
// R12: a Cursed==2 item that fails the stats check gets CursedStatsTooLow.
EquipOutcome equipCheck(Ctx& ctx, const Character& c, const Inventory& inv, const Item& item, bool storeLike) {
    if (!storeLike) {
        if (!handsCheckPasses(inv, itemHandWidth(ctx, item))) {
            return EquipOutcome::NoFreeHands;  // R6
        }
        if (!classCapPasses(ctx, inv, item)) {
            if (wearClassCap(ctx, item) == kWearClassUnwearable) {
                return EquipOutcome::SilentRefusal;  // R7 cap 0 exits silently
            }
            return EquipOutcome::TooManyWorn;  // R7
        }
    }
    if (!CanUse(ctx, item, c)) {  // R8
        return anyGuildJoined(c) ? EquipOutcome::GuildRestricted
                                 : EquipOutcome::ClassRestricted;
    }
    if (!MeetsUseLevel(ctx, item, c)) {  // R9
        return EquipOutcome::GuildLevelTooLow;
    }
    if (!statsRequirementMet(ctx, c, inv, item)) {  // R10
        return item.cursed == CurseKind2 ? EquipOutcome::CursedStatsTooLow  // R12
                                         : EquipOutcome::StatsTooLow;
    }
    return EquipOutcome::Ok;
}

// R5-R11: equip the pack slot if equipCheck passes.
// Equipped state lives in pack[slot].equipped. Hands in use are reduced by the
// item's hand width (R6). ApplyEquipCurse runs on success (R11).
bool equipSlot(Ctx& ctx, Character& c, Inventory& inv, i32 slot, bool storeLike) {
    if (isHandsSlot(slot) || !isItemSlot(slot)) {
        return false;  // R2 hands never equip; out-of-range slot
    }
    Item& it = inv.pack[static_cast<u32>(slot)];
    if (slotIsEmpty(it) || it.equipped) {
        return false;  // UNKNOWN: refusal for empty or already-equipped slot not given
    }
    const EquipOutcome outcome = equipCheck(ctx, c, inv, it, storeLike);
    if (outcome != EquipOutcome::Ok) {
        sayEquipRefusal(ctx, outcome);
        return false;
    }
    it.equipped = true;
    // UNKNOWN: store-like equips still consume hands; freeHands is not clamped.
    inv.freeHands -= itemHandWidth(ctx, it);  // R6
    applyEquipCurse(ctx, it, c.alignment);  // R11
    return true;
}


// R13: a slot with the curse flag set cannot be unequipped while worn.
bool unequipAllowed(const Item& slot) {
    return !slot.slotCursed;
}

// R13, R15: unequip a worn slot.
// A flagged slot is refused silently: the SRD gives no refusal text (UNKNOWN).
bool unequipSlot(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    (void)c;
    if (isHandsSlot(slot) || !isItemSlot(slot)) {
        return false;
    }
    Item& it = inv.pack[static_cast<u32>(slot)];
    if (!it.equipped) {
        return false;  // "isn't equipped" refusal path (quirk R18 area)
    }
    if (!unequipAllowed(it)) {
        return false;  // R13
    }
    clearSlotCurseOnUnequip(ctx, it);  // R15: no-op here, flag already clear
    it.equipped = false;
    inv.freeHands += itemHandWidth(ctx, it);  // R6 hands released
    return true;
}

// R14: item generation. A Cursed==2 item attaches itself to the slot as it
// lands; a Cursed==1 item does not. Chest treasure landing in a pack slot also
// sets the flag.
void applyGenerationCurse(Ctx& ctx, Item& item, bool fromChestTreasure) {
    (void)ctx;
    if (fromChestTreasure || item.cursed == CurseKind2) {
        item.slotCursed = true;
    }
}

// R15: unequipping clears the slot curse flag, so the curse returns on re-equip.
// The path that reaches a flagged slot is UNKNOWN (the normal unequip is refused).
void clearSlotCurseOnUnequip(Ctx& ctx, Item& slot) {
    (void)ctx;
    slot.slotCursed = false;
}

// R16: EquipProblem checks guild, guild level and stats only (no hands, no class
// cap). Returns true when there is a problem.
bool equipProblem(Ctx& ctx, const Character& c, const Inventory& inv, const Item& item) {
    if (!CanUse(ctx, item, c)) {
        return true;  // guild check
    }
    if (!MeetsUseLevel(ctx, item, c)) {
        return true;  // guild level check
    }
    if (!statsRequirementMet(ctx, c, inv, item)) {
        return true;  // stats check
    }
    return false;
}

// R16-R18: forced unequip pass. Pack slots 1..kLastItemSlot are scanned.
// Unequipped items get one message each, unless joining a guild, which shows one
// summary line instead. A cursed (flagged) item prints its message first and
// stays on.
// Quirk: the summary is shown whenever any item had a problem, even if it stayed
// on, because the anyProblem flag is set before the cursed check.
// UNKNOWN: message texts below are placeholders.
void forcedUnequipPass(Ctx& ctx, Character& c, Inventory& inv, bool joiningGuild) {
    bool anyProblem = false;
    for (i32 i = kFirstItemSlot; i <= kLastItemSlot; ++i) {
        Item& it = inv.pack[static_cast<u32>(i)];
        if (!it.equipped || slotIsEmpty(it)) {
            continue;  // R16 skip
        }
        // R16: "lookup fails" skip is not modelled: no lookup is available here.
        if (!equipProblem(ctx, c, inv, it)) {
            continue;
        }
        anyProblem = true;
        if (it.slotCursed) {
            // R17: cursed item stays on; message printed first.
            if (!joiningGuild) {
                ctx.say("items.forced_unequip.cursed",
                        "You can no longer use this item.", {}, {});  // R18
            }
            continue;
        }
        unequipSlot(ctx, c, inv, i);  // R17
        if (!joiningGuild) {
            ctx.say("items.forced_unequip.removed",
                    "You can no longer use this item.", {}, {});  // R18
        }
    }
    if (joiningGuild && anyProblem) {
        ctx.say("items.forced_unequip.summary",
                "You can no longer use some of your items.", {}, {});  // R18
    }
}

// Worth of an AD value after alignment fit.
// UNKNOWN: Full keeps raw. Half halves with floor (vbInt) rounding; the SRD
// rounding is not in this half. Subtract is taken as -raw.
i32 adWorthOfFit(i32 raw, FitClass fit) {
    switch (fit) {
        case FitClass::Full:
            return raw;
        case FitClass::Half:
            // Adjacent attack/defence worth: round_half_even(raw / 2.0)  (T-5, R19)
            return static_cast<i32>(roundHalfEven(static_cast<double>(raw) / 2.0));
        case FitClass::Subtract:
            return -raw;  // Opposed: -raw
    }
    return raw;
}

// Alignment fit for an item's flag bits against the character's alignment.
// UNKNOWN: the SRD alignment rule is not in this half. Chosen behaviour:
// flags 0 means unrestricted; the unaligned bit always allows; otherwise the
// character's own alignment bit must be set.
bool alignFlagsAllow(u8 alignFlags, Align charAlign) {
    if (alignFlags == 0) {
        return true;  // UNKNOWN
    }
    if ((alignFlags & kAlignBitUnaligned) != 0) {
        return true;  // UNKNOWN
    }
    u8 bit = 0;
    switch (charAlign) {
        case Align::Good:
            bit = kAlignBitGood;
            break;
        case Align::Neutral:
            bit = kAlignBitNeutral;
            break;
        case Align::Evil:
            bit = kAlignBitEvil;
            break;
    }
    return (alignFlags & bit) != 0;
}

// Raw MDATA alignment field to ItemAlign. 0..3 map directly (Unaligned, Good,
// Neutral, Evil). UNKNOWN: out-of-range values fall back to Unaligned.
ItemAlign normaliseItemAlign(i32 rawAlignField) {
    if (rawAlignField <= 1) {
        return Unaligned;  // R21: values 0 or 1 mean Unaligned
    }
    if (rawAlignField <= 3) {
        return static_cast<ItemAlign>(rawAlignField);
    }
    return Unaligned;  // UNKNOWN: out-of-range alignment field
}

// Asmodeus drop band for an item drop level.
// UNKNOWN: the band rule is in the SRD Formulas section (not supplied).
// Fallback is the single level itemDropLevel.
LevelBand asmodeusDropBand(i32 itemDropLevel) {
    LevelBand band;
    band.lo = itemDropLevel - 2;  // R22: band runs ItemDropLevel-2 .. ItemDropLevel
    band.hi = itemDropLevel;
    return band;
}

}  // namespace mordor
