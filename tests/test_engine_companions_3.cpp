#include "test_support.h"
#include "mordor/state_character.h"
#include "mordor/companions1a.h"
#include "mordor/companions1b.h"

using namespace mordor;

// SKIP B_0057#40: Librarian / FileCompanion has no equivalent in the new headers.
// SKIP B_0057#41: Librarian / FileCompanion has no equivalent in the new headers.
// SKIP B_0059#68: save/load of companion slots to JSON has no equivalent in the new headers.
// SKIP B_0059#69: save/load of companion slots to JSON has no equivalent in the new headers.
// SKIP B_0059#70: save/load of companion slots to JSON has no equivalent in the new headers.
// SKIP B_0059#71: JSON serialization of companion slots has no equivalent in the new headers.
// SKIP B_0059#72: vacancy after load depends on save/load, which has no equivalent in the new headers.
// SKIP B_0059#74: reading a legacy null slot on load has no equivalent in the new headers.

TEST(engine_B_0055_52_join_ninja_ref_and_id) {
    tsup::Env env({0.5, 0.9});

    Character owner{};
    owner.alignment = Align::Evil;

    // Setup fix: a join offer needs at least one living monster in the group.
    // acceptJoinOffer loops over offerCount and stops when count <= 0, and
    // refuses a front with hits <= 0 (R12.2 / R37). The original setup left
    // count, offerCount and front.hits at zero, so no join could happen.
    Creature front{};
    front.id = 307;
    front.ref = 389;
    front.align = CharAlign::Evil;
    front.hits = 1;

    MonsterGroup group{};
    group.front = front;
    group.joinOffer = true;
    group.count = 1;
    group.offerCount = 1;

    CompanionSlots slots{};
    acceptJoinOffer(env.ctx, owner, 0, slots, group);

    i32 found = 0;
    for (i32 i = 0; i < kCompanionSlotsPerCharacter; ++i) {
        if (slots[i].occupied) found = i;
    }
    CHECK(slots[found].occupied);
    CHECK_EQ(slots[found].ref, 389);
    CHECK_EQ(slots[found].monsterId, 307);
}

TEST(engine_B_0059_73_new_companion_takes_slot1_keeps_name) {
    tsup::Env env({0.5, 0.9});

    CompanionSlots slots{};
    slots[0].occupied = true;
    slots[0].name = Str("Rex");
    slots[1].occupied = false;
    slots[1].name = Str("Fluffy");

    // keepSlotName(true) is applied by the placement path; the new API takes
    // only the incoming companion, so the kept name is checked on slot 1.
    Companion incoming{};
    incoming.name = Str("");
    incoming.hits = 3;
    incoming.monsterId = 8;

    i32 slot = placeNewCompanion(env.ctx, slots, incoming);
    CHECK_EQ(slot, 1);
    CHECK_EQ(slots[1].name, Str("Fluffy"));
}
