#include "core.h"

namespace mc {

cs::Ref<Mdata1_Layout> Mdata1::Read(cs::Ref<RecordReader> r) {
    auto l = cs::New<Mdata1_Layout>();
    r->Seek(1); r->Read();
    l->Version = r->GetString();

    auto counts = cs::List<int32_t>::New(6);
    for (int32_t i = 0; i < 6; i++) { r->Seek(2 + i); r->Read(); counts[i] = r->GetUWord(); }

    int32_t rec = 8;
    auto Take = [&](int32_t n) { Tup_First_Count_ce1bd at{rec, n}; rec += n; return at; };
    l->Races = Take(counts[0]);
    l->Guilds = Take(counts[1]);
    l->ItemTypes = Take(counts[2]);
    l->ItemDescriptions = Take(counts[3]);
    l->MonsterClasses = Take(counts[4]);
    l->MonsterGroups = Take(counts[5]);
    return l;
}

cs::List<NamedIndex> Mdata1::ReadTable(cs::Ref<RecordReader> r, Tup_First_Count_ce1bd at) {
    auto list = cs::List<NamedIndex>::New();
    for (int32_t i = 0; i < at.Count; i++) {
        r->Seek(at.First + i); r->Read();
        cs::String name = r->GetString();
        list.Add(NamedIndex{name, r->GetWord()});
    }
    return list;
}

void Mdata1::LoadFrom(cs::String gameDir) {
    cs::String path = cs::Path::Combine(gameDir, "MDATA1.MDR");
    if (!cs::File::Exists(path)) return;
    try {
        auto r = cs::New<RecordReader>(path, RecordLength);
        auto l = Read(r);
        ItemTypes = ReadTable(r, l->ItemTypes);
        ItemDescriptions = ReadTable(r, l->ItemDescriptions);
        MonsterClasses = ReadTable(r, l->MonsterClasses);
        MonsterGroups = ReadTable(r, l->MonsterGroups);
        Loaded = ItemTypes.Count() > 0;
    } catch (...) { /* leave the tables empty; callers fall back to the item's own name */ }
}

cs::String Mdata1::At(cs::List<NamedIndex> table, int32_t i, cs::String fallback) {
    return i >= 0 && i < table.Count() ? table[i].Name : fallback;
}

cs::String Mdata1::ItemTypeName(int32_t kind, cs::String fallback) { return At(ItemTypes, kind, fallback); }

cs::String Mdata1::ItemDescriptionName(int32_t kind, cs::String fallback) {
    return kind >= 0 && kind < ItemTypes.Count() ? At(ItemDescriptions, ItemTypes[kind].Index, fallback) : fallback;
}

cs::String Mdata1::MonsterClassName(int32_t cls, cs::String fallback) { return At(MonsterClasses, cls, fallback); }

cs::String Mdata1::MonsterGroupName(int32_t cls, cs::String fallback) {
    return cls >= 0 && cls < MonsterClasses.Count() ? At(MonsterGroups, MonsterClasses[cls].Index, fallback) : fallback;
}

}  // namespace mc
