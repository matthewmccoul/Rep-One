using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int baseDmg = a.GetProperty("base").GetInt32();
        int power = a.GetProperty("power").GetInt32();
        int sl = a.GetProperty("sl").GetInt32();
        int resist = a.GetProperty("resist").GetInt32();
        bool hasResist = a.GetProperty("has_resist").GetInt32() != 0;
        // has_resist = 0 means no resistance is applied; that is the same as resistance 0 in the reference.
        int resistEff = hasResist ? resist : 0;
        int dmg = Rules.SpellDamage(rng, baseDmg, power, sl, resistEff);
        return dmg.ToString(CultureInfo.InvariantCulture);
    }
}
