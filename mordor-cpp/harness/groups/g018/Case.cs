using System;
using System.Globalization;
using System.Linq;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        long giverStart = a.GetProperty("giver_gold").GetInt64();
        long receiverStart = a.GetProperty("receiver_gold").GetInt64();
        long amount = a.GetProperty("amount").GetInt64();

        var giver = Character.Create("Giver", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        var receiver = Character.Create("Receiver", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        SetGold(giver, giverStart);
        SetGold(receiver, receiverStart);

        var said = GiveCommand.Give(giver, receiver, 0, amount, _ => null);
        int ret = said.Count > 0 ? 1 : 0;

        return ret.ToString(CultureInfo.InvariantCulture) + "|"
             + WholeGold(giver.GoldOnHand).ToString(CultureInfo.InvariantCulture) + "|"
             + WholeGold(receiver.GoldOnHand).ToString(CultureInfo.InvariantCulture);
    }

    // Core's Character.Create sets GoldOnHand = 1500 and SetGold adds to it.
    // Remove the starting 1500 first so the balance equals the case value exactly.
    static void SetGold(Character c, long gold)
    {
        if (!VbCurrency.TrySubtract(c.GoldOnHand, 1500L, out var zero))
            throw new InvalidOperationException("setup reset failed");
        c.GoldOnHand = zero;

        if (gold <= 0) return;
        if (!VbCurrency.TryAdd(c.GoldOnHand, gold, out var v))
            throw new InvalidOperationException("setup overflow");
        c.GoldOnHand = v;
    }

    static long WholeGold(VbCurrency v)
    {
        string digits = new string(v.Str().Where(ch => char.IsDigit(ch) || ch == '.' || ch == '-').ToArray());
        return (long)Math.Floor(decimal.Parse(digits, NumberStyles.Float, CultureInfo.InvariantCulture));
    }
}
