#include "core.h"

namespace mc {

cs::String SharedMap::SeenKey(int32_t level, int32_t x, int32_t y, int32_t width) {
    return cs::ToStr(level) + ":" + cs::ToStr(cs::Wrap<int32_t>((int64_t)y * width + x));
}

int64_t SharedMap::SeenFlags(int32_t level, int32_t x, int32_t y, int32_t width) {
    int64_t f{};
    return Seen.TryGetValue(SeenKey(level, x, y, width), f) ? f : 0LL;
}

bool SharedMap::Recorded(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit) {
    return (SeenFlags(level, x, y, width) & ((int64_t)1 << (bit & 63))) != 0;
}

void SharedMap::Record(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit) {
    Seen[SeenKey(level, x, y, width)] = SeenFlags(level, x, y, width) | ((int64_t)1 << (bit & 63));
}

void SharedMap::Unrecord(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit) {
    cs::String k = SeenKey(level, x, y, width);
    int64_t f{};
    if (Seen.TryGetValue(k, f)) Seen[k] = f & ~((int64_t)1 << (bit & 63));
}

bool SharedMap::HasRecordFor(int32_t level) {
    cs::String prefix = cs::ToStr(level) + ":";
    for (auto& k : Seen.Keys()) if (cs::Str::StartsWith(k, prefix)) return true;
    return false;
}

bool SharedMap::Knows(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit) {
    cs::List<int32_t> bits{};
    return Noticed.TryGetValue(cs::ToStr(level) + ":" + cs::ToStr(cs::Wrap<int32_t>((int64_t)y * width + x)), bits) && bits.Contains(bit);
}

void SharedMap::Note(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit) {
    cs::String key = cs::ToStr(level) + ":" + cs::ToStr(cs::Wrap<int32_t>((int64_t)y * width + x));
    cs::List<int32_t> bits{};
    if (!Noticed.TryGetValue(key, bits)) { bits = cs::List<int32_t>::New(); Noticed[key] = bits; }
    if (!bits.Contains(bit)) bits.Add(bit);
}

cs::String SharedMap::PathFor(cs::String savesDir) {
    return cs::Path::Combine(savesDir, "automap.json");
}

cs::Ref<SharedMap> SharedMap::Load(cs::String savesDir) {
    (void)savesDir;
    throw cs::NotSupportedException("not translated: JSON deserialization of SharedMap");
}

void SharedMap::Save(cs::String savesDir) {
    (void)savesDir;
    throw cs::NotSupportedException("not translated: JSON serialization of SharedMap");
}

cs::HashSet<Tup_x_y_82717> SharedMap::CellsFor(int32_t level, int32_t width) {
    auto set = cs::HashSet<Tup_x_y_82717>::New();
    cs::List<int32_t> cells{};
    if (Explored.TryGetValue(level, cells))
        for (auto& i : cells) set.Add(Tup_x_y_82717{i % width, i / width});
    return set;
}

void SharedMap::Merge(int32_t level, cs::List<Tup_x_y_82717> cells, int32_t width) {
    cs::List<int32_t> list{};
    auto have = Explored.TryGetValue(level, list) ? cs::HashSet<int32_t>::From(list) : cs::HashSet<int32_t>::New();
    for (auto& c : cells) { auto x = c.x; auto y = c.y; have.Add(cs::Wrap<int32_t>((int64_t)y * width + x)); }
    Explored[level] = have.ToList().OrderBy([&](auto& v) { return v; }).ToList();
}

int32_t SharedMap::DeepestVisited() {
    if (Explored.Count() == 0) return 0;
    cs::List<int32_t> keys = cs::List<int32_t>::New();
    for (auto& kv : Explored) if (kv.Value.Count() > 0) keys.Add(kv.Key);
    if (keys.Count() == 0) keys.Add(0);  // DefaultIfEmpty(0)
    return keys.Max();
}

void SharedMap::AbsorbLegacy(cs::Ref<Character> c, int32_t width) {
    for (auto& kv : c->Explored) {
        auto level = kv.Key;
        auto cells = kv.Value;
        Merge(level, cells.Select([&](auto& i) { return Tup_x_y_82717{i % width, i / width}; }), width);
    }
}

}  // namespace mc
