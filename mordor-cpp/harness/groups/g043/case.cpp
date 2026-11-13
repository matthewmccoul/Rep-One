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
