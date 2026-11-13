#include "case_api.h"
#include "mordor/attributes.h"
#include "mordor/state_character.h"

namespace mordor {

// status1a copy is file-local: reached only through its export wrapper in shim_status1a.cpp.
void unparalyse_export_status1a(Ctx& ctx, Character& c);

// status2 copy: same name as status1a, so the harness renames it to unparalyse__status2.
void unparalyse__status2(Ctx& ctx, Character& c);

// Builds the character as the reference does (Character.Create): Human, Nomad, six stats,
// then sets the two fields this rule reads.
static void setupCharacter(const JsonValue& a, Ctx& ctx, Character& c) {
    Arr<i32, 6> st{};
    st[0] = static_cast<i32>(A(a, "stat0").asInt());
    st[1] = static_cast<i32>(A(a, "stat1").asInt());
    st[2] = static_cast<i32>(A(a, "stat2").asInt());
    st[3] = static_cast<i32>(A(a, "stat3").asInt());
    st[4] = static_cast<i32>(A(a, "stat4").asInt());
    st[5] = static_cast<i32>(A(a, "stat5").asInt());
    createCharacter(ctx, c, Race::Human, Guild::Nomad, st);
    c.ailments.paralysisLevel = static_cast<i32>(A(a, "paralysisLevel").asInt());
    c.combat.adShift = static_cast<i32>(A(a, "adShift").asInt());
}

static Str run_unparalyse_status1a(const JsonValue& a, Ctx& ctx) {
    Character c;
    setupCharacter(a, ctx, c);
    unparalyse_export_status1a(ctx, c);
    return I(c.combat.adShift) + "|" + I(c.ailments.paralysisLevel);
}

static Str run_unparalyse_status2(const JsonValue& a, Ctx& ctx) {
    Character c;
    setupCharacter(a, ctx, c);
    unparalyse__status2(ctx, c);
    return I(c.combat.adShift) + "|" + I(c.ailments.paralysisLevel);
}

const CopyEntry kCopies[] = {
    {"unparalyse@status1a", &run_unparalyse_status1a},
    {"unparalyse@status2", &run_unparalyse_status2},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
