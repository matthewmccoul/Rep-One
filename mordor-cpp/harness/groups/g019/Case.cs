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
