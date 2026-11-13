#include "test_support.h"

#include "mordor/state_common.h"

// SKIP B_0063#95: theft slot draws over 120000 iterations need a simulation loop over monsterSteal; no header function returns a theft slot index
// SKIP B_0063#96: theft slot draws over 120000 iterations need a simulation loop over monsterSteal; no header function returns a theft slot index
// SKIP B_0063#97: distinct theft outcomes need a 120000-draw simulation; no header function exposes theft slot outcomes
// SKIP B_0063#98: bucket-balance check needs a 120000-draw theft simulation; no header function exposes theft slot outcomes

TEST(engine_B_0063_99_pack_size) {
    // Character.PackSize maps to the pack slot count constant (Inventory.pack is Arr<Item, kPackSlots>).
    CHECK_EQ(mordor::kPackSlots, 40);
}

// SKIP B_0065#34: needs a 300-seed simulation comparing monsterMeleeBlow outcomes with and without a chest count
// SKIP B_0065#35: needs a 300-seed simulation comparing monsterMeleeBlow outcomes with and without a chest count

// SKIP B_0066#74: EncounterGenerator.Generate has no equivalent header function; needs a 3000-seed simulation
// SKIP B_0066#75: EncounterGenerator.Generate has no equivalent header function; needs a 3000-seed simulation
// SKIP B_0066#76: EncounterGenerator.Generate has no equivalent header function; needs a 2000-seed simulation
