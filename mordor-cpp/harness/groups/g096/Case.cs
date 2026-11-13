using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        bool lostLevel = a.GetProperty("lostLevel").GetBoolean();
        bool lostPosition = a.GetProperty("lostPosition").GetBoolean();
        int level = a.GetProperty("level").GetInt32();
        int x = a.GetProperty("x").GetInt32();
        int y = a.GetProperty("y").GetInt32();

        string? place = Librarian.PlaceOf(lostLevel, lostPosition, level, x, y);
        // Pure function: no state fields change. Return value is ok flag plus place string (or "null").
        return $"{(place != null ? 1 : 0)}|{place ?? "null"}";
    }
}
