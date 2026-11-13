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

        // Stats in STR, INT, WIS, CON, CHA, DEX order, matching the C++ side.
        int[] stats = new int[]
        {
            a.GetProperty("stat_str").GetInt32(),
            a.GetProperty("stat_int").GetInt32(),
            a.GetProperty("stat_wis").GetInt32(),
            a.GetProperty("stat_con").GetInt32(),
            a.GetProperty("stat_cha").GetInt32(),
            a.GetProperty("stat_dex").GetInt32(),
        };

        // Same race, alignment, sex and starting guild as the C++ createCharacter call.
        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        c.ParalysisLevel = priorLevel;
        c.ADShift = priorAd;

        // No item lookup: all items absent on both sides.
        c.Paralyse(level, _ => null);

        // Paralyse returns void; state fields changed by the rule: ParalysisLevel, ADShift.
        return $"{c.ParalysisLevel}|{c.ADShift}";
    }
}

