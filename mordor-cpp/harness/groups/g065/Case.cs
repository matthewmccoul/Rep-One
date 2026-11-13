using System;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        // Chance draws no random numbers; rng is unused.
        var kind = (CompanionMishap.Kind)a.GetProperty("kind").GetInt32();
        int guildLevel = a.GetProperty("guildLevel").GetInt32();
        int bodies = a.GetProperty("bodies").GetInt32();
        return H.F(CompanionMishap.Chance(kind, guildLevel, bodies));
    }
}
