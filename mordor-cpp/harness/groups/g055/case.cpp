#include "case_api.h"
#include "mordor/spells1a.h"
#include "mordor/spells3.h"

namespace mordor {

// Export for the file-local monsterSpellDamage (see shim_spells1a.cpp).
i32 mordor_export_monsterSpellDamage(Ctx& ctx, i32 monsterLevel, i32 damage2, i32 specialEffect,
                                     bool hasResist, i32 resistPercent);

static i32 geti(const JsonValue& a, const char* k) {
    return static_cast<i32>(A(a, k).asInt());
}

// Reference applies resistance unconditionally; has_resist = 0 maps to resistance 0 (identity).
static i32 resistEff(const JsonValue& a) {
    return geti(a, "has_resist") != 0 ? geti(a, "resist") : 0;
}

static Str run_playerSpellDamage(const JsonValue& a, Ctx& ctx) {
    const i32 base = geti(a, "base");
    const i32 power = geti(a, "power");
    const i32 sl = geti(a, "sl");
    // Copy's parameter is specialEffect = power - 1 (reference computes power - 1 internally).
    const i32 r = playerSpellDamage(ctx, base, power - 1, sl, resistEff(a));
    return I(r);
}

static Str run_monsterSpellDamage(const JsonValue& a, Ctx& ctx) {
    const i32 base = geti(a, "base");
    const i32 power = geti(a, "power");
    const i32 sl = geti(a, "sl");
    const bool hasResist = geti(a, "has_resist") != 0;
    // Copy takes hasResist and the raw resistance, so pass them unmodified.
    const i32 r = mordor_export_monsterSpellDamage(ctx, sl, base, power - 1, hasResist, geti(a, "resist"));
    return I(r);
}

static Str run_spellDamage(const JsonValue& a, Ctx& ctx) {
    const i32 r = spellDamage(ctx, geti(a, "base"), geti(a, "sl"), geti(a, "power"), resistEff(a));
    return I(r);
}

const CopyEntry kCopies[] = {
    {"playerSpellDamage", &run_playerSpellDamage},
    {"monsterSpellDamage", &run_monsterSpellDamage},
    {"spellDamage", &run_spellDamage},
};
const std::size_t kNumCopies = 3;

} // namespace mordor
