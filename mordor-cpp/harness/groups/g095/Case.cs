using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        // Pure function of the type code; rng is not consumed by this rule.
        int t = a.GetProperty("type").GetInt32();
        var it = new Item { Type = (short)t };
        bool r = ItemUse.MustBeEquippedToUse(it);
        return (r ? 1 : 0).ToString();
    }
}
