// items1a_missing.cpp
// Implements: giveFromPack, giveFromVault, itemDisplayName (items1a.h).
// ASSUMED: GameData field names used here (items[].name, itemTypes[].name,
// itemDescriptions[].name) are not given in the foundation summary; they follow
// the data-file naming and must be checked against game_data.h.
#include <string>

#include "mordor/items1a.h"

namespace mordor {

// R47, R48, R53: give from pack.
// File-local helper, inside namespace mordor. Mirrors C# TrimEnd() as used by
// Party.GaveItem (Party.cs:502) and Party.NoFreeSlots (Party.cs:467).
namespace {
Str trimEndName(const Str& s) {
    auto end = s.size();
    while (end > 0 && s[end - 1] == ' ') {
        --end;
    }
    return s.substr(0, end);
}
}  // namespace

GiveOutcome giveFromPack(Ctx& ctx, const Character& giver, Inventory& giverInv,
                         Character& receiver, Inventory& receiverInv, i32 slot) {
    // R2: hands (slot 0) cannot be given; R1 item slots are 1..39.
    // ASSUMED: an out-of-range or empty slot is a silent no-op. GiveOutcome has no
    // "nothing" value, so Given is returned without any transfer. Callers must
    // check slotIsEmpty() first.
    if (slot < kFirstItemSlot || slot > kLastItemSlot) {
        return GiveOutcome::Given;
    }
    Item& item = giverInv.pack[static_cast<u32>(slot)];
    if (slotIsEmpty(item)) {
        return GiveOutcome::Given;  // ASSUMED: see above
    }

    // R47 (1): guild-issued items are refused first.
    // ASSUMED: guildIssued flag and the guild marker in ailments both identify guild-issued items.
    if (item.guildIssued || item.isGuildItem()) {
        ctx.say("items.give.guildIssued", "Cannot give Guild-Issued Items!",
                {slot}, {giver.name});
        return GiveOutcome::GuildIssued;
    }

    // R47 (2): equipped items are refused next.
    if (item.equipped) {
        ctx.say("items.give.equipped", "Cannot give equipped items!",
                {slot}, {giver.name});
        return GiveOutcome::Equipped;
    }

    // R47 (3): receiver must have a free slot. Scan starts at 1 (R3).
    // ASSUMED: firstFreePackSlot returns a value below kFirstItemSlot when no slot is free.
    const i32 freeSlot = firstFreePackSlot(receiverInv);
    if (freeSlot < kFirstItemSlot || freeSlot > kLastItemSlot) {
        // Name is quoted and TrimEnd'ed (Party.NoFreeSlots, Party.cs:467).
        ctx.say("items.give.receiverFull",
                Str("\"") + trimEndName(receiver.name) + "\" has no free slots!",
                {slot}, {receiver.name});
        return GiveOutcome::ReceiverFull;
    }

    // R48: the item moves, then the receiver reads it (Party.ReadGiven, R53).
    Item moved = item;
    item = Item{};  // empty slot
    // UNKNOWN: R76 buffer reset for the emptied giver slot is not applied here.
    // The giver is const in this signature, so resetEmptiedBuffers() cannot be called.
    receiverInv.pack[static_cast<u32>(freeSlot)] = moved;
    Item& placed = receiverInv.pack[static_cast<u32>(freeSlot)];
    giveRead(ctx, receiver, placed);

    // RESOLVED-BY-REFERENCE: the give line is
    // "Give of Item to \"<name>\" Completed!" (Party.GaveItem, Party.cs:502,
    // MAINMOD2.BAS:3004-3006), written after the read-again line.
    Str text = Str("Give of Item to \"") + trimEndName(receiver.name) + "\" Completed!";
    ctx.say("items.give.done", text, {slot, freeSlot}, {giver.name, receiver.name});
    return GiveOutcome::Given;
}

// R49, R50, R51: give from vault (bank).
VaultGiveOutcome giveFromVault(Ctx& ctx, Inventory& giverInv, Character& receiver,
                               Inventory& receiverInv, i32 bankSlot) {
    // ASSUMED: an out-of-range bank index is treated as an empty vault slot.
    if (bankSlot < 0 || bankSlot >= kBankSlots) {
        return VaultGiveOutcome::Empty;
    }
    Item& item = giverInv.bank[static_cast<u32>(bankSlot)];

    // R49: an empty vault slot returns "" (no message).
    if (slotIsEmpty(item)) {
        return VaultGiveOutcome::Empty;
    }

    // R50: a guild-issued item is refused. No equipped test on this branch.
    // ASSUMED: the refusal text matches the pack refusal.
    if (item.guildIssued || item.isGuildItem()) {
        ctx.say("items.vault.guildIssued", "Cannot give Guild-Issued Items!",
                {bankSlot}, {receiver.name});
        return VaultGiveOutcome::GuildIssued;
    }

    // R51: receiver must have a free pack slot. Message is unquoted.
    const i32 freeSlot = firstFreePackSlot(receiverInv);
    if (freeSlot < kFirstItemSlot || freeSlot > kLastItemSlot) {
        ctx.say("items.vault.receiverFull", receiver.name + " has no free slots!",
                {bankSlot}, {receiver.name});
        return VaultGiveOutcome::ReceiverFull;
    }

    // R51: on success the vault slot empties.
    Item moved = item;
    item = Item{};
    receiverInv.pack[static_cast<u32>(freeSlot)] = moved;
    Item& placed = receiverInv.pack[static_cast<u32>(freeSlot)];
    // ASSUMED: the vault give takes the same reading path as a pack give (R53).
    giveRead(ctx, receiver, placed);

    // RESOLVED-BY-REFERENCE: the vault give line is the same
    // "Give of Item to \"<name>\" Completed!" as the pack give (Party.GaveItem,
    // Party.cs:502: "from the pack or from the vault alike").
    Str text = Str("Give of Item to \"") + trimEndName(receiver.name) + "\" Completed!";
    ctx.say("items.vault.done", text, {bankSlot, freeSlot}, {receiver.name});
    return VaultGiveOutcome::Given;
}


}  // namespace mordor
