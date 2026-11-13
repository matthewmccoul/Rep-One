using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int n = a.GetProperty("n").GetInt32();
        int idx = a.GetProperty("idx").GetInt32();
        int[] stats = new int[] { 10, 10, 10, 10, 10, 10 };

        Character[] members = new Character[n];
        for (int i = 0; i < n; i++)
            members[i] = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        Party party = new Party(members);

        // The reference removes by identity. An index names a member when in range;
        // an index with no member names a non-member, which Remove must refuse.
        Character target = (idx >= 0 && idx < n)
            ? party.Members[idx]
            : Character.Create("Stranger", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);

        int before = party.Size;
        party.Remove(target);
        int after = party.Size;
        int shrunk = after < before ? 1 : 0;
        return shrunk + "|" + after;
    }
}
