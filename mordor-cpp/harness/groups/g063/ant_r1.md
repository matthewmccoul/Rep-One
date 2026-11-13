=== FILE: cases.json ===
[
{"args": {"int": 0, "wis": 0}, "rng": [0.5]},
{"args": {"int": 1, "wis": 0}, "rng": [0.5]},
{"args": {"int": 0, "wis": 1}, "rng": [0.5]},
{"args": {"int": 1, "wis": 1}, "rng": [0.5]},
{"args": {"int": 3, "wis": 3}, "rng": [0.5]},
{"args": {"int": 10, "wis": 10}, "rng": [0.5]},
{"args": {"int": 15, "wis": 14}, "rng": [0.5]},
{"args": {"int": 18, "wis": 18}, "rng": [0.5]},
{"args": {"int": 25, "wis": 25}, "rng": [0.5]},
{"args": {"int": 100, "wis": 0}, "rng": [0.5]},
{"args": {"int": 0, "wis": 100}, "rng": [0.5]},
{"args": {"int": -1, "wis": 0}, "rng": [0.5]},
{"args": {"int": 0, "wis": -1}, "rng": [0.5]},
{"args": {"int": -1, "wis": -1}, "rng": [0.5]},
{"args": {"int": 5, "wis": -5}, "rng": [0.5]},
{"args": {"int": -5, "wis": 5}, "rng": [0.5]},
{"args": {"int": 7, "wis": 3}, "rng": [0.5]},
{"args": {"int": 3, "wis": 7}, "rng": [0.5]},
{"args": {"int": 255, "wis": 255}, "rng": [0.5]},
{"args": {"int": 1000, "wis": 1000}, "rng": [0.5]},
{"args": {"int": 100000, "wis": 100000}, "rng": [0.5]},
{"args": {"int": -100, "wis": 40}, "rng": [0.5]},
{"args": {"int": 2, "wis": 3}, "rng": [0.5]},
{"args": {"int": 13, "wis": 4}, "rng": [0.5]},
{"args": {"int": 6, "wis": 0}, "rng": [0.5]},
{"args": {"int": 0, "wis": 6}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int intel = a.GetProperty("int").GetInt32();
        int wis = a.GetProperty("wis").GetInt32();

        var stats = new int[6];
        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        // Set natural stats directly so race caps do not alter the inputs.
        c.Stats[(int)Stat.Int] = intel;
        c.Stats[(int)Stat.Wis] = wis;

        int result = Spellbook.MaxSpellPoints(c);
        return result.ToString(CultureInfo.InvariantCulture);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
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
=== END FILE ===

UNREACHABLE: maxSpellPoints@casting3 - the copy takes a C++ Character and reads its stats through statWithMods(c, StatIndex::Int/Wis). The shown Character excerpt has no stat field, and the source of statWithMods and StatIndex is not shown, so natural Int and Wis cannot be set without inventing a field name. Supplying the stat field name would make this copy testable.

REFERENCE: Spellbook.MaxSpellPoints