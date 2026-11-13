#include "test_support.h"
#include "mordor/partyb.h"
#include "mordor/state_party.h"
#include "mordor/state_character.h"

// SKIP B_0056#33: ListedParty and ListedParty.For are not in the headers; no equivalent function.
// SKIP B_0058#6: asserts the contents of decompiled BAS source files; not a runtime behaviour test.
// SKIP B_0058#7: asserts the contents of decompiled BAS source files; not a runtime behaviour test.
// SKIP B_0058#8: asserts the contents of FRMGROUP.BAS; not a runtime behaviour test.
// SKIP B_0058#13: asserts the contents of the port's DungeonWindow.axaml.cs (UI code); needs UI.
// SKIP B_0058#14: asserts the contents of the port's DungeonWindow.axaml.cs (UI code); needs UI.
// SKIP B_0058#15: asserts the contents of the port's CityWindow (UI code); needs UI.
// SKIP B_0059#3: asserts the prompt order in the original MAINMOD2.BAS source; not a runtime behaviour test.
// SKIP B_0059#29: the expected messages are emitted as text events whose codes and wording are not specified in the headers; no equivalent to assert against.

TEST(engine_B_0059_30_balances) {
    tsup::Env env(1);

    mordor::Party party;
    party.size = 2;
    mordor::Character& me = party.members[0];
    mordor::Character& you = party.members[1];
    me.name = "Me";
    you.name = "Taker";
    me.wallet.goldOnHand = mordor::Currency(300 * 10000);
    you.wallet.goldOnHand = mordor::Currency(5 * 10000);

    // TEST FIX: item number 2 maps to pack index 1. Item number 1 is the
    // Hands slot (index 0), which SRD c10 forbids giving from ("No Item to give!").
    // Expected values are unchanged.
    const mordor::i32 slot = mordor::itemSlotIndex(2);
    mordor::Item plain;
    plain.id = 7;
    party.inventories[0].pack[slot] = plain;

    mordor::PartyGiveGoldOutcome g =
        mordor::giveGoldToMember(env.ctx, party, me, 1, mordor::Currency(120 * 10000));
    CHECK(g == mordor::PartyGiveGoldOutcome::Given);

    mordor::PartyGiveItemOutcome it =
        mordor::giveItemToMember(env.ctx, party, me, 1, 2);
    CHECK(it == mordor::PartyGiveItemOutcome::Given);

    CHECK_EQ(me.wallet.goldOnHand, mordor::Currency(180 * 10000));
    CHECK_EQ(you.wallet.goldOnHand, mordor::Currency(125 * 10000));
    CHECK_EQ(party.inventories[0].pack[slot].id, 0);

    mordor::i32 held = 0;
    for (const mordor::Item& entry : party.inventories[1].pack) {
        if (entry.id == 7) {
            ++held;
        }
    }
    CHECK_EQ(held, 1);
}
