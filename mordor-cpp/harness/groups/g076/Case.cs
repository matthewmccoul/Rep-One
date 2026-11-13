using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int spellIndex = a.GetProperty("spellIndex").GetInt32();
        int buffer = SpellBuffers.ForSpell(spellIndex);
        return buffer.ToString(CultureInfo.InvariantCulture);
    }
}
