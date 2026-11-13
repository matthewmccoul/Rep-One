#include "case_api.h"

namespace mordor {

// Renamed copies (same-name bufferForSpell in two files).
i32 bufferForSpell__feat_buffers(i32 spellIndex);
i32 bufferForSpell__items2a(i32 spellIndex);

// Export wrapper for the file-local copy, defined in shim_feat_buffers.cpp.
i32 shim_feat_buffers_export(i32 spellIndex);

static Str run_feat_buffers(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 spellIndex = static_cast<i32>(A(a, "spellIndex").asInt());
    return I(shim_feat_buffers_export(spellIndex));
}

static Str run_items2a(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 spellIndex = static_cast<i32>(A(a, "spellIndex").asInt());
    return I(bufferForSpell__items2a(spellIndex));
}

const CopyEntry kCopies[] = {
    {"bufferForSpell@feat_buffers", &run_feat_buffers},
    {"bufferForSpell@items2a", &run_items2a},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
