The reference `Character.RecalcSpellPoints` implements this rule. The test can't vary the maximum, though. The C# and C++ max formulas depend on character stats, and the C++ `Character` excerpt doesn't show the type or setter for `abilities`. So every case uses zero stats on both sides, and the test checks the refill and clamp branches against whatever maximum zero stats produce. Boundary tests on the maximum itself need the stat setter.

=== FILE: cases.json ===
[
{"args":{"refill":1,"cur":-5,"prevMax":0},"rng":[0.5]},
{"args":{"refill":1,"cur":0,"prevMax":0},"rng":[0.5]},
{"args":{"refill":1,"cur":1,"prevMax":0},"rng":[0.5]},
{"args":{"refill":1,"cur":10,"prevMax":0},"rng":[0.5]},
{"args":{"refill":1,"cur":100,"prevMax":0},"rng":[0.5]},
{"args":{"refill":1,"cur":-5,"prevMax":50},"rng":[0.5]},
{"args":{"refill":1,"cur":0,"prevMax":50},"rng":[0.5]},
{"args":{"refill":1,"cur":1,"prevMax":50},"rng":[0.5]},
{"args":{"refill":1,"cur":10,"prevMax":50},"rng":[0.5]},
{"args":{"refill":1,"cur":100,"prevMax":50},"rng":[0.5]},
{"args":{"refill":0,"cur":-5,"prevMax":0},"rng":[0.5]},
{"args":{"refill":0,"cur":0,"prevMax":0},"rng":[0.5]},
{"args":{"refill":0,"cur":1,"prevMax":0},"rng":[0.5]},
{"args":{"refill":0,"cur":10,"prevMax":0},"rng":[0.5]},
{"args":{"refill":0,"cur":100,"prevMax":0},"rng":[0.5]},
{"args":{"refill":0,"cur":-5,"prevMax":50},"rng":[0.5]},
{"args":{"refill":0,"cur":0,"prevMax":50},"rng":[0.5]},
{"args":{"refill":0,"cur":1,"prevMax":50},"rng":[0.5]},
{"args":{"refill":0,"cur":10,"prevMax":50},"rng":[0.5]},
{"args":{"refill":0,"cur":100,"prevMax":50},"rng":[0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        var stats = new int[6];
        var c = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        c.MaxSpellPoints = a.GetProperty("prevMax").GetInt32();
        c.CurSpellPoints = a.GetProperty("cur").GetInt32();
        bool refill = a.GetProperty("refill").GetInt32() != 0;
        c.RecalcSpellPoints(refill);
        return c.MaxSpellPoints.ToString(System.Globalization.CultureInfo.InvariantCulture)
             + "|" + c.CurSpellPoints.ToString(System.Globalization.CultureInfo.InvariantCulture);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
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
=== END FILE ===

REFERENCE: Character.RecalcSpellPoints