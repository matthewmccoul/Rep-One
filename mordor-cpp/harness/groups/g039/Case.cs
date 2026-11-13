using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int n = Get(a, "n");
        var list = new List<Character>();
        for (int i = 0; i < n; i++)
        {
            var c = NewCharacter(((char)('A' + i)).ToString());
            c.CurHits = Get(a, "h" + i);
            c.DungeonLevel = Get(a, "l" + i);
            c.X = Get(a, "x" + i);
            c.Y = Get(a, "y" + i);
            list.Add(c);
        }
        var party = new Party(list.ToArray());
        // SelectedIndex has a private setter; set it through the existing setter.
        typeof(Party).GetProperty("SelectedIndex")!.GetSetMethod(true)!.Invoke(party, new object[] { Get(a, "sel") });

        party.RemoveTheDead(); // returned messages and rng are not part of the canonical string

        return Dump(party);
    }

    // Character.Create's stats argument must be a real instance, not null (passing default
    // for a reference type gives null, which is a likely source of the NullReferenceException).
    // Build it reflectively from the 5-parameter Create overload.
    static Character NewCharacter(string name)
    {
        var create = typeof(Character)
            .GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static)
            .First(m => m.Name == "Create" && m.GetParameters().Length == 5);
        object stats = Activator.CreateInstance(create.GetParameters()[4].ParameterType)!;
        return (Character)create.Invoke(null, new object[]
        {
            name, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats
        })!;
    }

    static int Get(JsonElement a, string key) => a.TryGetProperty(key, out var v) ? v.GetInt32() : 0;

    static string Dump(Party p)
    {
        var sb = new StringBuilder();
        sb.Append(p.Size).Append('|').Append(p.SelectedIndex);
        foreach (var c in p.Members)
        {
            sb.Append('|').Append(c.Name).Append('|').Append(c.CurHits)
              .Append('|').Append(c.IsDead ? 1 : 0)
              .Append('|').Append(c.Stoned ? 1 : 0)
              .Append('|').Append(c.DungeonLevel)
              .Append('|').Append(c.X).Append('|').Append(c.Y);
        }
        return sb.ToString();
    }
}
