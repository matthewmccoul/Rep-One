using System;
using System.Text.Json;
using System.Globalization;
using Mordor.Core;

public static class Case
{
    // Pack occupancy is given as two bitmasks: bit k of "lo" = pack slot k+1 (slots 1..20),
    // bit k of "hi" = pack slot k+21 (slots 21..40). Slot 0 (Hands) is never stored.
    // FirstFreeSlot consumes no random draws, so rng is unused.
    public static string Run(JsonElement a, System.Random rng)
    {
        var c = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);

        long lo = a.GetProperty("lo").GetInt64();
        long hi = a.GetProperty("hi").GetInt64();
        for (int k = 0; k < 20; k++)
        {
            if (((lo >> k) & 1) != 0) c.PutInSlot(k + 1, new InventorySlot { ItemId = 1 });
            if (((hi >> k) & 1) != 0) c.PutInSlot(k + 21, new InventorySlot { ItemId = 1 });
        }

        int r = c.FirstFreeSlot();
        return r.ToString(CultureInfo.InvariantCulture);
    }
}
