=== FILE: cases.json ===
[
  {"args": {"level": -1}, "rng": [0.5]},
  {"args": {"level": 0}, "rng": [0.5]},
  {"args": {"level": 1}, "rng": [0.5]},
  {"args": {"level": 2}, "rng": [0.5]},
  {"args": {"level": 3}, "rng": [0.5]},
  {"args": {"level": 4}, "rng": [0.5]},
  {"args": {"level": 5}, "rng": [0.5]},
  {"args": {"level": 6}, "rng": [0.5]},
  {"args": {"level": 7}, "rng": [0.5]},
  {"args": {"level": 8}, "rng": [0.5]},
  {"args": {"level": 9}, "rng": [0.5]},
  {"args": {"level": 10}, "rng": [0.5]},
  {"args": {"level": 11}, "rng": [0.5]},
  {"args": {"level": 12}, "rng": [0.5]},
  {"args": {"level": 13}, "rng": [0.5]},
  {"args": {"level": 14}, "rng": [0.5]},
  {"args": {"level": 15}, "rng": [0.5]},
  {"args": {"level": 16}, "rng": [0.5]},
  {"args": {"level": 17}, "rng": [0.5]},
  {"args": {"level": 18}, "rng": [0.5]},
  {"args": {"level": 19}, "rng": [0.5]},
  {"args": {"level": 20}, "rng": [0.5]},
  {"args": {"level": 21}, "rng": [0.5]},
  {"args": {"level": 22}, "rng": [0.5]},
  {"args": {"level": 23}, "rng": [0.5]},
  {"args": {"level": 24}, "rng": [0.5]},
  {"args": {"level": 25}, "rng": [0.5]},
  {"args": {"level": 26}, "rng": [0.5]},
  {"args": {"level": 27}, "rng": [0.5]},
  {"args": {"level": 28}, "rng": [0.5]},
  {"args": {"level": 29}, "rng": [0.5]},
  {"args": {"level": 30}, "rng": [0.5]},
  {"args": {"level": 31}, "rng": [0.5]},
  {"args": {"level": 32}, "rng": [0.5]},
  {"args": {"level": 33}, "rng": [0.5]},
  {"args": {"level": 34}, "rng": [0.5]},
  {"args": {"level": 35}, "rng": [0.5]},
  {"args": {"level": 36}, "rng": [0.5]},
  {"args": {"level": 37}, "rng": [0.5]},
  {"args": {"level": 38}, "rng": [0.5]},
  {"args": {"level": 39}, "rng": [0.5]},
  {"args": {"level": 40}, "rng": [0.5]}
]

=== END FILE ===
=== FILE: Case.cs ===
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, System.Random rng)
    {
        int level = a.GetProperty("level").GetInt32();
        var stats = new int[6];
        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        c.Guild.Level = level;
        long cap = c.ExperienceCap();
        return cap.ToString(CultureInfo.InvariantCulture);
    }
}

=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/progression1a.h"
#include "mordor/progression2.h"
#include "mordor/progression2b.h"
#include "mordor/state_character.h"

namespace mordor {

// Same-name copies, renamed by the harness; declared here with the original signatures.
i64 experienceCap__progression1a(const Ctx& ctx, const Character& ch, Guild g);
i64 experienceCap__progression2b(i32 level, double guildExpFactor, double raceExpFactor);

static Character nomadAt(i32 level) {
    Character ch;
    ch.race = Race::Human;
    ch.guilds[static_cast<u32>(Guild::Nomad)].joined = true;
    ch.guilds[static_cast<u32>(Guild::Nomad)].level = level;
    return ch;
}

static Str run_experienceCapOf(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    return I(experienceCapOf(ch, Guild::Nomad));
}

static Str run_experienceCap_progression1a(const JsonValue& a, Ctx& ctx) {
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    return I(experienceCap__progression1a(ctx, ch, Guild::Nomad));
}

// Factors come from the C++ tables through ctx, the same Nomad and Human values the reference uses.
static Str run_experienceCap_progression2b(const JsonValue& a, Ctx& ctx) {
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    const i32 level = ch.guilds[static_cast<u32>(Guild::Nomad)].level;
    const double gf = guildExpFactor(ctx, Guild::Nomad);
    const double rf = raceExpFactor(ctx, ch.race);
    return I(experienceCap__progression2b(level, gf, rf));
}

const CopyEntry kCopies[] = {
    {"experienceCapOf", &run_experienceCapOf},
    {"experienceCap@progression1a", &run_experienceCap_progression1a},
    {"experienceCap@progression2b", &run_experienceCap_progression2b},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor

=== END FILE ===

Assumptions to check before running:
- Case.cs sets the Nomad level through `c.Guild.Level`, the same member the reference reads in `ExperienceCap()`. If `Guild` is not settable there, the C# side needs whatever setter the reference uses.
- `guildExpFactor(ctx, g)` and `raceExpFactor(ctx, race)` are the table-reading functions that the progression1a copy calls. I took their return type as double and assumed they are declared in one of the included headers.

REFERENCE: Character.ExperienceCap (via Rules.Fn13ED)