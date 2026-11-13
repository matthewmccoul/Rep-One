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
