using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int intel = a.GetProperty("int").GetInt32();
        int wis = a.GetProperty("wis").GetInt32();

        var stats = new int[6];
        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        // Set natural stats directly so race caps do not alter the inputs.
        c.Stats[(int)Stat.Int] = intel;
        c.Stats[(int)Stat.Wis] = wis;

        int result = Spellbook.MaxSpellPoints(c);
        return result.ToString(CultureInfo.InvariantCulture);
    }
}
