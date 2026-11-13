using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        long flags = a.GetProperty("flags").GetInt64();
        int bit = a.GetProperty("bit").GetInt32();
        var m = new Monster { SpecialPropertyFlags = unchecked((int)(uint)flags) };
        bool r = MonsterProperty.Has(m, bit);
        return r ? "1" : "0";
    }
}
