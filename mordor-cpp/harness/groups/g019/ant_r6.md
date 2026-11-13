=== FILE: cases.json ===
[
  {"args": {"level": -1}, "rng": [0.5]},
  {"args": {"level": 0}, "rng": [0.5]},
  {"args": {"level": 1}, "rng": [0.5]},
  {"args": {"level": 2}, "rng": [0.5]},
  {"args": {"level": 3}, "rng": [0.5]},
  {"args": {"level": 4}, "rng": [0.5]},
  {"args": {"level": 5}, "rng": [0.5]},
  {"args": {"level": 9}, "rng": [0.5]},
  {"args": {"level": 10}, "rng": [0.5]},
  {"args": {"level": 11}, "rng": [0.5]},
  {"args": {"level": 49}, "rng": [0.5]},
  {"args": {"level": 50}, "rng": [0.5]},
  {"args": {"level": 99}, "rng": [0.5]},
  {"args": {"level": 100}, "rng": [0.5]},
  {"args": {"level": 150}, "rng": [0.5]},
  {"args": {"level": 196}, "rng": [0.5]},
  {"args": {"level": 197}, "rng": [0.5]},
  {"args": {"level": 198}, "rng": [0.5]},
  {"args": {"level": 199}, "rng": [0.5]},
  {"args": {"level": 200}, "rng": [0.5]},
  {"args": {"level": 500}, "rng": [0.5]},
  {"args": {"level": 998}, "rng": [0.5]},
  {"args": {"level": 999}, "rng": [0.5]}
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
        // The rule uses no randomness; rng is unused.
        int level = a.GetProperty("level").GetInt32();

        // Human character, Nomad is the default active guild.
        var ch = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male,
                                  new int[] { 10, 10, 10, 10, 10, 10 });

        // Same formula as Character.ExperienceCap(), with the level taken from args
        // (the level setter is not in the shown API, so the level is passed directly).
        float guildExp = (float)ch.ActiveGuildDef.ExpFactor;
        float raceExp = (float)ch.RaceDef.ExpFactor;
        long cap = Rules.Fn13ED(level, guildExp, raceExp);

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

// Factor getters exported from progression1a.cpp by shim_progression1a.cpp.
double guildExpFactor_factor__progression1a(const Ctx& ctx, Guild g);
double raceExpFactor_factor__progression1a(const Ctx& ctx, Race r);

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

// Factors come from the C++ tables through ctx, via the shim exports, the same Nomad and Human values the reference uses.
static Str run_experienceCap_progression2b(const JsonValue& a, Ctx& ctx) {
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    const i32 level = ch.guilds[static_cast<u32>(Guild::Nomad)].level;
    const double gf = guildExpFactor_factor__progression1a(ctx, Guild::Nomad);
    const double rf = raceExpFactor_factor__progression1a(ctx, ch.race);
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

=== FILE: shim_progression1a.cpp ===
namespace mordor {

double guildExpFactor_factor__progression1a(const Ctx& ctx, Guild g) {
    return guildExpFactor(ctx, g);
}

double raceExpFactor_factor__progression1a(const Ctx& ctx, Race r) {
    return raceExpFactor(ctx, r);
}

}  // namespace mordor
=== END FILE ===

Notes on the reference side:
- `Case.cs` calls `Rules.Fn13ED` directly with the same arguments that `Character.ExperienceCap()` passes. The shown API has no setter for `Guild.Level`, so I pass the level from args rather than setting it on a Character.
- The `progression2b` copy returns 0 for level 0, while the reference gives `Fn12A1(1) - 1`. That difference is expected to show up in the level 0 case, and the test is not adjusted to hide it.
- `Character.Create`'s stats argument is assumed to be `int[]`, and `ExpFactor` is cast to `float`. Both are unverified against the actual signatures.

REFERENCE: Rules.Fn13ED (same formula as Character.ExperienceCap)