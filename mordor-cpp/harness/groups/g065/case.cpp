#include "case_api.h"
#include "mordor/companions1a.h"
#include "mordor/companions2a.h"
#include "mordor/combat1b.h"

namespace mordor {

// Same-name copies, renamed by the harness at compile time.
f64 mishapChance__combat1b(MishapSource source, i32 monsterLevel, i32 partyBodies);
f64 mishapChance__companions1a(MishapKind kind, i32 guildLevel, i32 bodies);

// Ordinal mapping matches the C# Kind enum: 0=Breath, 1=Spit, 2=Spell.
// ASSUMPTION: C++ enum ordinals follow the same order.

static Str run_mishapChance_companions1a(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 kind = A(a, "kind").asInt();
    const i32 guildLevel = A(a, "guildLevel").asInt();
    const i32 bodies = A(a, "bodies").asInt();
    const f64 v = mishapChance__companions1a(static_cast<MishapKind>(kind), guildLevel, bodies);
    return F(v);
}

static Str run_mishapChance_combat1b(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 kind = A(a, "kind").asInt();
    const i32 level = A(a, "guildLevel").asInt();
    const i32 bodies = A(a, "bodies").asInt();
    // 0 -> Breath, 1 -> SpitOrMelee, 2 -> spell source (any other value takes the spell tuning).
    MishapSource source = static_cast<MishapSource>(kind);
    if (kind == 0) source = MishapSource::Breath;
    else if (kind == 1) source = MishapSource::SpitOrMelee;
    const f64 v = mishapChance__combat1b(source, level, bodies);
    return F(v);
}

const CopyEntry kCopies[] = {
    {"mishapChance@combat1b", &run_mishapChance_combat1b},
    {"mishapChance@companions1a", &run_mishapChance_companions1a},
};
const std::size_t kNumCopies = sizeof(kCopies) / sizeof(kCopies[0]);

} // namespace mordor
