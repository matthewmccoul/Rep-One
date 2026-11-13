Three files follow. The C# reference is `Rules.Fn13ED`. Two assumptions limit the test:

- **Factors are pinned at 1.0.** The C++ copies `experienceCapOf` and `experienceCap@progression1a` read guild and race factors from the tables, so the test cannot vary them without breaking identical inputs. I used 1.0 because the `progression2b` comment says the quadratic reproduces the Human Nomad anchors with unit factors. If the table values differ, those two copies will mismatch.
- **Guild is Nomad and race is Human throughout.** The only guild and race identifiers shown on the C++ side are `Guild::Nomad` and `Race::Human`. The factor-driven clamp branches in the `progression2b` copy are therefore not covered. Level 0 and the negative-level path are covered.

=== FILE: cases.json ===
[
  {"args": {"level": -5, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": -1, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 0, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 1, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 2, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 3, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 4, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 5, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 9, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 10, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 11, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 19, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 20, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 49, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 50, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 99, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 100, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 101, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 196, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 197, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 198, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 199, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 500, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 998, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]},
  {"args": {"level": 999, "guildExp": 1.0, "raceExp": 1.0}, "rng": [0.5]}
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
        float guildExp = (float)a.GetProperty("guildExp").GetDouble();
        float raceExp = (float)a.GetProperty("raceExp").GetDouble();
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

static Character nomadAt(i32 level) {
    Character ch;
    ch.race = Race::Human;
    ch.guilds[static_cast<u32>(Guild::Nomad)].joined = true;
    ch.guilds[static_cast<u32>(Guild::Nomad)].level = level;
    return ch;
}

// Table-derived factors (unit for Human/Nomad); args factors are not passed to these copies.
static Str run_experienceCapOf(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    return I(experienceCapOf(ch, Guild::Nomad));
}

static Str run_experienceCap_progression1a(const JsonValue& a, Ctx& ctx) {
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    return I(experienceCap__progression1a(ctx, ch, Guild::Nomad));
}

static Str run_experienceCap_progression2b(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 level = static_cast<i32>(A(a, "level").asInt());
    const double gf = A(a, "guildExp").asDouble();
    const double rf = A(a, "raceExp").asDouble();
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

REFERENCE: Rules.Fn13ED