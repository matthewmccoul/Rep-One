public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        // Rule takes no Random; rng is unused.
        int tier = a.GetProperty("tier").GetInt32();
        int band = Mordor.Core.Rules.QuestItemBand(tier);
        return band.ToString(System.Globalization.CultureInfo.InvariantCulture);
    }
}
