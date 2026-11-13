using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // UNKNOWN: unit of the implicit int conversion. Guarded by the probe in Run.
    static VbCurrency Amt(int units) => units;

    // Uses the shown Str() readback pattern (BankAllText).
    static string Fmt(VbCurrency v) => v.Str().TrimStart(' ');

    // Builds start doubled n times with TryAdd; false on overflow (input-side).
    static bool Build(VbCurrency start, int doublings, out VbCurrency v)
    {
        v = start;
        for (int k = 0; k < doublings; k++)
        {
            if (!VbCurrency.TryAdd(v, v, out var s)) return false;
            v = s;
        }
        return true;
    }

    public static string Run(JsonElement a, Random rng)
    {
        string p1 = Fmt(Amt(12345)), p0 = Fmt(Amt(0)), pn = Fmt(Amt(-5));
        if (p1 != "1.2345" || p0 != "0.0000" || pn != "-0.0005")
            throw new InvalidOperationException(
                "UNIT/FORMAT ASSUMPTION FAILED: Amt(12345)='" + p1 + "' Amt(0)='" + p0 + "' Amt(-5)='" + pn + "'");

        int n = a.GetProperty("n").GetInt32();
        var members = new List<Character>();
        for (int i = 0; i < n; i++)
        {
            if (!Build(Amt(a.GetProperty($"b{i}").GetInt32()), a.GetProperty($"bd{i}").GetInt32(), out VbCurrency bank))
                return "INPUT_OVERFLOW";
            if (!Build(Amt(a.GetProperty($"h{i}").GetInt32()), a.GetProperty($"hd{i}").GetInt32(), out VbCurrency hand))
                return "INPUT_OVERFLOW";

            var c = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male,
                                     new int[] { 10, 10, 10, 10, 10, 10 });
            c.GoldInBank = bank;
            c.GoldOnHand = hand;
            members.Add(c);
        }

        var party = new Party(members.ToArray());
        VbCurrency moved = Party.PartyDeposit(party.Members);

        var sb = new StringBuilder();
        sb.Append(Fmt(moved));
        foreach (var c in members)
            sb.Append('|').Append(Fmt(c.GoldInBank)).Append('|').Append(Fmt(c.GoldOnHand));
        return sb.ToString();
    }
}
