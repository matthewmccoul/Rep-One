using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // Causes: 0 = Stoned, 1 = Rock, 2 = ordinary (Rules.DeathOrdinary), 3 = any other int
    // that is neither Stoned nor Rock. The rule consumes no random draws.
    public static string Run(JsonElement a, Random rng)
    {
        int code = a.GetProperty("cause").GetInt32();
        int cause = code switch
        {
            0 => Rules.DeathStoned,
            1 => Rules.DeathRock,
            2 => Rules.DeathOrdinary,
            _ => OtherCause(),
        };
        int age = Raising.AgeDaysFor(cause);
        return age.ToString(CultureInfo.InvariantCulture);
    }

    private static int OtherCause()
    {
        int v = 0;
        while (v == Rules.DeathStoned || v == Rules.DeathRock) v++;
        return v;
    }
}
