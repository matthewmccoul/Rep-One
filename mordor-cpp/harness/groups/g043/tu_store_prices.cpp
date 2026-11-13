// General Store prices from the reference StoreStock.cs (PriceOf, SellPrice, IdentifyFee, RealignCost).
#include "mordor/store_prices.h"

#include <cmath>

#include "mordor/math_util.h"

namespace mordor {

namespace {

constexpr Currency kPerGold = 10000;

const ItemDef* itemDefById(const GameData& data, i32 id) {
    for (const ItemDef& d : data.items) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

// The reference counts identification down: 0 is completely identified, 3 barely. Ours has an extra Unknown
// at the start (Unknown, Barely, Partial, Mostly, All), so Unknown and Barely both count as 3.
i32 idCountdown(IdLevel level) {
    const i32 c = 4 - static_cast<i32>(level);
    return c > 3 ? 3 : (c < 0 ? 0 : c);
}

// Potions, scrolls and the rest: sold a charge-chunk at a time (StoreStock.SoldByCharge).
bool soldByCharge(const ItemDef& d) {
    if (d.charges <= 0) return false;
    switch (d.type) {
        case 23: case 24: case 25: case 26: case 27: case 28: case 30: case 31: return true;
        default: return false;
    }
}

i32 capShelf(i32 n) { return n > kStoreShelfCap ? kStoreShelfCap : n; }

}  // namespace

Currency storeBuyPrice(const GameData& data, const Item& item, i32 sameOnShelf) {
    const ItemDef* d = itemDefById(data, item.id);
    if (d == nullptr) return 0;
    if (d->floor == 0) return static_cast<Currency>(d->price) * kPerGold;
    if (sameOnShelf <= 0) return 0;
    const i32 shelf = capShelf(sameOnShelf - 1);
    const double price = static_cast<double>(d->price);
    const double cost = (1.5 * price) / (std::log(shelf + 1.0) + 1.15) - std::log(d->charges + 1.0) * (0.05 * price);
    return static_cast<Currency>(roundHalfEven(cost)) * kPerGold;
}

Currency storeSellPrice(const GameData& data, const Item& item, i32 stockSame, bool anyAlignmentInStock) {
    const ItemDef* d = itemDefById(data, item.id);
    if (d == nullptr) return 0;
    const double price = (d->cursed >= 1 && item.equipped) ? 0.0 : static_cast<double>(d->price);
    i32 stock = stockSame;
    if (stock == 0 && anyAlignmentInStock) stock = 1;
    stock = capShelf(stock);
    i32 charges = item.charges;
    double chargeFactor = 1.0;
    if (soldByCharge(*d)) {
        chargeFactor = 5.0;
        if (item.charges > d->charges) charges = d->charges;
    }
    chargeFactor *= std::log(std::abs(charges - d->charges) + 1.0);
    const i32 id = idCountdown(item.idLevel);
    if (id == 0 && d->cursed != 0) return 0;
    const i64 based = roundHalfEven(((0.9 * price) / (stock + 1.15)) / (id + 1));
    const i64 sold = roundHalfEven(static_cast<double>(based) - chargeFactor * (0.05 * static_cast<double>(based)));
    return static_cast<Currency>(sold) * kPerGold;
}

Currency storeIdentifyFee(const GameData& data, const Item& item, i32 stockSame) {
    const ItemDef* d = itemDefById(data, item.id);
    if (d == nullptr) return 0;
    const i32 stock = capShelf(stockSame);
    const i32 id = idCountdown(item.idLevel);
    if (id <= 0) return 0;
    const i64 fee = roundHalfEven((0.23 * static_cast<double>(d->price)) / ((stock / 2.0 + 1.0) * 0.9) / id + 1.0);
    return static_cast<Currency>(fee) * kPerGold;
}

Currency storeRealignFee(const GameData& data, const Item& item, i32 alignmentSteps) {
    const ItemDef* d = itemDefById(data, item.id);
    if (d == nullptr) return 0;
    const i32 steps = alignmentSteps < 0 ? -alignmentSteps : alignmentSteps;
    const i64 fee = roundHalfEven(0.72 * static_cast<double>(d->price) * (steps > 1 ? 2 : 1));
    return static_cast<Currency>(fee) * kPerGold;
}

}  // namespace mordor
