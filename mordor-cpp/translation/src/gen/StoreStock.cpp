#include "core.h"

namespace mc {

cs::String StoreStock::Key(int16_t id, ItemAlignment a) {
    return cs::ToStr(id) + ":" + cs::ToStr(static_cast<int32_t>(a));
}

int32_t StoreStock::Count(int16_t id, ItemAlignment a) {
    int32_t n{};
    return Shelf.TryGetValue(Key(id, a), n) ? n : 0;
}

cs::List<int32_t> StoreStock::Counts(int16_t id) {
    return ItemAlignments::All.Select([&](auto& a) { return Count(id, a); }).ToArray();
}

bool StoreStock::AnyInStock(int16_t id) {
    return ItemAlignments::All.Any([&](auto& a) { return Count(id, a) > 0; });
}

void StoreStock::Add(int16_t id, ItemAlignment a) {
    Shelf[Key(id, a)] = Count(id, a) + 1;
}

bool StoreStock::Take(int16_t id, ItemAlignment a) {
    int32_t n = Count(id, a);
    if (n <= 0) return false;
    if (n == 1) Shelf.Remove(Key(id, a)); else Shelf[Key(id, a)] = n - 1;
    return true;
}

int64_t StoreStock::PriceOf(cs::Ref<Item> it, ItemAlignment a) {
    // FRMCTYS.BAS:594-597 tests the floor alone
    if (it->Floor == 0) return it->Price;
    // an empty shelf takes Log(0) and the error path leaves the price 0 (FRMCTYS.BAS:600-613)
    if (Count(it->Id, a) <= 0) return 0;
    int32_t shelf = cs::Math::Min(Count(it->Id, a) - 1, ShelfCap);
    double cost = (1.5 * it->Price) / (cs::Math::Log(shelf + 1) + 1.15)
                - cs::Math::Log(it->Charges + 1) * (0.05 * it->Price);
    return cs::CastDouble<int64_t>(cs::Math::Round(cost, cs::MidpointRounding::ToEven));   // modItemCost As Long (FRMCTYS.BAS:587)
}

bool StoreStock::SoldByCharge(cs::Ref<Item> it) {
    int16_t t = it->Type;
    return it->Charges > 0 && (t == 23 || t == 24 || t == 25 || t == 26 || t == 27 || t == 28 || t == 30 || t == 31);
}

int32_t StoreStock::IdCountdown(IdLevel known) {
    return 3 - static_cast<int32_t>(known);
}

int64_t StoreStock::SellPrice(cs::Ref<Item> it, cs::Ref<InventorySlot> slot) {
    double price = it->Cursed >= 1 && slot->Equipped ? 0.0 : (double)it->Price;

    int32_t stock = Count(it->Id, slot->Alignment);
    if (stock == 0 && AnyInStock(it->Id)) stock = 1;
    stock = cs::Math::Min(stock, ShelfCap);

    int32_t charges = slot->Charges;
    double chargeFactor = 1;
    if (SoldByCharge(it)) {
        chargeFactor = 5;
        if (slot->Charges > it->Charges) charges = it->Charges;
    }
    chargeFactor *= cs::Math::Log(cs::Math::Abs(charges - it->Charges) + 1);

    int32_t id = IdCountdown(slot->Known());
    if (id == 0 && it->Cursed != 0) return 0;
    // l023A and sellPrice are both Longs (FRMCTYS.BAS:699, 708): each step rounds, and nothing floors it
    int64_t based = cs::CastDouble<int64_t>(cs::Math::Round(((0.9 * price) / (stock + 1.15)) / (id + 1), cs::MidpointRounding::ToEven));
    return cs::CastDouble<int64_t>(cs::Math::Round(based - chargeFactor * (0.05 * based), cs::MidpointRounding::ToEven));
}

int64_t StoreStock::IdentifyFee(cs::Ref<Item> it, cs::Ref<InventorySlot> slot) {
    int32_t stock = cs::Math::Min(Count(it->Id, slot->Alignment), ShelfCap);
    int32_t id = IdCountdown(slot->Known());
    if (id <= 0) return 0;
    return cs::CastDouble<int64_t>(cs::Math::Round((0.23 * it->Price) / ((stock / 2.0 + 1) * 0.9) / id + 1, cs::MidpointRounding::ToEven));   // identifyFee As Long (568)
}

int64_t StoreStock::RealignCost(cs::Ref<Item> it, ItemAlignment from, ItemAlignment to) {
    // l011A is a Long: the Double is rounded into it, half to even, not truncated
    return cs::CastDouble<int64_t>(cs::Math::Round(0.72 * it->Price * (cs::Math::Abs(static_cast<int32_t>(from) - static_cast<int32_t>(to)) > 1 ? 2 : 1), cs::MidpointRounding::ToEven));
}

bool StoreStock::DepletedByBuying(cs::Ref<Item> it, ItemAlignment bought) {
    return it->Floor > 0 || bought != ItemAlignment::Unaligned;
}

bool StoreStock::ShelvedBySelling(cs::Ref<Item> it, ItemAlignment a) {
    return !(it->Floor == 0 && a == ItemAlignment::Unaligned);
}

bool StoreStock::ShelvesSale(cs::Ref<Item> it, ItemAlignment a) {
    return it->Cursed == 0 && ShelvedBySelling(it, a) && Count(it->Id, a) < ShelfCap;
}

cs::String StoreStock::PathFor(cs::String savesDir) {
    return cs::Path::Combine(savesDir, "store.json");
}

cs::Ref<StoreStock> StoreStock::Load(cs::String savesDir, cs::List<cs::Ref<Item>> items) {
    cs::String p = PathFor(savesDir);
    if (!cs::File::Exists(p) && items != nullptr) {
        auto fresh = CreateInitial(items);
        fresh->Save(savesDir);
        return fresh;
    }
    return Load(savesDir);
}

cs::Ref<StoreStock> StoreStock::Load(cs::String savesDir) {
    (void)savesDir;
    throw cs::NotSupportedException("not translated: StoreStock JSON deserialization");
}

void StoreStock::Save(cs::String savesDir) {
    (void)savesDir;
    throw cs::NotSupportedException("not translated: StoreStock JSON serialization");
}

cs::Ref<StoreStock> StoreStock::CreateInitial(cs::List<cs::Ref<Item>> items) {
    auto s = cs::New<StoreStock>();
    for (auto& it : items) {
        if (it->Id == 0) continue;
        if (it->Id >= 356 && it->Id <= 361) {
            ItemAlignment a;
            if (it->Id == 358) a = ItemAlignment::Good;
            else if (it->Id == 359) a = ItemAlignment::Evil;
            else a = ItemAlignment::Neutral;
            s->Shelf[Key(it->Id, a)] = 1;
            continue;
        }
        if (it->Floor > 1 || it->Price > 200000 || it->Price == 0 || it->Cursed != 0) continue;
        // "If Items(l01CA).Align > 1" (MAINMOD.BAS:894) - the flag word as a number, not a count of its bits
        if (it->AlignmentFlags > 1) {
            s->Shelf[Key(it->Id, ItemAlignment::Good)] = 1;
            s->Shelf[Key(it->Id, ItemAlignment::Neutral)] = 1;
            s->Shelf[Key(it->Id, ItemAlignment::Evil)] = 1;
        }
        else s->Shelf[Key(it->Id, ItemAlignment::Unaligned)] = it->Floor == 0 ? 99 : 5;
    }
    return s;
}

void StoreStock::Replenish(cs::List<cs::Ref<Item>> items, int32_t days, cs::Ref<cs::Random> rng, int32_t deepestReached,
                           cs::Func<void(cs::Ref<Item>)> shelved) {
    // `Get #gv1B58, 2, l02BC` and the cap at 14 (1505-1508); the map record is never below 1 (MAINMOD.BAS:510-512)
    int32_t deepest = cs::Math::Clamp(deepestReached, (int32_t)1, (int32_t)DeepestStocked);
    int32_t n = items.Count();
    int32_t kinds = Mdata1::ItemTypes.Count() > 0 ? Mdata1::ItemTypes.Count() : n == 0 ? 0 : items.Max([&](auto& i) { return (int32_t)i->Type; }) + 1;
    if (kinds == 0) return;

    // `l02BE = (p02A6 / 100) * 100`, a Single, clamped 1..50 (1510-1512)
    float chance = cs::Math::Clamp(days / 100.0f * 100.0f, 1.0f, 50.0f);
    int32_t last = 0;                                   // l02B6: Dim'd once, so it outlives a pass

    while (true)                                                                              // 1509
    {
        if (rng->NextDouble() * 100 > chance) break;                                          // 1513-1515
        bool adding = rng->NextDouble() * 100 < 60;                                           // 1517-1521

        // a floor, biased towards the shallow end (1522-1531)
        int32_t roll = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * 100)) + 1, floor = 0;
        for (int32_t f = 1; f <= deepest; f++) {
            if (roll < 100 - (f / (double)(deepest + 1)) * 100) floor = f; else break;
        }
        floor = cs::Math::Clamp(floor, (int32_t)1, deepest);

        // Selling loops until something is drawn (1593); an unreachable table would hang the original
        if (adding && !Reachable(items, kinds, floor)) continue;

        int32_t tries = 0;                                                                    // 1595
        while (true) {
            tries++;                                                                          // 1597
            int32_t chosen = DrawOne(items, kinds, floor, rng, last);
            if (chosen > 0) {
                auto it = items[chosen];
                if (it->Cursed == 0 && it->Floor > 0)                                         // 1574, 1638
                {
                    // sub0730 files what was shelved, inside the If that adds it (1582-1586)
                    if (adding) { if (AddOne(it, rng)) { if (shelved) shelved(it); } }
                    else if (!RemoveOne(it, rng)) chosen = 0;
                }
                else chosen = 0;
            }
            if (adding ? chosen != 0 : tries >= 100 || chosen != 0) break;                    // 1593, 1671
        }
    }
}

int32_t StoreStock::DrawOne(cs::List<cs::Ref<Item>> items, int32_t kinds, int32_t floor, cs::Ref<cs::Random> rng, int32_t& last) {
    // MAINMOD.BAS:1535 and 1598: `l02B8 = (Rnd * NumItemTypes)` into an Integer rounds, so type 0 is half
    // as likely and the draw can land on N itself, matching no item
    int32_t kind = Rules::RoundedIndex(rng, kinds);
    int32_t n = items.Count();
    int32_t first = 0;                                                                        // 1537-1543
    for (int32_t i = 0; i < n; i++)
        if (items[i]->Type == kind && items[i]->Floor <= floor) { first = i; break; }
    if (first == 0) return 0;                                                                 // 1544

    for (int32_t i = first; i < n; i++)                                                       // 1545-1550: no
        if (items[i]->Floor > floor || items[i]->Type != kind) { last = i - 1; break; }       // end, l02B6 stale
    if (last < first) last = first;                                                           // 1551-1553

    float total = 0;                                                                          // 1554-1557, Single
    for (int32_t i = first; i <= last; i++) total += items[i]->Rarity;

    float pick = (float)(rng->NextDouble() * 100), acc = 0;                                   // 1565-1566
    for (int32_t i = first; i <= last; i++) {
        acc += Weight(items[i]->Rarity, total);                                               // 1558-1564, 1569
        if (pick <= acc) return i;                                                            // 1570-1573
    }
    return 0;
}

int32_t StoreStock::Weight(int32_t rarity, float total) {
    return rarity > 0 ? cs::CastDouble<int32_t>(cs::Math::Round((double)(rarity / total * 100.0f), cs::MidpointRounding::ToEven)) : 0;
}

bool StoreStock::Reachable(cs::List<cs::Ref<Item>> items, int32_t kinds, int32_t floor) {
    int32_t n = items.Count();
    for (int32_t k = 0; k < kinds; k++) {
        int32_t first = 0;
        for (int32_t i = 0; i < n; i++) if (items[i]->Type == k && items[i]->Floor <= floor) { first = i; break; }
        if (first == 0) continue;
        int32_t last = first;
        for (int32_t i = first; i < n; i++) if (items[i]->Floor > floor || items[i]->Type != k) { last = cs::Math::Max(first, i - 1); break; }
        float total = 0;
        for (int32_t i = first; i <= last; i++) total += items[i]->Rarity;
        int32_t acc = 0;
        for (int32_t i = first; i <= last && acc < 100; i++) {
            int32_t w = Weight(items[i]->Rarity, total);
            auto it = items[i];
            if (w > 0 && it->Cursed == 0 && it->Floor > 0 && (it->AlignmentFlags <= 1 || (it->AlignmentFlags & 0xF) != 0)) return true;
            acc += w;
        }
    }
    return false;
}

bool StoreStock::AddOne(cs::Ref<Item> it, cs::Ref<cs::Random> rng) {
    auto a = ItemAlignment::Unaligned;
    if (it->AlignmentFlags > 1)                                                               // 1577
    {
        // an Align with none of bits 0-3 set would spin for ever in the original
        if ((it->AlignmentFlags & 0xF) == 0) return false;
        do a = static_cast<ItemAlignment>(cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * 4)));   // 1578-1580
        while ((it->AlignmentFlags & (1 << static_cast<int32_t>(a))) == 0);
    }
    if (Count(it->Id, a) >= ShelfCap) return false;                                           // 1582
    Add(it->Id, a);                                                                           // 1583-1585
    return true;
}

bool StoreStock::RemoveOne(cs::Ref<Item> it, cs::Ref<cs::Random> rng) {
    if (Count(it->Id, ItemAlignment::Unaligned) <= 0) return false;
    ItemAlignment a;
    do a = static_cast<ItemAlignment>(cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * 4)));       // 1648-1650
    while (Count(it->Id, a) <= 0);
    Take(it->Id, a);
    return true;
}

}  // namespace mc
