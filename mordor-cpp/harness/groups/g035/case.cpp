#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/attributes.h"
#include "mordor/casting1.h"

namespace mordor {

static Str canon_sp(const Character& c) {
    return I(c.vitals.maxSpellPoints) + Str("|") + I(c.vitals.curSpellPoints);
}

static Str run_recalcSP(const JsonValue& a, Ctx& ctx) {
    Character c{};
    c.vitals.maxSpellPoints = static_cast<i32>(A(a, "prevMax").asInt());
    c.vitals.curSpellPoints = static_cast<i32>(A(a, "cur").asInt());
    const bool refill = A(a, "refill").asInt() != 0;
    recalcSP(ctx, c, refill);
    return canon_sp(c);
}

static Str run_recalcSpellPoints(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Character c{};
    c.vitals.maxSpellPoints = static_cast<i32>(A(a, "prevMax").asInt());
    c.vitals.curSpellPoints = static_cast<i32>(A(a, "cur").asInt());
    const bool refill = A(a, "refill").asInt() != 0;
    recalcSpellPoints(c, refill);
    return canon_sp(c);
}

const CopyEntry kCopies[] = {
    {"recalcSP", &run_recalcSP},
    {"recalcSpellPoints", &run_recalcSpellPoints},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
