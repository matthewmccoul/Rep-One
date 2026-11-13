using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        var stats = new int[6];
        var c = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        c.MaxSpellPoints = a.GetProperty("prevMax").GetInt32();
        c.CurSpellPoints = a.GetProperty("cur").GetInt32();
        bool refill = a.GetProperty("refill").GetInt32() != 0;
        c.RecalcSpellPoints(refill);
        return c.MaxSpellPoints.ToString(System.Globalization.CultureInfo.InvariantCulture)
             + "|" + c.CurSpellPoints.ToString(System.Globalization.CultureInfo.InvariantCulture);
    }
}
