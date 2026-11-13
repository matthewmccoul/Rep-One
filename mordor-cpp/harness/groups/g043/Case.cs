using System;
using System.Linq;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // mode 0: first table item with Floor == 0; mode 1: first with Floor > 0; mode 2: item by id.
    public static string Run(JsonElement a, Random rng)
    {
        int mode = a.GetProperty("mode").GetInt32();
        int count = a.GetProperty("count").GetInt32();

        Item it = mode switch
        {
            0 => H.Items.First(x => x.Floor == 0),
            1 => H.Items.First(x => x.Floor > 0),
            _ => H.ItemsById[(short)a.GetProperty("id").GetInt32()],
        };

        // Fill the shelf for one alignment with 'count' units (Add is the public way to set Count).
        var stock = new StoreStock();
        for (int i = 0; i < count; i++) stock.Add(it.Id, ItemAlignment.Neutral);

        long price = stock.PriceOf(it, ItemAlignment.Neutral);
        return H.F(price);
    }
}
