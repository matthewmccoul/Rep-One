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

}  // namespace

// ---------------------------------------------------------------------------
// Chest class and lock
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Trap bits
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Chest contents
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Trap effects
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Trap identify and disarm
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Items, thieves, purse and bank
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Item alignment, shelves, identify level
// ---------------------------------------------------------------------------

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
