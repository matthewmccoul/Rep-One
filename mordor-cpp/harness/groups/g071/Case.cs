using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, System.Random rng)
    {
        // The rule is not returned by any shown method. Rules.DiesOfOldAge computes
        // pct internally but returns a bool. Raising.ComplicationChance(ageDays, L2) returns
        // ageDays/365.0/(L2/2.0)*100. With L2 = 2*L (L > 0) this is exactly the rule's
        // pct, with the same operation order and bit-identical doubles. For L <= 0 the
        // reference returns 0 for any argument <= 0, so L is passed through unchanged.
        // 2*L must not overflow, so the test lifespans stay <= 1073741823.
        int ageDays = a.GetProperty("ageDays").GetInt32();
        int lifespan = a.GetProperty("raceLifespanYears").GetInt32();
        int arg = lifespan <= 0 ? lifespan : 2 * lifespan;
        double pct = Raising.ComplicationChance(ageDays, arg);
        return H.F(pct);
    }
}
