#include "test_support.h"
#include "mordor/loot1b.h"
#include "mordor/attributes.h"
#include "mordor/items1b.h"
#include "mordor/items1a.h"  // FIXED: identifyChance(f64, bool) is declared here

using namespace mordor;

namespace {

Item& packAt(Inventory& inv, i32 i) { return inv.pack[static_cast<u32>(i)]; }
const Item& packAt(const Inventory& inv, i32 i) { return inv.pack[static_cast<u32>(i)]; }

void fillPack(Inventory& inv) {
    for (i32 i = 1; i < kPackSlots; ++i) packAt(inv, i).id = 1;
}

}  // namespace

// FitFor: unaligned or match -> Full; good vs evil -> Subtract; other -> Half.
TEST(fit_for_alignment_table) {
    CHECK(fitFor(ItemAlign::Unaligned, Align::Evil) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Good, Align::Good) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Evil, Align::Evil) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Good, Align::Evil) == FitClass::Subtract);
    CHECK(fitFor(ItemAlign::Evil, Align::Good) == FitClass::Subtract);
    CHECK(fitFor(ItemAlign::Good, Align::Neutral) == FitClass::Half);
    CHECK(fitFor(ItemAlign::Neutral, Align::Good) == FitClass::Half);
}

// transferGold moves min(max(amount,0), from.gold); negative moves nothing.


