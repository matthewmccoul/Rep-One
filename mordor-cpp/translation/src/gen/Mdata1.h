#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct NamedIndex {
    cs::String Name{};
    int32_t Index = 0;
    bool operator==(const NamedIndex& o) const { return Name == o.Name && Index == o.Index; }
    bool operator!=(const NamedIndex& o) const { return !(*this == o); }
};

struct Mdata1_Layout {
    cs::String Version = "";
    Tup_First_Count_ce1bd Races{};
    Tup_First_Count_ce1bd Guilds{};
    Tup_First_Count_ce1bd ItemTypes{};
    Tup_First_Count_ce1bd ItemDescriptions{};
    Tup_First_Count_ce1bd MonsterClasses{};
    Tup_First_Count_ce1bd MonsterGroups{};
};

struct Mdata1 {
    using Layout = Mdata1_Layout;

    static constexpr int32_t RecordLength = 260;

    static cs::Ref<Mdata1_Layout> Read(cs::Ref<RecordReader> r);
    static cs::List<NamedIndex> ReadTable(cs::Ref<RecordReader> r, Tup_First_Count_ce1bd at);

    inline static cs::List<NamedIndex> ItemTypes = cs::List<NamedIndex>::New();
    inline static cs::List<NamedIndex> ItemDescriptions = cs::List<NamedIndex>::New();
    inline static cs::List<NamedIndex> MonsterClasses = cs::List<NamedIndex>::New();
    inline static cs::List<NamedIndex> MonsterGroups = cs::List<NamedIndex>::New();
    inline static bool Loaded = false;

    static void LoadFrom(cs::String gameDir);
    static cs::String At(cs::List<NamedIndex> table, int32_t i, cs::String fallback);
    static cs::String ItemTypeName(int32_t kind, cs::String fallback = cs::String("Item"));
    static cs::String ItemDescriptionName(int32_t kind, cs::String fallback = cs::String("Unidentified Item"));
    static cs::String MonsterClassName(int32_t cls, cs::String fallback = cs::String("Monster"));
    static cs::String MonsterGroupName(int32_t cls, cs::String fallback = cs::String("Monster"));
};

}  // namespace mc
