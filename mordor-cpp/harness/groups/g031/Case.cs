using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int curHits = a.GetProperty("curHits").GetInt32();
        int maxHits = a.GetProperty("maxHits").GetInt32();
        int age = a.GetProperty("age").GetInt32();
        // partyMany is not a parameter of the reference; it is read by the copies only.

        var stats = new int[] { 10, 10, 10, 10, 10, 10 };
        var ch = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        ch.MaxHits = maxHits;
        ch.CurHits = curHits;
        ch.Age = age;

        int days = ch.RestInCity();

        // Only the age cost is compared: return value and Age.
        return $"{days}|{ch.Age}";
    }
}
