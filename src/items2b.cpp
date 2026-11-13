// items2b.cpp: Part B items. Giving, dragging, equip/drop/use, item orders,
// chests, theft, pack re-read, bank withdrawal, swap parsing.
//
// CONFLICT: items2b.h uses EquipOutcome, UseOutcome, DropOutcome, UncurseOutcome,
// RealignOutcome, BuyOutcome, CombineOutcome (items1a.h) and SwapResult (items1b.h)
// without including the headers that define them. This file includes both before
// items2b.h. Enumerator values were not supplied, so the enumerator names used here
// are placeholders to reconcile with items1a.h / items1b.h.
// CONFLICT: canBankItem(const Item&) is declared in items1b.h and items2b.h with the
// same signature. Defined once, here.
// CONFLICT: uncurseItem(Ctx&,Character&,Inventory&,i32) is declared in items1a.h and
// items2b.h with the same signature. Defined once, in items2b_2.cpp.
// UNKNOWN: slot numbering. Inventory indices are 0-based; index 0 is Hands; storage
// is indices 1..39. A user-typed slot n maps to index n-1 (R4, R56).
// UNKNOWN: empty slot means Item.id == 0.
// FIXED: the enumerators EquipOutcome::{Refused, Equipped, Unequipped},
// DropOutcome::{Refused, Dropped}, UseOutcome::{Refused, NoCharges} and
// SwapResult::{Refused, Swapped} are not in the API INDEX and do not exist in the
// headers. They are replaced by the placeholder constants defined below. The ordinal
// values (0, 1, 2) are GUESSES and must be reconciled with the real enumerator lists
// in items1a.h and items1b.h before this code is used.

#include "mordor/items1a.h"
#include "mordor/items1b.h"
#include "mordor/items2b.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// FIXED: placeholder enumerator values. UNKNOWN ordinals; reconcile with items1a.h / items1b.h.
constexpr EquipOutcome kEquipRefused = static_cast<EquipOutcome>(0);
constexpr EquipOutcome kEquipUnequipped = static_cast<EquipOutcome>(1);
constexpr EquipOutcome kEquipEquipped = static_cast<EquipOutcome>(2);
constexpr DropOutcome kDropRefused = static_cast<DropOutcome>(0);
constexpr DropOutcome kDropDropped = static_cast<DropOutcome>(1);
constexpr UseOutcome kUseRefused = static_cast<UseOutcome>(0);
constexpr UseOutcome kUseNoCharges = static_cast<UseOutcome>(1);
constexpr SwapResult kSwapRefused = static_cast<SwapResult>(0);
constexpr SwapResult kSwapSwapped = static_cast<SwapResult>(1);

bool scanNumber(const Str& s, i32 from, i32 to, double& out) {
    bool neg = false;
    bool any = false;
    double v = 0.0;
    i32 i = from;
    while (i < to && s[i] == ' ') ++i;
    if (i < to && (s[i] == '-' || s[i] == '+')) {
        neg = (s[i] == '-');
        ++i;
    }
    while (i < to && s[i] >= '0' && s[i] <= '9') {
        v = v * 10.0 + static_cast<double>(s[i] - '0');
        ++i;
        any = true;
    }
    if (i < to && s[i] == '.') {
        ++i;
        double scale = 0.1;
        while (i < to && s[i] >= '0' && s[i] <= '9') {
            v += static_cast<double>(s[i] - '0') * scale;
            scale *= 0.1;
            ++i;
            any = true;
        }
    }
    out = neg ? -v : v;
    return any;
}

// Typed slot number -> pack index. Typed 1 (Hands) -> 0. Result clamped to -1..1000.
i32 slotIndexOf(double typed) {
    i64 v = vbInt(typed) - 1;
    if (v < -1) v = -1;
    if (v > 1000) v = 1000;
    return static_cast<i32>(v);
}

bool validStorage(i32 index) {
    return index >= 1 && index <= kLastPackIndex;
}

i32 freeStorageSlot(const Inventory& inv) {
    for (i32 i = 1; i <= kLastPackIndex; ++i) {
        if (isEmptySlot(inv.pack[i])) return i;
    }
    return -1;
}

Str i64Str(i64 v) {
    if (v == 0) return Str("0");
    bool neg = v < 0;
    u64 m = neg ? static_cast<u64>(-(v + 1)) + 1u : static_cast<u64>(v);
    Str out;
    while (m > 0) {
        out.insert(out.begin(), static_cast<char>('0' + static_cast<i32>(m % 10)));
        m /= 10;
    }
    if (neg) out.insert(out.begin(), '-');
    return out;
}

}  // namespace

// ---- R3: gold answer. Non-numeric or sub-1 gives 0; otherwise CLng (half to even).
i64 parseGiveGoldAnswer(const Str& answer) {
    double d = 0.0;
    if (!scanNumber(answer, 0, static_cast<i32>(answer.size()), d)) return 0;
    if (d < 1.0) return 0;
    return roundHalfEven(d);
}

// ---- R4: item answer read as a typed slot number; returns pack index (typed-1).
i32 slotFromAnswer(const Str& answer) {
    double d = 0.0;
    if (!scanNumber(answer, 0, static_cast<i32>(answer.size()), d)) return -1;
    return slotIndexOf(d);
}

// ---- R1-R3: gold give.
GiveResult giveGold(Ctx& ctx, Character& giver, Character& receiver,
                    i32 giverIndex, i32 receiverIndex, i64 wholeGold, bool inCombat) {
    if (inCombat) {  // R1
        ctx.say("items.giveCombat", "You cannot give items or gold during combat!", {}, {});
        return GiveResult::InCombat;
    }
    if (giverIndex == receiverIndex) return GiveResult::SameCharacter;  // R2
    i64 gold = wholeGold < 0 ? 0 : wholeGold;
    Currency amount = currencyFromWhole(gold);
    if (amount > giver.wallet.goldOnHand) {  // R3
        ctx.say("items.giveNotEnough", "Not enough gold!", {}, {});
        return GiveResult::NotEnoughGold;
    }
    giver.wallet.goldOnHand -= amount;
    receiver.wallet.goldOnHand += amount;
    ctx.say("items.giveGold",
            Str("Give of ") + i64Str(gold) + " gold to \"" + receiver.name + "\" Completed!",
            {gold}, {receiver.name});  // R3
    return GiveResult::Done;
}

// ---- R1-R10: pack-to-pack item give.
GiveResult giveItem(Ctx& ctx, Character& giver, Inventory& giverInv, i32 giverIndex,
                    Character& receiver, Inventory& receiverInv, i32 receiverIndex,
                    i32 slot, bool inCombat) {
    if (inCombat) {  // R1
        ctx.say("items.giveCombat", "You cannot give items or gold during combat!", {}, {});
        return GiveResult::InCombat;
    }
    if (giverIndex == receiverIndex) return GiveResult::SameCharacter;  // R2
    if (slot < 0 || slot > kLastPackIndex) return GiveResult::SlotOutOfRange;  // R4
    Item& item = giverInv.pack[slot];
    if (slot == 0 || isEmptySlot(item)) {  // R4
        ctx.say("items.giveNone", "No Item to give!", {}, {});
        return GiveResult::NoItem;
    }
    if (item.equipped) {  // R10 (pack path text)
        ctx.say("items.giveEquipped", "Cannot give equipped items!", {}, {});
        return GiveResult::Equipped;
    }
    if (item.guildIssued || item.isGuildItem()) {  // R5
        ctx.say("items.giveGuild", "Cannot give Guild-Issued Items!", {}, {});
        return GiveResult::GuildIssued;
    }
    i32 free = freeStorageSlot(receiverInv);
    if (free < 0) {  // R8
        ctx.say("items.givePackFull", "SORRY! Pack is full.", {}, {});
        return GiveResult::ReceiverPackFull;
    }
    // CONFLICT: R7 says the re-read uses the wrong item on the original path. The
    // given item is re-read here (UNKNOWN whether the give path shares the bug).
    Item moved = item;
    giverInv.pack[slot] = Item{};
    moved.idLevel = rereadKnowledge(ctx, receiver, moved);  // R7
    // R7: Library filing needs library state, which this module does not receive.
    receiverInv.pack[free] = moved;
    ctx.say("items.give", Str("Give of Item to \"") + receiver.name + "\" Completed!",
            {}, {receiver.name});  // R6
    return GiveResult::Done;
}

bool hasFreePackSlot(const Inventory& inv) {
    return freeStorageSlot(inv) >= 0;
}

bool isEmptySlot(const Item& item) {
    return item.id == 0;  // UNKNOWN: empty encoding
}

// ---- R7: re-read of knowledge. UNKNOWN: the knowledge formula is not in the dossier;
// the current level is kept.
IdLevel rereadKnowledge(const Ctx& ctx, const Character& receiver, const Item& item) {
    (void)ctx;
    (void)receiver;
    return item.idLevel;
}

// ---- R7: Library grade. All -> 4, Mostly -> 3, otherwise 0.
i32 libraryGradeFor(IdLevel known) {
    if (known == IdLevel::All) return kLibraryGradeAll;
    if (known == IdLevel::Mostly) return kLibraryGradeMostly;
    return kLibraryGradeNone;
}

// ---- R7: grade rises only.
i32 libraryGradeAfterRead(i32 currentGrade, IdLevel knownNow) {
    i32 g = libraryGradeFor(knownNow);
    return g > currentGrade ? g : currentGrade;
}

// ---- R12: dropping a dragged slot on a buffer empties the buffer.
DragOutcome dropOnBuffer(Ctx& ctx, Character& c, i32 bufferIndex) {
    (void)ctx;
    if (bufferIndex >= 0 && bufferIndex < kSpellBuffers) {
        c.order.spellBuffers[bufferIndex] = 0;
    }
    return DragOutcome::BufferEmptied;
}

// ---- R14, R16, R17, R55: dropping a dragged slot on the vault.
DragOutcome dropOnVault(Ctx& ctx, Inventory& inv, i32 slot) {
    if (slot == 0) return DragOutcome::HandsRefused;  // R16 Hands never go into vault
    if (slot < 0 || slot > kLastPackIndex) return DragOutcome::HandsRefused;  // UNKNOWN
    Item& item = inv.pack[slot];
    if (isEmptySlot(item)) return DragOutcome::HandsRefused;  // UNKNOWN: empty source
    if (item.guildIssued || item.isGuildItem()) return DragOutcome::GuildRefused;  // R17
    if (!canBankItem(item)) return DragOutcome::EquippedRefused;  // R55
    i32 free = -1;
    for (i32 i = 0; i < kBankSlots; ++i) {
        if (isEmptySlot(inv.bank[i])) {
            free = i;
            break;
        }
    }
    if (free < 0) {  // R14
        ctx.say("items.vaultFull", "Out of room!", {}, {});
        return DragOutcome::VaultFull;
    }
    inv.bank[free] = item;
    inv.pack[slot] = Item{};
    return DragOutcome::VaultStored;
}


// ---- R18, R26: paralysed characters cannot equip or drop.
bool paralysedBlocksPackAction(const Character& c) {
    return c.ailments.paralysed;
}

// UNKNOWN: wearable type set is not in the dossier. Assumed: Other.
bool isWearableType(ItemType type) {
    return type == ItemType::Other;
}

// ---- R18-R24: toggle equip.
// UNKNOWN: WearRefusal (R20) is not in the dossier; no wear refusal is applied here.
EquipOutcome toggleEquip(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    if (paralysedBlocksPackAction(c)) return kEquipRefused;  // R18 silent
    if (!validStorage(slot)) return kEquipRefused;
    Item& it = inv.pack[slot];
    if (isEmptySlot(it) || !isWearableType(it.type)) return kEquipRefused;  // R19 silent
    if (it.equipped) {
        if (it.slotCursed) {  // R21
            ctx.say("items.unequipCursed", "Cannot unequip!  Item is cursed!", {}, {});
            return kEquipRefused;
        }
        it.equipped = false;
        it.slotCursed = false;  // R22: unequip clears the slot curse
        return kEquipUnequipped;
    }
    it.equipped = true;  // R22
    applyEquipCurse(it);
    return kEquipEquipped;  // R23: no message
    // R24: selection advance is a UI concern, not modelled here.
}

// ---- R22: equip curse sets the slot curse flag.
// UNKNOWN: which curse values count is not in the dossier; any non-zero value is used.
void applyEquipCurse(Ctx& ctx, Item& item, Align wearer) {
    (void)ctx;
    // R11: sets the slot curse flag if the item's Cursed field is nonzero...
    if (item.cursed != CurseNone) {
        item.slotCursed = true;
        return;
    }
    // R19: unaligned items, or items without the aligned flag, are Fine.
    if (!item.aligned || item.alignment == Unaligned) {
        return;
    }
    // ...or if the alignment fit is Opposed (two steps: Good<->Evil).
    // ItemAlign Good=1, Neutral=2, Evil=3 -> 0, 1, 2 to match Align.
    const i32 itemIdx = static_cast<i32>(item.alignment) - 1;
    const i32 wearerIdx = static_cast<i32>(wearer);
    const i32 steps = (itemIdx > wearerIdx) ? (itemIdx - wearerIdx) : (wearerIdx - itemIdx);
    if (steps == 2) {
        item.slotCursed = true;  // Opposed
    }
}

// ---- R26-R32: drop. Port order per R30 (kPortDropConfirmsFirst): confirm, then curse.
DropOutcome dropPackItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot, bool confirmed) {
    if (paralysedBlocksPackAction(c)) return kDropRefused;  // R26
    if (!validStorage(slot)) return kDropRefused;
    Item& it = inv.pack[slot];
    if (isEmptySlot(it)) return kDropRefused;  // UNKNOWN
    if (it.guildIssued || it.isGuildItem()) return kDropRefused;  // R27 (text UNKNOWN)
    if (!confirmed) {  // R29, R30
        ctx.say("items.dropConfirm", "Drop the item?", {}, {});
        return DropOutcome::NeedsConfirm;
    }
    if (it.equipped && it.slotCursed) {  // R28 caption
        ctx.say("items.dropCursed",
                Str("You can't drop item ") + i64Str(slot) + "!  It's cursed!", {slot}, {});
        return kDropRefused;
    }
    inv.pack[slot] = Item{};  // R31: destroyed, nothing left on floor
    return kDropDropped;
}

// ---- R33: silent refusal while paralysed; buffered items skip this test.
bool paralysedBlocksUse(const Character& c, bool buffered) {
    return !buffered && c.ailments.paralysed;
}

// ---- R33-R39: use an item from the pack.
// UNKNOWN: ChangesStats / SpellListChecked (R37) are not in the dossier.
UseOutcome useItemFromPack(Ctx& ctx, Character& user, Inventory& inv, i32 slot, bool buffered) {
    if (paralysedBlocksUse(user, buffered)) return kUseRefused;  // R33
    if (!validStorage(slot)) return kUseRefused;
    Item& it = inv.pack[slot];
    if (isEmptySlot(it)) return kUseRefused;  // R34 guards
    if (it.spellId != 0) {  // R36
        if (!castItemSpell(ctx, user, it)) return kUseRefused;
        spendCharge(ctx, it);  // R36: charge taken only when the cast goes through
    } else {
        if (!spendCharge(ctx, it)) return kUseNoCharges;  // R37
        applyItemSpecial(ctx, user, inv, slot);  // R37
    }
    if (it.maxCharges > 0 && it.charges <= 0) {  // R39: UNKNOWN timing (CONFLICT)
        consumeUsedItem(inv, slot);
    }
    return UseOutcome::Used;
}

// UNKNOWN: spell resolution lives in the casting modules. Only the gate is modelled.
bool castItemSpell(Ctx& ctx, Character& user, Item& item) {
    if (item.spellId == 0) return false;
    ctx.say("items.castSpell", "The item casts its spell.", {item.spellId}, {user.name});
    return true;
}

// R37, R42: consumes one charge. False when no charges remain.
bool spendCharge(Ctx& ctx, Item& item) {
    (void)ctx;
    if (item.charges <= 0) return false;
    item.charges -= 1;
    return true;
}

// UNKNOWN: the special-effect table is not in the dossier. No effect is applied.
void applyItemSpecial(Ctx& ctx, Character& user, Inventory& inv, i32 slot) {
    (void)ctx;
    (void)user;
    (void)inv;
    (void)slot;
}

// ---- R38: stat item. Order: alignment test, then raise or lower.
// UNKNOWN: stat index (spellIndex) and amount (level) mapping is not in the dossier.
StatItemOutcome applyStatItem(Ctx& ctx, Character& user, Item& item, bool reversed) {
    if (!statItemAlignmentPasses(user, item)) {  // R38.2 (text UNKNOWN)
        ctx.say("items.statAlign", "Alignment refused.", {}, {});
        return StatItemOutcome::AlignmentRefused;
    }
    i32 s = item.spellIndex;
    if (s < 0) s = 0;
    if (s >= kStatCount) s = kStatCount - 1;
    i32 amount = item.level;
    if (reversed) {  // R38.4: positive modifiers lowered; no Hall check
        if (user.abilities.statMods[s] > 0) {
            user.abilities.statMods[s] -= amount;
            if (user.abilities.statMods[s] < 0) user.abilities.statMods[s] = 0;
        }
        return StatItemOutcome::Lowered;
    }
    // R38.3: raise path with clamp. Hall of Records check (line 6219) is not modelled.
    i32 v = user.abilities.stats[s] + amount;
    if (v > user.abilities.statMax[s]) v = user.abilities.statMax[s];
    user.abilities.stats[s] = v;
    return StatItemOutcome::Raised;
}

// CONFLICT: alignment encoding. ItemAlign-1 == Align per [c26]; Unaligned passes.
bool statItemAlignmentPasses(const Character& user, const Item& item) {
    if (item.alignment == ItemAlign::Unaligned) return true;
    return static_cast<i32>(item.alignment) - 1 == static_cast<i32>(user.alignment);
}

// R39: item used up.
void consumeUsedItem(Inventory& inv, i32 slot) {
    if (slot < 0 || slot > kLastPackIndex) return;
    inv.pack[slot] = Item{};
}

// ---- R41: an item used in a fight becomes the item order for this member.
// UNKNOWN: the combat-order field to set is not given; only itemOrders is set.
void setItemOrder(Party& party, i32 memberIndex, i32 slot) {
    if (memberIndex < 0 || memberIndex >= kPartyMaxSize) return;
    party.itemOrders[memberIndex].slot = slot;
    party.itemOrders[memberIndex].item = -1;
    party.itemOrders[memberIndex].cast = false;
}

// ---- R41, R42: one round of an item order.
ItemOrderStep itemOrderRound(Ctx& ctx, Party& party, i32 memberIndex, Inventory& inv) {
    if (memberIndex < 0 || memberIndex >= kPartyMaxSize) return ItemOrderStep::NoOrder;
    ItemOrder& order = party.itemOrders[memberIndex];
    if (!validStorage(order.slot)) return ItemOrderStep::NoOrder;
    Item& it = inv.pack[order.slot];
    if (!spendCharge(ctx, it)) {  // R42
        ctx.say("items.outOfCharges", "Your item has run out of charges!", {}, {});
        order.slot = -1;
        party.combatOrders[memberIndex].kind = CombatOrderKind::Fight;
        return ItemOrderStep::RanOutOfCharges;
    }
    return ItemOrderStep::Continues;  // R41, R43: spell orders keep draining the item
}

// R44: any non-zero curse value sets the slot curse flag on chest drop.
void applyChestDropCurse(Item& item) {
    item.slotCursed = (item.cursed != CurseKind::CurseNone);
}

// ---- R44-R48: chest item attaches when Cursed == 2.
// UNKNOWN: WearRefusal (R45) is not in the dossier; no refusal is applied here.
ChestAttach chestItemAttach(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    if (!validStorage(slot)) return ChestAttach::NoCurse;
    Item& it = inv.pack[slot];
    applyChestDropCurse(it);  // R44
    if (it.cursed != CurseKind::CurseKind2) return ChestAttach::NoCurse;  // R45
    if (!isWearableType(it.type)) return ChestAttach::NotWearable;  // R48 silent
    it.equipped = true;  // R45
    applyEquipCurse(it);
    ctx.say("items.attach", "An item you found has attached itself to you!!", {}, {c.name});  // R46
    return ChestAttach::Attached;
}


// ---- R50: pack re-read on level gain. Items 1..39 (user slots 2..40).
i32 rereadPackOnLevelGain(Ctx& ctx, const Character& c, Inventory& inv) {
    i32 rose = 0;
    for (i32 i = 1; i <= kLastPackIndex; ++i) {
        Item& it = inv.pack[i];
        if (isEmptySlot(it)) continue;
        IdLevel before = it.idLevel;
        IdLevel now = rereadKnowledge(ctx, c, it);
        if (static_cast<i32>(now) > static_cast<i32>(before)) {  // R50: Known rose
            it.idLevel = now;
            ++rose;
            ctx.say("items.reread", Str("Item #") + i64Str(i + 1), {i + 1}, {});
            // R50: Library filing needs library state, which this module does not receive.
        }
    }
    return rose;
}

// ---- R53, R54: withdraw from bank into the first free pack slot.
WithdrawOutcome withdrawFromBank(Ctx& ctx, const Character& c, Inventory& inv, i32 bankIndex) {
    if (bankIndex < 0 || bankIndex >= kBankSlots) return WithdrawOutcome::PackFull;  // UNKNOWN
    i32 free = freeStorageSlot(inv);
    if (free < 0) {  // R53
        ctx.say("items.bankFull", "SORRY! Pack is full.", {}, {});
        return WithdrawOutcome::PackFull;
    }
    Item it = inv.bank[bankIndex];
    IdLevel now = rereadKnowledge(ctx, c, it);  // R54
    if (static_cast<i32>(now) > static_cast<i32>(it.idLevel)) {
        it.idLevel = now;
        ctx.say("items.bankIdentified", "Item identified.", {}, {});  // R54: logged
    }
    inv.pack[free] = it;
    inv.bank[bankIndex] = Item{};
    return WithdrawOutcome::Withdrawn;
}

// ---- R56: swap request parsing. Input is "a,b".
SwapRequest parseSwapRequest(const Str& input) {
    SwapRequest r;
    i32 n = static_cast<i32>(input.size());
    i32 comma = -1;
    for (i32 i = 0; i < n && comma < 0; ++i) {
        if (input[i] == ',') comma = i;
    }
    if (comma < 0) {  // R56
        r.status = SwapParse::NoComma;
        return r;
    }
    if (n > kSwapInputMaxChars) {  // R56
        r.status = SwapParse::Refused;
        return r;
    }
    double a = 0.0;
    double b = 0.0;
    bool okA = scanNumber(input, 0, comma, a);
    bool okB = scanNumber(input, comma + 1, n, b);
    r.first = okA ? slotIndexOf(a) : -1;
    r.second = okB ? slotIndexOf(b) : -1;
    bool bad = !okA || !okB || !validStorage(r.first) || !validStorage(r.second);  // R56
    r.status = bad ? SwapParse::Refused : SwapParse::Swapped;
    return r;
}

// ---- R57, R58: swap two pack slots. Empty slots may be swapped.
// CONFLICT: buffer remap on swap. Buffers follow only when buffersFollow is set (R58 passes false).
SwapResult swapSlots(Ctx& ctx, Character& c, Inventory& inv, i32 a, i32 b, bool buffersFollow) {
    (void)ctx;
    if (!validStorage(a) || !validStorage(b)) return kSwapRefused;  // R56
    if (a == b) return kSwapSwapped;
    Item tmp = inv.pack[a];
    inv.pack[a] = inv.pack[b];
    inv.pack[b] = tmp;
    if (buffersFollow) {
        for (i32 i = 0; i < kSpellBuffers; ++i) {
            if (c.order.spellBuffers[i] == a) {
                c.order.spellBuffers[i] = b;
            } else if (c.order.spellBuffers[i] == b) {
                c.order.spellBuffers[i] = a;
            }
        }
    }
    // R58: saving the character after a dungeon swap is the caller's duty.
    return kSwapSwapped;
}

}  // namespace mordor
