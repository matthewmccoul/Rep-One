// items1a_a.cpp: gate, curse and alignment helpers for module items1a.
// Source: SRD c13p1 Part A, rules R1-R18 and Table T-2. The Formulas section
// (slot emptiness, alignment fit detail, asmodeus band, worth halving rounding)
// is not in this half of the SRD, so those points are marked UNKNOWN.
// Item-data field names (ItemDef / MDATA) are not in the supplied headers, so
// the per-item lookups are isolated in the helpers below with UNKNOWN fallbacks.
#include "mordor/items1a.h"
#include "mordor/guilds3.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// FIXED: kHandsSlot is not declared in any supplied header. R2 says Hands is
// internal index 0, so it is defined locally.
constexpr i32 kHandsSlot = 0;

}  // namespace

// R3: first free slot scanning from internal index 1.
i32 firstFreePackSlot(const Inventory& inv) {
    for (i32 i = kFirstItemSlot; i <= kLastItemSlot; ++i) {
        if (slotIsEmpty(inv.pack[static_cast<u32>(i)])) {
            return i;
        }
    }
    return -1;  // UNKNOWN: no-free-slot sentinel value not given; -1 chosen.
}

// R4: slot emptiness is defined in the Formulas section (not supplied).
// UNKNOWN: simplest sensible rule, an item with id 0 is an empty slot.
bool slotIsEmpty(const Item& item) {
    return item.id == 0;
}

}  // namespace mordor
