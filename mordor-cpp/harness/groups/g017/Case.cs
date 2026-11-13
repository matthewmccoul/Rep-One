using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int id = a.GetProperty("id").GetInt32();
        var stats = new int[6];
        for (int i = 0; i < 6; i++)
            stats[i] = a.GetProperty("s" + i).GetInt32();

        // Absent record: same canonical text the C++ side emits when findItem fails.
        if (!H.ItemsById.ContainsKey((short)id)) return "MISSING_ITEM";

        Item it = H.ItemsById[(short)id];

        // Same character the C++ side builds with createCharacter(ctx, c, Race::Human, Guild::Nomad, stats):
        // Human, starting guild Nomad, level 1, the same six stats.
        Character c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);

        // Non-null lookup so the hands and worn-count checks run, matching the C++ copies.
        string? why = c.EquipProblem(it, _ => null);
        return why ?? "null";
    }
}
