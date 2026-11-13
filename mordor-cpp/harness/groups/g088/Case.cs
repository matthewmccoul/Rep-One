using System;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        int pl = a.GetProperty("paralysisLevel").GetInt32();
        int ad = a.GetProperty("adShift").GetInt32();
        var stats = new int[6];
        for (int i = 0; i < 6; i++) stats[i] = a.GetProperty("stat" + i).GetInt32();

        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        c.ParalysisLevel = pl;
        c.ADShift = ad;

        // No items are worn; the rule takes no random draws, so rng is not passed.
        c.Unparalyse(_ => null);

        return $"{c.ADShift}|{c.ParalysisLevel}";
    }
}
