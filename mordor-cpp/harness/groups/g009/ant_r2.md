The build failed because `Character.TopGuildLevel` is read-only, and the outline doesn't show a setter or constructor parameter for it. Rather than invent a setter name, `Case.cs` now finds the existing integer storage field whose name contains `TopGuildLevel`, writes the value there, and throws if the property doesn't read back the same value. If that field doesn't exist, the run fails loudly instead of testing the wrong top level. The first run may still fail at runtime if `TopGuildLevel` is computed from something other than a backing field.

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
using System.Linq;
using System.Reflection;
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
        SetTopGuildLevel(who, top);

        var def = new Monster { GuildLevel = checked((short)guild) };
        var c = new Companion { BindingLevel = binding };

        double drift = CompanionMood.Drift(who, c, def);
        return H.F(drift);
    }

    // TopGuildLevel is a read-only property. Locate the existing int storage field
    // whose name contains "TopGuildLevel" and write to it. Fail if it is not found
    // or if the property does not read back the value written.
    static void SetTopGuildLevel(Character who, int top)
    {
        var fields = typeof(Character)
            .GetFields(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public)
            .Where(f => f.FieldType == typeof(int) && f.Name.Contains("TopGuildLevel"))
            .ToArray();
        if (fields.Length != 1)
            throw new InvalidOperationException("TopGuildLevel storage field not found (matches: " + fields.Length + ")");
        fields[0].SetValue(who, top);
        if (who.TopGuildLevel != top)
            throw new InvalidOperationException("TopGuildLevel did not read back the value set");
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