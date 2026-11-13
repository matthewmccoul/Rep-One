Reference used: `Character.Paralyse`, the C# method that implements the R13.1 guard and R13.2 AD shift. The `status1a` copy matches it. The `status2` copy has no guard, no AD shift, and sets extra flags, so it is expected to diverge.

=== FILE: cases.json ===
[
  {"args": {"level": 5, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": 1, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": 0, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": -1, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": -5, "prior_level": 0, "prior_adshift": 3}, "rng": [0.5]},
  {"args": {"level": 2147483647, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": -2147483648, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": 255, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": 256, "prior_level": 0, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": 7, "prior_level": 3, "prior_adshift": 4}, "rng": [0.5]},
  {"args": {"level": 0, "prior_level": 3, "prior_adshift": 4}, "rng": [0.5]},
  {"args": {"level": 9, "prior_level": -1, "prior_adshift": 4}, "rng": [0.5]},
  {"args": {"level": 1, "prior_level": 1, "prior_adshift": 0}, "rng": [0.5]},
  {"args": {"level": 12, "prior_level": 12, "prior_adshift": -7}, "rng": [0.5]},
  {"args": {"level": -5, "prior_level": -5, "prior_adshift": 2}, "rng": [0.5]},
  {"args": {"level": 4, "prior_level": 2, "prior_adshift": 12}, "rng": [0.5]},
  {"args": {"level": 4, "prior_level": 0, "prior_adshift": 12}, "rng": [0.5]},
  {"args": {"level": 10, "prior_level": 0, "prior_adshift": -5}, "rng": [0.5]},
  {"args": {"level": 6, "prior_level": 0, "prior_adshift": 2147483647}, "rng": [0.5]},
  {"args": {"level": 2, "prior_level": 0, "prior_adshift": -2147483648}, "rng": [0.5]},
  {"args": {"level": 15, "prior_level": 0, "prior_adshift": 7}, "rng": [0.5]},
  {"args": {"level": 8, "prior_level": 0, "prior_adshift": 0}, "rng": [0.0]},
  {"args": {"level": 8, "prior_level": 0, "prior_adshift": 0}, "rng": [0.999]},
  {"args": {"level": 0, "prior_level": 0, "prior_adshift": 9}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // paralyse consumes no random draws (the reference takes no Random); rng is unused.
    public static string Run(JsonElement a, Random rng)
    {
        int level = a.GetProperty("level").GetInt32();
        int priorLevel = a.GetProperty("prior_level").GetInt32();
        int priorAd = a.GetProperty("prior_adshift").GetInt32();

        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        c.ParalysisLevel = priorLevel;
        c.ADShift = priorAd;

        // No item lookup: all items absent on both sides.
        c.Paralyse(level, _ => null);

        // Paralyse returns void; state fields changed by the rule: ParalysisLevel, ADShift.
        return $"{c.ParalysisLevel}|{c.ADShift}";
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

// status2.cpp copy is not file-local: harness renames it to paralyse__status2.
void paralyse__status2(Ctx& ctx, Character& c, i32 level);

// status1a.cpp copy is file-local: reached through shim_status1a.cpp.
void paralyse_via_status1a(Ctx& ctx, Character& c, i32 level);

static void setup(const JsonValue& a, Character& c) {
    c.ailments.paralysisLevel = static_cast<i32>(A(a, "prior_level").asInt());
    c.combat.adShift = static_cast<i32>(A(a, "prior_adshift").asInt());
}

static Str canon(const Character& c) {
    return I(c.ailments.paralysisLevel) + "|" + I(c.combat.adShift);
}

static Str run_status1a(const JsonValue& a, Ctx& ctx) {
    Character c;
    setup(a, c);
    paralyse_via_status1a(ctx, c, static_cast<i32>(A(a, "level").asInt()));
    return canon(c);
}

static Str run_status2(const JsonValue& a, Ctx& ctx) {
    Character c;
    setup(a, c);
    paralyse__status2(ctx, c, static_cast<i32>(A(a, "level").asInt()));
    return canon(c);
}

const CopyEntry kCopies[] = {
    {"paralyse@status1a", &run_status1a},
    {"paralyse@status2", &run_status2},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

=== FILE: shim_status1a.cpp ===
namespace mordor { void paralyse_via_status1a(Ctx& ctx, Character& c, i32 level) { return paralyse__status1a(ctx, c, level); } }
=== END FILE ===

Known limits of this test:
- The C# `StatsADNow` and `StoredADShift` bodies are not in the outline, and the C++ helpers `storedADShift`/`statsADNow` are not shown either. Both sides therefore use a default character with no items, and the AD-shift result is only meaningful if the defaults agree.
- The `status2` copy also sets `paralysed` and `paralyzedRounds`. The reference has no such fields, so they are not in the output.
- The `status2` copy has no level-0 or already-paralysed guard, so it is expected to disagree on the guard cases.

REFERENCE: Character.Paralyse