using System;
using System.Collections.Generic;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int trap = a.GetProperty("trap").GetInt32();
        int level = a.GetProperty("level").GetInt32();
        int opener = a.GetProperty("opener").GetInt32();
        int count = a.GetProperty("count").GetInt32();

        var party = new List<Character>();
        for (int i = 0; i < count; i++)
        {
            var c = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
            c.MaxHits = a.GetProperty("mh" + i).GetInt32();
            c.CurHits = a.GetProperty("h" + i).GetInt32();
            c.MaxSpellPoints = a.GetProperty("ms" + i).GetInt32();
            c.CurSpellPoints = a.GetProperty("s" + i).GetInt32();
            party.Add(c);
        }

        var who = party[opener];
        var r = ChestTraps.Spring(trap, party, who, level, rng, _ => null, (x, y) => 0);

        // Canonical: poisoned|diseased|itemsDestroyed|hits per member|spell points per member.
        // The reference TrapResult has no damage field, so damage is compared through hits.
        var fields = new List<string>
        {
            who.Poisoned ? "1" : "0",
            who.Diseased ? "1" : "0",
            r.Lines.Count > 0 ? "1" : "0",
        };
        foreach (var c in party) fields.Add(c.CurHits.ToString());
        foreach (var c in party) fields.Add(c.CurSpellPoints.ToString());
        return string.Join("|", fields);
    }
}
