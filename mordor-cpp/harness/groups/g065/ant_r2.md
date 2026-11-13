The build failed because `Kind` is nested inside `CompanionMishap`, so `Case.cs` needs to qualify it. The C++ side and `cases.json` are unchanged.

=== FILE: cases.json ===
[
{"args":{"kind":1,"guildLevel":10,"bodies":20},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":100,"bodies":20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":100,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":100,"bodies":20},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":0,"bodies":20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":0,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":0,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":-5,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":1,"bodies":0},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":1,"bodies":0},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":1,"bodies":0},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":1,"bodies":-20},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":1,"bodies":-20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":1,"bodies":-20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":1998,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":1999,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":999,"bodies":20},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":1000,"bodies":40},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":999,"bodies":20},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":1000,"bodies":20},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":500,"bodies":20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":999,"bodies":20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":666,"bodies":20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":1,"bodies":40},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":1,"bodies":1},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":1,"bodies":1000},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":2000,"bodies":1},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":2000,"bodies":1},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":2000,"bodies":1},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":20,"bodies":400},"rng":[0.0]},
{"args":{"kind":1,"guildLevel":10,"bodies":200},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":40,"bodies":10},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":40,"bodies":10},"rng":[0.0]},
{"args":{"kind":0,"guildLevel":-100,"bodies":20},"rng":[0.0]},
{"args":{"kind":2,"guildLevel":0,"bodies":0},"rng":[0.0]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        // Chance draws no random numbers; rng is unused.
        var kind = (CompanionMishap.Kind)a.GetProperty("kind").GetInt32();
        int guildLevel = a.GetProperty("guildLevel").GetInt32();
        int bodies = a.GetProperty("bodies").GetInt32();
        return H.F(CompanionMishap.Chance(kind, guildLevel, bodies));
    }
}
=== END FILE ===

=== FILE: case.cpp ===
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
=== END FILE ===

REFERENCE: CompanionMishap.Chance