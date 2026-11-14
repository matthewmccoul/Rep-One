#include "core.h"

namespace mc {

char ItemAlignments::Letter(ItemAlignment a) {
    switch (a) {
        case ItemAlignment::Good: return 'G';
        case ItemAlignment::Neutral: return 'N';
        case ItemAlignment::Evil: return 'E';
        default: return 'U';
    }
}

cs::String ItemAlignments::Name(ItemAlignment a) {
    return a == ItemAlignment::Unaligned ? cs::String("Unaligned") : ToStr(a);
}

cs::List<ItemAlignment> ItemAlignments::Possible(cs::Ref<Item> it) {
    auto list = cs::List<ItemAlignment>::New();
    for (auto& a : All) if ((it->AlignmentFlags & (1 << static_cast<int32_t>(a))) != 0) list.Add(a);
    if (list.Count() == 0) list.Add(ItemAlignment::Unaligned);
    return list;
}

cs::Nullable<Alignment> ItemAlignments::AsCharacter(ItemAlignment a) {
    switch (a) {
        case ItemAlignment::Good: return Alignment::Good;
        case ItemAlignment::Neutral: return Alignment::Neutral;
        case ItemAlignment::Evil: return Alignment::Evil;
        default: return std::nullopt;
    }
}

int32_t ItemAlignments::AttDefWorth(int32_t raw, ItemFit fit) {
    switch (fit) {
        case ItemFit::Fine: return raw;
        case ItemFit::Adjacent: return cs::CastDouble<int32_t>(cs::Math::Round(raw / 2.0, cs::MidpointRounding::ToEven));
        default: return -raw;
    }
}

int32_t ItemAlignments::StatModWorth(int32_t raw, ItemFit fit) {
    switch (fit) {
        case ItemFit::Fine: return raw;
        case ItemFit::Adjacent: return cs::CastDouble<int32_t>(cs::Math::Floor(raw / 2.0));
        default: return -raw;
    }
}

ItemFit ItemAlignments::FitFor(ItemAlignment item, Alignment who) {
    auto a = AsCharacter(item);
    if (!a.has_value() || a.value() == who) return ItemFit::Fine;
    return Party::Opposed(a.value(), who) ? ItemFit::Opposed : ItemFit::Adjacent;
}

MonsterKind Monster::Kind() { return static_cast<MonsterKind>(Type); }

Floor_Field Floor::At(int32_t x, int32_t y) { return Fields[y * Width + x]; }

bool Floor::Has(int32_t x, int32_t y, int32_t bit) { return (At(x, y).Flags & (1LL << bit)) != 0; }

cs::Nullable<Floor_Area> Floor::AreaAt(int32_t x, int32_t y) {
    int32_t id = At(x, y).AreaId;
    if (id >= 1 && id < Areas.Count()) return Areas[id];
    return std::nullopt;
}

int32_t Floor::AreaIdAt(int32_t x, int32_t y) { return At(x, y).AreaId; }

int32_t Floor::AreasInUse() {
    if (_areasInUse >= 0) return _areasInUse;
    auto seen = cs::HashSet<int32_t>::New();
    for (int32_t y = 0; y < Height; y++)
        for (int32_t x = 0; x < Width; x++)
            if (At(x, y).AreaId >= 1) seen.Add(At(x, y).AreaId);
    return _areasInUse = seen.Count();
}

}  // namespace mc
