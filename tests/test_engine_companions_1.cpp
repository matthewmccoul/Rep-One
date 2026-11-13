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

// SKIP B_0055#17: message text constant not exposed in headers; sayDragSwapRefused emits it
//   but EventLog inspection API is not in the headers.
// SKIP B_0055#19: Character.Create does not exist in headers; Morloch AlignmentValue not available.
// SKIP B_0055#20: no CombatMonster type in headers; Morloch definition alignment value not available.
// SKIP B_0055#21: no CombatMonster type in headers; Morloch definition alignment value not available.
// SKIP B_0055#27: CompanionSlots.Settle has no equivalent; CompanionAlignment has no Unaligned value.

