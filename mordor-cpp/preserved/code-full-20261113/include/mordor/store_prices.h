// General Store prices, exactly as the reference StoreStock computes them (PriceOf, SellPrice, IdentifyFee, RealignCost).
// Amounts are Currency (gold x 10000); the reference rounds each result to whole gold (half to even) first.
#pragma once
#include "mordor/game_data.h"
#include "mordor/state_item.h"
#include "mordor/types.h"

namespace mordor {

inline constexpr i32 kStoreShelfCap = 9;   // ShelfCap: the shelf holds at most nine of one item per alignment

// What the store charges for one item. sameOnShelf = number of that item (same id and alignment) on the shelf.
// Floor-0 items sell at the list price; an item with none on the shelf has no price (0).
Currency storeBuyPrice(const GameData& data, const Item& item, i32 sameOnShelf);

// What the store pays for an item the character holds. stockSame = shelf count of that item and alignment;
// anyAlignmentInStock = the store stocks that item in some alignment (a stock of 0 then counts as 1).
Currency storeSellPrice(const GameData& data, const Item& item, i32 stockSame, bool anyAlignmentInStock);

// The fee for one step of identification (0 when the item is completely identified).
Currency storeIdentifyFee(const GameData& data, const Item& item, i32 stockSame);

// The fee to change an item's alignment: 0.72 x price, doubled when crossing more than one step.
Currency storeRealignFee(const GameData& data, const Item& item, i32 alignmentSteps);

}  // namespace mordor
