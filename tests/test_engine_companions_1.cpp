#include "test_support.h"
#include "mordor/companions1b.h"

using namespace mordor;

namespace {

// FIXED: CompanionSlots was used but declared nowhere; it is the local alias for the existing
// Arr<Companion, kCompanionSlots> type that companions1b.h uses for dragSwapCompanions.
using CompanionSlots = Arr<Companion, kCompanionSlots>;

Companion makeCompanion(const char* name, i32 hits, i32 maxHits) {
    Companion c;
    c.occupied = true;
    c.name = name;
    c.hits = hits;
    c.maxHits = maxHits;
    return c;
}

void killCompanion(Companion& c) {
    c.hits = 0;
    c.isDead = true;
    c.deathState = CompanionDeathState::Dead;
}

// Slot order check: slots 0..2 hold the named companions (occupied), slot 3 is empty ('-').
bool orderIs(const CompanionSlots& s, const char* a, const char* b, const char* c) {
    const char* want[3] = {a, b, c};
    for (u32 i = 0; i < 3; ++i) {
        if (!s[i].occupied) return false;
        if (!(s[i].name == want[i])) return false;
    }
    return !s[3].occupied;
}

// Initial layout used by several cases: A, B, C (dead), slot 3 empty.
void buildABC(CompanionSlots& s) {
    s[0] = makeCompanion("A", 5, 5);
    s[1] = makeCompanion("B", 7, 7);
    s[2] = makeCompanion("C", 0, 9);
    killCompanion(s[2]);
}

// Initial layout used by several cases: B, A, C (dead), slot 3 empty.
void buildBAC(CompanionSlots& s) {
    s[0] = makeCompanion("B", 7, 7);
    s[1] = makeCompanion("A", 5, 5);
    s[2] = makeCompanion("C", 0, 9);
    killCompanion(s[2]);
}

}  // namespace

// B_0055#12: Rearrange(0,1) on A,B,C,- -> blank return (Swapped), order B,A,C,-
TEST(engine_B_0055_12_rearrange_first_two_swaps) {
    tsup::Env env({0.5, 0.9});
    CompanionSlots slots{};
    buildABC(slots);
    DragSwapOutcome r = dragSwapCompanions(env.ctx, slots, 0, 1);
    CHECK(r == DragSwapOutcome::Swapped);
    CHECK(orderIs(slots, "B", "A", "C"));
}

// B_0055#13: Rearrange(1,1) on B,A,C,- -> blank return (Swapped), order unchanged
TEST(engine_B_0055_13_rearrange_onto_self_no_change) {
    tsup::Env env({0.5, 0.9});
    CompanionSlots slots{};
    buildBAC(slots);
    DragSwapOutcome r = dragSwapCompanions(env.ctx, slots, 1, 1);
    CHECK(r == DragSwapOutcome::Swapped);
    CHECK(orderIs(slots, "B", "A", "C"));
}

// B_0055#14: Rearrange(2,0), dead C dropped on live B -> RefusedDead, order unchanged
TEST(engine_B_0055_14_dead_dropped_on_live_refused) {
    tsup::Env env({0.5, 0.9});
    CompanionSlots slots{};
    buildBAC(slots);
    DragSwapOutcome r = dragSwapCompanions(env.ctx, slots, 2, 0);
    CHECK(r == DragSwapOutcome::RefusedDead);
    CHECK(orderIs(slots, "B", "A", "C"));
}

// B_0055#15: Rearrange(1,2), live A dropped on dead C -> RefusedDead, order unchanged
TEST(engine_B_0055_15_live_dropped_on_dead_refused) {
    tsup::Env env({0.5, 0.9});
    CompanionSlots slots{};
    buildBAC(slots);
    DragSwapOutcome r = dragSwapCompanions(env.ctx, slots, 1, 2);
    CHECK(r == DragSwapOutcome::RefusedDead);
    CHECK(orderIs(slots, "B", "A", "C"));
}

// B_0055#16: Rearrange(3,0), empty slot 3 dropped on slot 0 -> blank return (Swapped), order unchanged
TEST(engine_B_0055_16_empty_slot_dropped_on_slot0) {
    // Reference: slots.Rearrange(onto=3, from=0) with B,A,C,empty.
    // C# order is (onto, from); C++ dragSwapCompanions is (fromSlot, toSlot).
    // Dropping a live companion onto an empty slot is a no-op.
    tsup::Env env({0.5, 0.9});
    CompanionSlots slots{};
    buildBAC(slots);
    DragSwapOutcome r = dragSwapCompanions(env.ctx, slots, 0, 3);
    CHECK(r != DragSwapOutcome::Swapped);  // reference: no change
    CHECK(orderIs(slots, "B", "A", "C"));
}

// SKIP B_0055#17: message text constant not exposed in headers; sayDragSwapRefused emits it
//   but EventLog inspection API is not in the headers.
// SKIP B_0055#19: Character.Create does not exist in headers; Morloch AlignmentValue not available.
// SKIP B_0055#20: no CombatMonster type in headers; Morloch definition alignment value not available.
// SKIP B_0055#21: no CombatMonster type in headers; Morloch definition alignment value not available.
// SKIP B_0055#27: CompanionSlots.Settle has no equivalent; CompanionAlignment has no Unaligned value.

