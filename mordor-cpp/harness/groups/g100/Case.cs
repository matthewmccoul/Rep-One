using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // R21 is a pure function of guildLevel; the Random argument is not used by the reference.
    public static string Run(JsonElement a, Random rng)
    {
        int guildLevel = a.GetProperty("guildLevel").GetInt32();
        int sl = Rules.SpellLevel(guildLevel);
        return sl.ToString(CultureInfo.InvariantCulture);
    }
}
