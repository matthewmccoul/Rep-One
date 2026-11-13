#include "case_api.h"

namespace mordor {

// Exported by the shims appended to each copy's source file (see shim_*.cpp).
int mordor_export_questItemBand(int tier);
int mordor_export_itemBand(int tier);
int mordor_export_ItemQuestBand(int tier);

static Str run_questItemBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule takes no RNG
    int tier = static_cast<int>(A(a, "tier").asInt());
    return I(mordor_export_questItemBand(tier));
}

static Str run_itemBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    int tier = static_cast<int>(A(a, "tier").asInt());
    return I(mordor_export_itemBand(tier));
}

static Str run_ItemQuestBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    int tier = static_cast<int>(A(a, "tier").asInt());
    return I(mordor_export_ItemQuestBand(tier));
}

const CopyEntry kCopies[] = {
    {"questItemBand", &run_questItemBand},
    {"itemBand", &run_itemBand},
    {"ItemQuestBand", &run_ItemQuestBand},
};
const std::size_t kNumCopies = sizeof(kCopies) / sizeof(kCopies[0]);

}  // namespace mordor
