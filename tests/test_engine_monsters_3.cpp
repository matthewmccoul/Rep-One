#include "test_support.h"
#include "mordor/monsters1a.h"

// B_0057#34: duplicate-Id scan over MDATA5 (MdrLoader.LoadMonsters). No equivalent function in the headers.
// SKIP B_0057#34: no duplicate-Id scan or MDATA5 loader function in the headers

TEST(engine_B_0057_35_first_morgul_index) {
    tsup::Env env(1);
    // Expected: first monster with Id 307 sits at table index 278 (Morgul).
    CHECK_EQ(mordor::monsterTableIndexForId(env.ctx, 307), 278);
}

// B_0057#35 (last index 389 and trimmed names 'Morgul' / 'Shadow Ninja'): no last-match or name lookup in the headers.
// SKIP B_0057_35: last-index lookup and trimmed-name accessors are not in the headers

// B_0061#163: pen capacity for kind-0 monsters. The headers only expose kPenCapacity = 9 (pen overflow cap),
// which does not express the "40 or more" count being asserted.
// SKIP B_0061_163: no function counts kind-0 monsters the pens can hold; kPenCapacity is a different quantity

// B_0061#168-174: spawn level / item-drop level per monster come from the MDATA5 loader (LevelFound/ItemDropLevel).
// The headers have no loader or item-drop-level accessor; Creature::levelFound/spawnLevel are runtime state only.
// SKIP B_0061_168: MDATA5 LevelFound/ItemDropLevel for Aboleth not reachable from headers
// SKIP B_0061_169: MDATA5 LevelFound/ItemDropLevel for Pseudo-Dragon not reachable from headers
// SKIP B_0061_170: MDATA5 LevelFound/ItemDropLevel for Slave Masher not reachable from headers
// SKIP B_0061_171: MDATA5 LevelFound/ItemDropLevel for Tengu not reachable from headers
// SKIP B_0061_172: MDATA5 LevelFound/ItemDropLevel for Slave Driver not reachable from headers
// SKIP B_0061_173: MDATA5 LevelFound/ItemDropLevel for Cockatrice not reachable from headers
// SKIP B_0061_174: MDATA5 LevelFound/ItemDropLevel for Twisted Elf not reachable from headers
