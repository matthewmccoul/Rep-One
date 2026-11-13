#include "case_api.h"

namespace mordor {

// Same-name copy in src/casting2.cpp; the harness compiles it under this renamed symbol.
i32 maxSpellPoints__casting2(i32 naturalInt, i32 naturalWis);

static Str run_maxSpellPoints_casting2(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 intel = static_cast<i32>(A(a, "int").asInt());
    const i32 wis = static_cast<i32>(A(a, "wis").asInt());
    return I(maxSpellPoints__casting2(intel, wis));
}

const CopyEntry kCopies[] = {
    {"maxSpellPoints@casting2", &run_maxSpellPoints_casting2},
};
const std::size_t kNumCopies = 1;

}  // namespace mordor
