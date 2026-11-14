#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct ImportNote {
    NoteKind Kind{};
    cs::String Where{};
    cs::String Text{};
    cs::String ToString();
};

struct ImportFile {
    cs::String Name{};
    cs::String Holds{};
    cs::String Status{};
    int32_t Records = 0;
};

struct ImportedCharacter {
    int32_t Record = 0;
    cs::String Name{};
    cs::Ref<mc::Character> Character{};
    cs::String Refusal{};
    cs::List<uint8_t> Raw = cs::List<uint8_t>::New();
};

struct ImportedParty {
    cs::String Leader{};
    cs::List<cs::String> Members{};
};

struct OriginalSaves {
    cs::String DataDir{};
    cs::List<cs::Ref<ImportNote>> Notes = cs::List<cs::Ref<ImportNote>>::New();
    cs::List<cs::Ref<ImportFile>> Files = cs::List<cs::Ref<ImportFile>>::New();
    cs::List<cs::Ref<ImportedCharacter>> Characters = cs::List<cs::Ref<ImportedCharacter>>::New();
    int32_t CharacterCount = 0;

    cs::Ref<DungeonState> Dungeon{};
    cs::Ref<SharedMap> Automap{};
    cs::Nullable<int32_t> DeepestReached{};
    cs::Ref<StoreStock> Store{};
    cs::Ref<Librarian_Store> Library{};
    cs::Ref<HallRecords> Hall{};
    cs::Ref<Confinement> Pens{};
    cs::Ref<mc::Guildmasters> Guildmasters{};
    cs::List<cs::Ref<GuildLog_Entry>> GuildLog{};
    cs::List<cs::Ref<ImportedParty>> Parties = cs::List<cs::Ref<ImportedParty>>::New();

    cs::List<cs::Ref<Item>> _items{};
    cs::List<cs::Ref<Monster>> _monsters{};
    cs::Dictionary<int16_t, cs::Ref<Item>> _itemById{};

    static constexpr int32_t BeenHereBit = 23;

    OriginalSaves(cs::String dataDir, cs::List<cs::Ref<Item>> items, cs::List<cs::Ref<Monster>> monsters);

    void Note(NoteKind k, cs::String where, cs::String text);
    static cs::String FindDataDir(cs::String picked);
    static cs::Ref<OriginalSaves> Read(cs::String dataDir, cs::List<cs::Ref<Item>> items, cs::List<cs::Ref<Monster>> monsters);
    void Guard(cs::String file, cs::String holds, cs::Func<int32_t(cs::String, cs::String)> read);
    void Refuse(cs::String file, cs::String holds, cs::String why);
    bool StampOk(cs::Ref<VbFile> f, cs::String file, cs::String holds);
    void CompareTables();
    int32_t ReadCharacters(cs::String path, cs::String file);
    cs::Ref<ImportedCharacter> Convert(int32_t recordNo, cs::String name, cs::List<uint8_t> raw);
    cs::Ref<InventorySlot> SlotFrom(VbRecord s, cs::String where, cs::String which);
    int32_t ReadStore(cs::String path, cs::String file);
    int32_t ReadPens(cs::String path, cs::String file);
    int32_t ReadGuildmasters(cs::String path, cs::String file);
    int32_t ReadAutomap(cs::String path, cs::String file);
    int32_t ReadGuildLog(cs::String path, cs::String file);
    int32_t ReadRooms(cs::String path, cs::String file);
    int32_t ReadParties(cs::String path, cs::String file);
    int32_t ReadLibrary(cs::String path, cs::String file);
    int32_t ReadHall(cs::String path, cs::String file);
};

}  // namespace mc
