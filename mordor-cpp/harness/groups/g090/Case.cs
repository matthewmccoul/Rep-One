using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // Reference: Character.Pay(long cost). Costs are whole gold, matching the C++ units (gold x 10000).
    public static string Run(JsonElement a, Random rng)
    {
        long cost = a.GetProperty("cost").GetInt64();
        long onHand = a.GetProperty("onHand").GetInt64();
        long bank = a.GetProperty("bank").GetInt64();

        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        c.GoldOnHand = onHand;
        c.GoldInBank = bank;

        bool ok = c.Pay(cost);

        // GoldOnHand/GoldInBank are VbCurrency; read them with RoundToWhole() (as Pay does) to get whole gold.
        return string.Join("|", new[]
        {
            ok ? "1" : "0",
            H.F(c.GoldOnHand.RoundToWhole()),
            H.F(c.GoldInBank.RoundToWhole()),
        });
    }
}
