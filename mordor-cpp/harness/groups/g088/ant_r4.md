Two copies are reachable: `unparalyse__status2` directly, and the file-local `unparalyse` in status1a through `shim_status1a.cpp`. Each C++ case now builds its Character with `createCharacter(ctx, c, Race::Human, Guild::Nomad, st)`, and the stats come from `stat0`..`stat5` keys, which match the C# side. The case file has no array accessor, so I used separate keys.

Assumptions to check: `createCharacter` takes `(Ctx&, Character&, Race, Guild, Arr<i32,6>)`, and `Arr<i32,6>` supports `operator[]`. The shim calls `unparalyse__status1a`, the renamed name, since the harness compiles that source with the rename.

=== FILE: cases.json ===
[
{"args": {"paralysisLevel": 0, "adShift": 0, "stat0": 10, "stat1": 10, "stat2": 10, "stat3": 10, "stat4": 10, "stat5": 10}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": 5, "stat0": 18, "stat1": 3, "stat2": 12, "stat3": 15, "stat4": 8, "stat5": 14}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": -7, "stat0": 0, "stat1": 0, "stat2": 0, "stat3": 0, "stat4": 0, "stat5": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": 1000, "stat0": 25, "stat1": 25, "stat2": 25, "stat3": 25, "stat4": 25, "stat5": 25}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": -1, "stat0": 3, "stat1": 18, "stat2": 7, "stat3": 9, "stat4": 11, "stat5": 16}, "rng": [0.5]},
{"args": {"paralysisLevel": 0, "adShift": 2147483000, "stat0": 12, "stat1": 6, "stat2": 4, "stat3": 17, "stat4": 2, "stat5": 20}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": 0, "stat0": 10, "stat1": 10, "stat2": 10, "stat3": 10, "stat4": 10, "stat5": 10}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": 5, "stat0": 16, "stat1": 8, "stat2": 13, "stat3": 11, "stat4": 9, "stat5": 7}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": -7, "stat0": 0, "stat1": 0, "stat2": 0, "stat3": 0, "stat4": 0, "stat5": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": -1, "stat0": 25, "stat1": 1, "stat2": 25, "stat3": 1, "stat4": 25, "stat5": 1}, "rng": [0.5]},
{"args": {"paralysisLevel": 2, "adShift": 0, "stat0": 14, "stat1": 14, "stat2": 14, "stat3": 14, "stat4": 14, "stat5": 14}, "rng": [0.5]},
{"args": {"paralysisLevel": 2, "adShift": 12, "stat0": 5, "stat1": 19, "stat2": 9, "stat3": 22, "stat4": 4, "stat5": 13}, "rng": [0.5]},
{"args": {"paralysisLevel": 3, "adShift": 1000, "stat0": 18, "stat1": 18, "stat2": 18, "stat3": 18, "stat4": 18, "stat5": 18}, "rng": [0.5]},
{"args": {"paralysisLevel": 3, "adShift": -1000, "stat0": 7, "stat1": 11, "stat2": 15, "stat3": 3, "stat4": 19, "stat5": 2}, "rng": [0.5]},
{"args": {"paralysisLevel": 5, "adShift": 7, "stat0": 9, "stat1": 9, "stat2": 9, "stat3": 9, "stat4": 9, "stat5": 9}, "rng": [0.5]},
{"args": {"paralysisLevel": 9, "adShift": 0, "stat0": 20, "stat1": 4, "stat2": 6, "stat3": 8, "stat4": 10, "stat5": 12}, "rng": [0.5]},
{"args": {"paralysisLevel": 10, "adShift": -50, "stat0": 13, "stat1": 13, "stat2": 13, "stat3": 13, "stat4": 13, "stat5": 13}, "rng": [0.5]},
{"args": {"paralysisLevel": 127, "adShift": 1, "stat0": 1, "stat1": 2, "stat2": 3, "stat3": 4, "stat4": 5, "stat5": 6}, "rng": [0.5]},
{"args": {"paralysisLevel": 255, "adShift": 0, "stat0": 25, "stat1": 0, "stat2": 25, "stat3": 0, "stat4": 25, "stat5": 0}, "rng": [0.5]},
{"args": {"paralysisLevel": 255, "adShift": -100, "stat0": 11, "stat1": 15, "stat2": 17, "stat3": 19, "stat4": 21, "stat5": 23}, "rng": [0.5]},
{"args": {"paralysisLevel": -1, "adShift": 0, "stat0": 10, "stat1": 10, "stat2": 10, "stat3": 10, "stat4": 10, "stat5": 10}, "rng": [0.5]},
{"args": {"paralysisLevel": -1, "adShift": 3, "stat0": 8, "stat1": 16, "stat2": 24, "stat3": 2, "stat4": 6, "stat5": 4}, "rng": [0.5]},
{"args": {"paralysisLevel": 1, "adShift": 2147483000, "stat0": 15, "stat1": 15, "stat2": 15, "stat3": 15, "stat4": 15, "stat5": 15}, "rng": [0.5]},
{"args": {"paralysisLevel": 2147483647, "adShift": 0, "stat0": 12, "stat1": 12, "stat2": 12, "stat3": 12, "stat4": 12, "stat5": 12}, "rng": [0.5]}
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
        var stats = new int[6];
        for (int i = 0; i < 6; i++) stats[i] = a.GetProperty("stat" + i).GetInt32();

        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
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
=== END FILE ===
=== FILE: shim_status1a.cpp ===
namespace mordor {

// Export wrapper for the file-local copy unparalyse in src/status1a.cpp.
void unparalyse_export_status1a(Ctx& ctx, Character& c) { return unparalyse__status1a(ctx, c); }

} // namespace mordor
=== END FILE ===

REFERENCE: Character.Unparalyse