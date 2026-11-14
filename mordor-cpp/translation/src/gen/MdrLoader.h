#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct MdrLoader {
    static cs::List<cs::Ref<Spell>> LoadSpells(cs::String path);
    static cs::List<cs::Ref<Item>> LoadItems(cs::String path);
    static cs::List<cs::Ref<Monster>> LoadMonsters(cs::String path);
    static cs::List<cs::Ref<Floor>> LoadDungeon(cs::String path);
    static cs::Ref<Floor> LoadFloor(cs::Ref<RecordReader> r);
    static cs::Ref<Library> LoadLibrary(cs::String path);
    static cs::Ref<LibraryRecord> ReadLibRecord(cs::Ref<RecordReader> r);
    static cs::List<cs::Ref<Race>> LoadRaces(cs::String path);
    static cs::List<cs::Ref<GuildDef>> LoadGuilds(cs::String path);
    static cs::List<cs::Ref<HallRecord>> LoadHallRecords(cs::String path);
};

}  // namespace mc
