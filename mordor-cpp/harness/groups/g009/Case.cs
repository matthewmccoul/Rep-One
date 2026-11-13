using System;
using System.Linq;
using System.Reflection;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int guild = a.GetProperty("guild_level").GetInt32();
        int top = a.GetProperty("top_guild_level").GetInt32();
        int binding = a.GetProperty("binding_level").GetInt32();

        var who = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        SetTopGuildLevel(who, top);

        var def = new Monster { GuildLevel = checked((short)guild) };
        var c = new Companion { BindingLevel = binding };

        double drift = CompanionMood.Drift(who, c, def);
        return H.F(drift);
    }

    // TopGuildLevel is a read-only property. Locate the existing int storage field
    // whose name contains "TopGuildLevel" and write to it. Fail if it is not found
    // or if the property does not read back the value written.
    static void SetTopGuildLevel(Character who, int top)
    {
        var fields = typeof(Character)
            .GetFields(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public)
            .Where(f => f.FieldType == typeof(int) && f.Name.Contains("TopGuildLevel"))
            .ToArray();
        if (fields.Length != 1)
            throw new InvalidOperationException("TopGuildLevel storage field not found (matches: " + fields.Length + ")");
        fields[0].SetValue(who, top);
        if (who.TopGuildLevel != top)
            throw new InvalidOperationException("TopGuildLevel did not read back the value set");
    }
}
