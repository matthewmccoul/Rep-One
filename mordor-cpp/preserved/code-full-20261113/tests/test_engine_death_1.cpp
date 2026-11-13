#include "test_support.h"
#include "mordor/death1b.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"

using namespace mordor;

// SKIP B_0055#110: checkWealthiestRecord returns void; offerHallRecord returns bool but needs a category id that is not declared in the headers.

TEST(engine_B_0055_111_wealthiest_value) {
    tsup::Env env({0.5, 0.9});
    HallOfRecords hall;
    hall.wealthiest = 10000000;          // Wealthiest record of 1000 gold
    Character ch;
    ch.wallet.goldInBank = 10005000;     // 1000.5 gold banked
    checkWealthiestRecord(env.ctx, hall, ch, ch.wallet.goldInBank, 0);
    CHECK_EQ(hall.wealthiest, 10005000); // 1000.5 gold
}

// SKIP B_0055#112: checkWealthiestRecord returns void, so there is no null return to assert.

// SKIP B_0056#35: CombatEngine.MonsterAttacks and the random draw count are not in the headers.

// SKIP B_0056#36: CombatEngine.MonsterAttacks and the spoken-line output are not in the headers.

// SKIP B_0057#24: Librarian store (KnowsMonster, FileSighting, ForMonster) is not in the headers.

// SKIP B_0057#36: Librarian store (FileSighting, ForMonster) is not in the headers.

// SKIP B_0057#37: Librarian store (FileSighting) is not in the headers.

// SKIP B_0057#38: Librarian store (ForMonster) is not in the headers.

// SKIP B_0057#39: Librarian store (ForMonster) is not in the headers.
