#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct StoreStock {
    cs::Dictionary<cs::String, int32_t> Shelf = cs::Dictionary<cs::String, int32_t>::New();

    static cs::String Key(int16_t id, ItemAlignment a);
    int32_t Count(int16_t id, ItemAlignment a);
    cs::List<int32_t> Counts(int16_t id);
    bool AnyInStock(int16_t id);
    void Add(int16_t id, ItemAlignment a);
    bool Take(int16_t id, ItemAlignment a);
    int64_t PriceOf(cs::Ref<Item> it, ItemAlignment a);

    static constexpr int32_t ShelfCap = 9;

    static bool SoldByCharge(cs::Ref<Item> it);
    static int32_t IdCountdown(IdLevel known);
    int64_t SellPrice(cs::Ref<Item> it, cs::Ref<InventorySlot> slot);
    int64_t IdentifyFee(cs::Ref<Item> it, cs::Ref<InventorySlot> slot);
    static int64_t RealignCost(cs::Ref<Item> it, ItemAlignment from, ItemAlignment to);

    static constexpr int32_t PackSlots = 39;

    static bool DepletedByBuying(cs::Ref<Item> it, ItemAlignment bought);
    static bool ShelvedBySelling(cs::Ref<Item> it, ItemAlignment a);
    bool ShelvesSale(cs::Ref<Item> it, ItemAlignment a);
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<StoreStock> Load(cs::String savesDir, cs::List<cs::Ref<Item>> items);
    static cs::Ref<StoreStock> Load(cs::String savesDir);
    void Save(cs::String savesDir);
    static cs::Ref<StoreStock> CreateInitial(cs::List<cs::Ref<Item>> items);

    static constexpr int32_t DeepestStocked = 14;

    void Replenish(cs::List<cs::Ref<Item>> items, int32_t days, cs::Ref<cs::Random> rng, int32_t deepestReached = DeepestStocked,
                   cs::Func<void(cs::Ref<Item>)> shelved = {});
    static int32_t DrawOne(cs::List<cs::Ref<Item>> items, int32_t kinds, int32_t floor, cs::Ref<cs::Random> rng, int32_t& last);
    static int32_t Weight(int32_t rarity, float total);
    static bool Reachable(cs::List<cs::Ref<Item>> items, int32_t kinds, int32_t floor);
    bool AddOne(cs::Ref<Item> it, cs::Ref<cs::Random> rng);
    bool RemoveOne(cs::Ref<Item> it, cs::Ref<cs::Random> rng);
};

}  // namespace mc
