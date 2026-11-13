using System;
using System.Linq;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, Random rng)
    {
        int hits = a.GetProperty("hits").GetInt32();
        int maxHits = a.GetProperty("max_hits").GetInt32();
        int binding = a.GetProperty("binding").GetInt32();
        int monsterId = a.GetProperty("monster_id").GetInt32();

        var who = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        var c = new Companion { Hits = hits, MaxHits = maxHits, BindingLevel = binding };
        var def = H.Monsters.First(m => m.Id == monsterId);

        // Only the Binding < 0 branch is under test; rng is unused by this rule.
        return H.F(CompanionMood.Drift(who, c, def));
    }
}
