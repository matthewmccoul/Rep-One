using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        // Race is selected by its index in RaceData.All (assumed to match the C++ Race enum value).
        var race = RaceData.All[a.GetProperty("race").GetInt32()];
        int[] stats =
        {
            a.GetProperty("STR").GetInt32(),
            a.GetProperty("INT").GetInt32(),
            a.GetProperty("WIS").GetInt32(),
            a.GetProperty("CON").GetInt32(),
            a.GetProperty("CHA").GetInt32(),
            a.GetProperty("DEX").GetInt32(),
        };

        // Reference R5 creation. Name, sex and alignment are not part of the compared R5 state.
        var c = Character.Create("Test", race, Alignment.Neutral, Sex.Male, stats);

        // Character.Race holds the race name; recover its index in RaceData.All.
        int raceNum = -1;
        for (int i = 0; i < RaceData.All.Count; i++)
        {
            if (RaceData.All[i].Name == c.Race)
            {
                raceNum = i;
                break;
            }
        }

        // GoldOnHand is a VbCurrency (no implicit conversion to double), so go through its text form.
        // NumberStyles.Any tolerates thousands separators and currency symbols.
        double gold = double.Parse(c.GoldOnHand.ToString(), NumberStyles.Any, CultureInfo.InvariantCulture);

        var p = new List<string>();
        p.Add(raceNum.ToString());
        foreach (var s in c.Stats) p.Add(s.ToString());
        foreach (var s in c.StatMax) p.Add(s.ToString());
        p.Add(c.MaxHits.ToString());
        p.Add(c.CurHits.ToString());
        p.Add(H.F(gold));
        p.Add(c.Area.ToString());
        p.Add(c.Guilds.Count.ToString());
        p.Add(c.Guilds[0].Level.ToString());
        p.Add(c.Guilds[0].Experience.ToString());
        return string.Join("|", p);
    }
}
