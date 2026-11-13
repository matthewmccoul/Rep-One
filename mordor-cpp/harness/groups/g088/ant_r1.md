=== FILE: cases.json ===
[
{"args": {"paralysisLevel": 0, "adShift": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": 5}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": -7}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": 1000}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": -1}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": 2147483000}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": 5}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": -7}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": -1}, "rng": [0.5]},
{"args": {"paralysisLevel": 2, "adShift": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 2, "adShift": 12}, "rng": [0.5]},
{"args": {"paralysisLevel": 3, "adShift": 1000}, "rng": [0.5]},
{"args": {"paralysisLevel": 3, "adShift": -1000}, "rng": [0.5]},
{"args": {"paralysisLevel": 5, "adShift": 7}, "rng": [0.5]},
{"args": {"paralysisLevel": 9, "adShift": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 10, "adShift": -50}, "rng": [0.5]},
{"args": {"paralysisLevel": 127, "adShift": 1}, "rng": [0.5]},
{"args": {"paralysisLevel": 255, "adShift": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 255, "adShift": -100}, "rng": [0.5]},
{"args": {"paralysisLevel": -1, "adShift": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": -1, "adShift": 3}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": 2147483000}, "rng": [0.5]},
{"args": {"paralysisLevel": 2147483647, "adShift": 0}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        int pl = a.GetProperty("paralysisLevel").GetInt32();
        int ad = a.GetProperty("adShift").GetInt32();

        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        c.ParalysisLevel = pl;
        c.ADShift = ad;

        // No items are worn; the rule takes no random draws, so rng is not passed.
        c.Unparalyse(_ => null);

        return $"{c.ADShift}|{c.ParalysisLevel}";
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

// Renamed copies (same-name copies share the name "unparalyse").
void unparalyse__status1a(Ctx& ctx, Character& c);
void unparalyse__status2(Ctx& ctx, Character& c);

// Export wrapper for the file-local copy in src/status1a.cpp (see shim_status1a.cpp).
void unparalyse_status1a_export(Ctx& ctx, Character& c);

static Str run_unparalyse_status1a(const JsonValue& a, Ctx& ctx) {
    Character c;
    c.ailments.paralysisLevel = static_cast<i32>(A(a, "paralysisLevel").asInt());
    c.combat.adShift = static_cast<i32>(A(a, "adShift").asInt());
    unparalyse_status1a_export(ctx, c);
    return I(c.combat.adShift) + "|" + I(c.ailments.paralysisLevel);
}

static Str run_unparalyse_status2(const JsonValue& a, Ctx& ctx) {
    Character c;
    c.ailments.paralysisLevel = static_cast<i32>(A(a, "paralysisLevel").asInt());
    c.combat.adShift = static_cast<i32>(A(a, "adShift").asInt());
    unparalyse__status2(ctx, c);
    return I(c.combat.adShift) + "|" + I(c.ailments.paralysisLevel);
}

const CopyEntry kCopies[] = {
    {"unparalyse@status1a", &run_unparalyse_status1a},
    {"unparalyse@status2", &run_unparalyse_status2},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
=== END FILE ===

=== FILE: shim_status1a.cpp ===
namespace mordor {
void unparalyse_status1a_export(Ctx& ctx, Character& c) {
    unparalyse__status1a(ctx, c);
}
} // namespace mordor
=== END FILE ===

REFERENCE: Character.Unparalyse