=== FILE: cases.json ===
[
{"args":{"mode":0,"count":0},"rng":[0.5]},
{"args":{"mode":0,"count":1},"rng":[0.5]},
{"args":{"mode":0,"count":5},"rng":[0.5]},
{"args":{"mode":0,"count":-1},"rng":[0.5]},
{"args":{"mode":0,"count":12},"rng":[0.5]},
{"args":{"mode":1,"count":0},"rng":[0.5]},
{"args":{"mode":1,"count":-1},"rng":[0.5]},
{"args":{"mode":1,"count":1},"rng":[0.5]},
{"args":{"mode":1,"count":2},"rng":[0.5]},
{"args":{"mode":1,"count":5},"rng":[0.5]},
{"args":{"mode":1,"count":9},"rng":[0.5]},
{"args":{"mode":1,"count":10},"rng":[0.5]},
{"args":{"mode":1,"count":11},"rng":[0.5]},
{"args":{"mode":1,"count":12},"rng":[0.5]},
{"args":{"mode":1,"count":40},"rng":[0.5]},
{"args":{"mode":1,"count":1000},"rng":[0.5]},
{"args":{"mode":2,"id":1,"count":1},"rng":[0.5]},
{"args":{"mode":2,"id":1,"count":10},"rng":[0.5]},
{"args":{"mode":2,"id":10,"count":3},"rng":[0.5]},
{"args":{"mode":2,"id":50,"count":7},"rng":[0.5]},
{"args":{"mode":2,"id":100,"count":11},"rng":[0.5]},
{"args":{"mode":2,"id":150,"count":2},"rng":[0.5]},
{"args":{"mode":2,"id":200,"count":9},"rng":[0.5]},
{"args":{"mode":2,"id":250,"count":0},"rng":[0.5]},
{"args":{"mode":2,"id":250,"count":30},"rng":[0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
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
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_item.h"

namespace mordor {

// Same-name copies: renamed at compile time by the harness.
Currency storeBuyPrice__loot1a(i32 floor, i32 shelfCount, Currency price, i32 charges);
Currency storeBuyPrice__store_prices(const GameData& data, const Item& item, i32 sameOnShelf);

// Currency = gold x 10000.
static const Currency kGoldUnit = 10000;

// mode 0: first item with floor 0; mode 1: first with floor > 0; mode 2: by id.
static const ItemDef* pickItem(const JsonValue& a, Ctx& ctx) {
    const i32 mode = A(a, "mode").asInt();
    for (const ItemDef& d : ctx.data.items) {
        if (mode == 0 && d.floor == 0) return &d;
        if (mode == 1 && d.floor > 0) return &d;
    }
    if (mode == 2) {
        const i32 id = A(a, "id").asInt();
        for (const ItemDef& d : ctx.data.items) {
            if (d.id == id) return &d;
        }
    }
    return nullptr;
}

static Str run_storeBuyPrice_loot1a(const JsonValue& a, Ctx& ctx) {
    const ItemDef* d = pickItem(a, ctx);
    if (d == nullptr) return Str("NOITEM");
    const i32 count = A(a, "count").asInt();
    // Copy takes price in Currency units and charges from the record.
    const Currency r = storeBuyPrice__loot1a(d->floor, count,
                                             static_cast<Currency>(d->price) * kGoldUnit,
                                             d->charges);
    return F(static_cast<double>(r) / 10000.0);
}

static Str run_storeBuyPrice_store_prices(const JsonValue& a, Ctx& ctx) {
    const ItemDef* d = pickItem(a, ctx);
    if (d == nullptr) return Str("NOITEM");
    const i32 count = A(a, "count").asInt();
    Item item;
    item.id = d->id;
    const Currency r = storeBuyPrice__store_prices(ctx.data, item, count);
    return F(static_cast<double>(r) / 10000.0);
}

const CopyEntry kCopies[] = {
    {"storeBuyPrice@loot1a", &run_storeBuyPrice_loot1a},
    {"storeBuyPrice@store_prices", &run_storeBuyPrice_store_prices},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

Notes:
- Both copies return Currency; results are divided by 10000 so they print in whole gold, matching the reference's long gold output. The loot1a copy rounds at Currency precision (1/10000 gold), while the reference rounds to whole gold. Any non-integer-gold result from that copy will therefore show as a mismatch, and that is intended.
- Item selection uses table order (first floor-0 or first floor>0 record) on both sides. Charges and price come from the table record, so the charges branch is exercised only through the records chosen. The cases assume ids 1, 10, 50, 100, 150, 200 and 250 exist in the shareware table.
- The case uses `new StoreStock()` and `Add`, which are not in the outline. If the class has no accessible parameterless constructor, the C# side will fail to compile.
- Neither copy needs a shim because both are non-static.

REFERENCE: StoreStock.PriceOf