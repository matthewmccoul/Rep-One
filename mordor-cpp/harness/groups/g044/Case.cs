using System;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        int level = a.GetProperty("level").GetInt32();
        int headerAreas = a.GetProperty("headerAreas").GetInt32();
        int tally = a.GetProperty("tally").GetInt32();

        // Pure function: no Random draws and no state fields are changed.
        TimeSpan d = DungeonState.RespawnDelay(level, headerAreas, tally);
        return H.F(d.TotalSeconds);
    }
}
