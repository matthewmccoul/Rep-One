#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SharedMap {
    cs::Dictionary<int32_t, cs::List<int32_t>> Explored = cs::Dictionary<int32_t, cs::List<int32_t>>::New();
    cs::Dictionary<cs::String, cs::List<int32_t>> Noticed = cs::Dictionary<cs::String, cs::List<int32_t>>::New();
    cs::Dictionary<cs::String, int64_t> Seen = cs::Dictionary<cs::String, int64_t>::New();

    int32_t DeepestVisited();
    static cs::String SeenKey(int32_t level, int32_t x, int32_t y, int32_t width);
    int64_t SeenFlags(int32_t level, int32_t x, int32_t y, int32_t width);
    bool Recorded(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit);
    void Record(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit);
    void Unrecord(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit);
    bool HasRecordFor(int32_t level);
    bool Knows(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit);
    void Note(int32_t level, int32_t x, int32_t y, int32_t width, int32_t bit);
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<SharedMap> Load(cs::String savesDir);
    void Save(cs::String savesDir);
    cs::HashSet<Tup_x_y_82717> CellsFor(int32_t level, int32_t width);
    void Merge(int32_t level, cs::List<Tup_x_y_82717> cells, int32_t width);
    void AbsorbLegacy(cs::Ref<Character> c, int32_t width);
};

}  // namespace mc
