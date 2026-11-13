#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

i32 exp_freeStorageSlot_items2b(const Inventory& inv);
i32 exp_freeSlot_items2b_2(const Inventory& inv);

// Pack occupancy: bit k of "lo" = pack slot k+1 (1..20), bit k of "hi" = pack slot k+21 (21..40).
static Inventory buildInv(const JsonValue& a) {
    Inventory inv{};
    long long lo = A(a, "lo").asInt();
    long long hi = A(a, "hi").asInt();
    for (int k = 0; k < 20; ++k) {
        if ((lo >> k) & 1) inv.pack[k + 1].id = 1;
        if ((hi >> k) & 1) inv.pack[k + 21].id = 1;
    }
    return inv;
}

static Str run_freeStorageSlot(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Inventory inv = buildInv(a);
    return I(exp_freeStorageSlot_items2b(inv));
}

static Str run_freeSlot(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Inventory inv = buildInv(a);
    return I(exp_freeSlot_items2b_2(inv));
}

const CopyEntry kCopies[] = {
    {"freeStorageSlot", &run_freeStorageSlot},
    {"freeSlot", &run_freeSlot},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
