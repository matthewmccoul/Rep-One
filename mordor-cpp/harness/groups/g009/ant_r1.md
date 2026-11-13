Only `CompanionMood.Drift` implements this rule, so I built the test against it. The cases stay within the rule's stated scope, `Binding > 0`. The other branch of `Drift` (`BindingLevel < 0`) is a different rule and is not exercised.

Three assumptions to check before running:
- `Character.TopGuildLevel` is read by the reference, so I assume it has a public setter. The `Character` outline is truncated before that member.
- The `Monster` is built fresh with `new Monster { GuildLevel = ... }` rather than picked from `H.Monsters`. The rule reads only `GuildLevel`, and mutating a shared table record would corrupt other tests. The C++ side has no monster table to pick from for this reason.
- The C++ copies' headers are assumed to live at `mordor/companions1b.h`, `mordor/companions2a.h` and `mordor/status1b.h`. All three functions are declared in headers, so no shim is needed.

=== FILE: cases.json ===
[
  {"args": {"guild_level": 0, "top_guild_level": 0, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 3, "top_guild_level": 0, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 1, "top_guild_level": 1, "binding_level": 2}, "rng": [0.5]},
  {"args": {"guild_level": 5, "top_guild_level": 2, "binding_level": 4}, "rng": [0.5]},
  {"args": {"guild_level": 12, "top_guild_level": 12, "binding_level": 12}, "rng": [0.5]},
  {"args": {"guild_level": 10, "top_guild_level": 4, "binding_level": 6}, "rng": [0.5]},
  {"args": {"guild_level": 6, "top_guild_level": 3, "binding_level": 3}, "rng": [0.5]},
  {"args": {"guild_level": 7, "top_guild_level": 0, "binding_level": 20}, "rng": [0.5]},
  {"args": {"guild_level": 2, "top_guild_level": 5, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 1, "top_guild_level": 1, "binding_level": 5}, "rng": [0.5]},
  {"args": {"guild_level": 1, "top_guild_level": 1, "binding_level": 4}, "rng": [0.5]},
  {"args": {"guild_level": 32767, "top_guild_level": 0, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 32767, "top_guild_level": 32767, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 12, "top_guild_level": 0, "binding_level": 2147483647}, "rng": [0.5]},
  {"args": {"guild_level": 12, "top_guild_level": 12, "binding_level": 2147483647}, "rng": [0.5]},
  {"args": {"guild_level": -2, "top_guild_level": 3, "binding_level": 2}, "rng": [0.5]},
  {"args": {"guild_level": 0, "top_guild_level": 12, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 4, "top_guild_level": 1, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 9, "top_guild_level": 9, "binding_level": 9}, "rng": [0.5]},
  {"args": {"guild_level": 9, "top_guild_level": 9, "binding_level": 8}, "rng": [0.5]},
  {"args": {"guild_level": 8, "top_guild_level": 3, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 3, "top_guild_level": 2, "binding_level": 7}, "rng": [0.5]},
  {"args": {"guild_level": 20, "top_guild_level": 1, "binding_level": 100}, "rng": [0.5]},
  {"args": {"guild_level": 1, "top_guild_level": 0, "binding_level": 3}, "rng": [0.5]},
  {"args": {"guild_level": 2, "top_guild_level": 12, "binding_level": 3}, "rng": [0.5]},
  {"args": {"guild_level": 15, "top_guild_level": 6, "binding_level": 2}, "rng": [0.5]},
  {"args": {"guild_level": 12, "top_guild_level": 1, "binding_level": 24}, "rng": [0.5]},
  {"args": {"guild_level": 5, "top_guild_level": 5, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 100, "top_guild_level": 50, "binding_level": 1}, "rng": [0.5]},
  {"args": {"guild_level": 0, "top_guild_level": 0, "binding_level": 2147483647}, "rng": [0.5]}
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
        int guild = a.GetProperty("guild_level").GetInt32();
        int top = a.GetProperty("top_guild_level").GetInt32();
        int binding = a.GetProperty("binding_level").GetInt32();

        var who = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        who.TopGuildLevel = top;

        var def = new Monster { GuildLevel = checked((short)guild) };
        var c = new Companion { BindingLevel = binding };

        double drift = CompanionMood.Drift(who, c, def);
        return H.F(drift);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/companions1b.h"
#include "mordor/companions2a.h"
#include "mordor/status1b.h"

namespace mordor {

static Str run_companionDriftBound(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const int guild = A(a, "guild_level").asInt();
    const int top = A(a, "top_guild_level").asInt();
    const int binding = A(a, "binding_level").asInt();
    return F(companionDriftBound(guild, top, binding));
}

static Str run_driftBoundStep(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const int guild = A(a, "guild_level").asInt();
    const int top = A(a, "top_guild_level").asInt();
    const int binding = A(a, "binding_level").asInt();
    return F(driftBoundStep(guild, top, binding));
}

static Str run_boundDrift(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const int guild = A(a, "guild_level").asInt();
    const int top = A(a, "top_guild_level").asInt();
    const int binding = A(a, "binding_level").asInt();
    return F(boundDrift(guild, top, binding));
}

const CopyEntry kCopies[] = {
    {"companionDriftBound", &run_companionDriftBound},
    {"driftBoundStep", &run_driftBoundStep},
    {"boundDrift", &run_boundDrift},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor
=== END FILE ===

REFERENCE: CompanionMood.Drift