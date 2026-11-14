#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Librarian_Store {
    inline static const cs::String MonsterPrefix = "mr:";
    inline static const cs::String LegacyMonsterPrefix = "m:";
    inline static const cs::String ItemPrefix = "i:";

    cs::Dictionary<cs::String, cs::Ref<LibraryRecord>> Entries = cs::Dictionary<cs::String, cs::Ref<LibraryRecord>>::New();
    bool MonstersUnsettled = false;
    inline static cs::Func<void(cs::String)> Note = {};
    bool ReadOnly = false;
    cs::String MovedAside{};

    static cs::String MonsterKey(int32_t tableIndex);
    static cs::String ItemKey(int32_t itemId);
    void MonstersSettled();
    cs::Ref<LibraryRecord> Get(cs::String k);
    cs::Ref<LibraryRecord> Make(cs::String k);
    cs::Ref<LibraryRecord> ForMonster(int32_t tableIndex);
    cs::Ref<LibraryRecord> EditMonster(int32_t tableIndex);
    cs::Ref<LibraryRecord> ForItem(int32_t itemId);
    cs::Ref<LibraryRecord> EditItem(int32_t itemId);
    bool KnowsMonster(int32_t tableIndex);
    bool KnowsItem(int32_t itemId);
    bool Known(cs::String k);
    int32_t SettleMonsterKeys(cs::List<cs::Ref<Monster>> table);
    static void Merge(cs::Ref<LibraryRecord> there, cs::Ref<LibraryRecord> newer);
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<Librarian_Store> Load(cs::String savesDir, cs::List<cs::Ref<Monster>> monsters = {});
    static cs::Ref<Librarian_Store> Read(cs::String savesDir);
    void Save(cs::String savesDir);
};

struct Librarian {
    using Store = Librarian_Store;

    static constexpr int32_t VeryLittle = 1;
    static constexpr int32_t AGoodDealMore = 2;
    static constexpr int32_t AlmostEverything = 3;
    static constexpr int32_t Everything = 4;

    // MODULE1.BAS:473-533
    inline static cs::List<cs::String> Sizes = cs::List<cs::String>::Of({"Very Small", "Small", "Normal", "Big", "Very Big", "Huge"});
    inline static cs::List<cs::String> Alignments = cs::List<cs::String>::Of({"Good", "Neutral", "Evil"});
    inline static cs::List<cs::String> MonsterProperties = cs::List<cs::String>::Of(
        {"See Invisible", "Invisible", "Resistant to All Magic", "Charm Resistant",
         "Weapon Resistance", "Complete Weapon Resistance"});
    inline static cs::List<cs::String> MonsterAttacks = cs::List<cs::String>::Of(
        {"Poison", "Disease", "Paralyze", "Breathe Fire", "Breathe Cold", "Acid", "Electrocute",
         "Drain", "Stone", "Age", "Critical Hit", "Backstab", "Destroy Item", "Steal"});
    inline static cs::List<cs::String> SpellSchools = cs::List<cs::String>::Of(
        {"Fire", "Cold", "Electrical", "Mind", "Damage", "Element", "Kill", "Charm", "Bind",
         "Heal", "Movement", "Banish", "Dispell", "Resistant", "Visual", "Magical", "Location",
         "Protection"});
    inline static cs::List<cs::String> Resistances = cs::List<cs::String>::Of(
        {"Fire", "Cold", "Electrical", "Mind", "Disease", "Poison", "Magic", "Stone",
         "Paralysis", "Drain", "Acid"});
    inline static cs::List<cs::String> ItemAbilities = cs::List<cs::String>::Of(
        {"Levitate", "Invisible", "Protect", "See Invisible", "Critical Hit", "Backstab"});
    inline static cs::List<cs::String> HandWords = cs::List<cs::String>::Of({"Zero", "One", "Two", "Three", "Four"});

    inline static cs::Func<cs::String(int32_t)> SpellNameOf = [](int32_t) { return cs::String("a spell"); };

    static cs::String Size(int32_t i);
    static cs::String Align(int32_t i);
    static bool Bit(int64_t mask, int32_t b);
    static cs::String Listed(cs::List<cs::String> parts);
    static cs::String DescribeMonster(cs::Ref<Monster> m, cs::Ref<LibraryRecord> e, int32_t today, cs::String plural);
    static cs::String DescribeItem(cs::Ref<Item> it, cs::Ref<LibraryRecord> e, int32_t today, cs::Func<cs::String(int32_t)> monsterName = {});
    static cs::String Hand(int32_t n);
    static cs::String SpellName(int32_t oneBasedIndex);
    static cs::String DaysAgo(int32_t seen, int32_t today);
    static cs::String DaysAgoForItem(int32_t seen, int32_t today);
    static int32_t Serial(cs::DateTime when);
    static void FileItem(cs::Ref<Librarian_Store> store, cs::Ref<Character> c, int32_t itemId, IdLevel known);
    static void FileItem(cs::String savesDir, cs::Ref<Character> c, int32_t itemId, IdLevel known);
    static void FileFind(cs::Ref<Librarian_Store> store, cs::Ref<Character> c, int32_t itemId, IdLevel known, int16_t fromMonsterId,
                         int32_t today, bool lostLevel, bool lostPosition, int32_t level);
    static void FileRestock(cs::Ref<Librarian_Store> store, int32_t itemId, int32_t today);
    static int16_t ChestFindFrom(cs::Ref<Monster> lead, int32_t leadIndex, int32_t foes0KnownAs, cs::Ref<Character> taker);
    static cs::String FinderName(cs::List<cs::Ref<Monster>> table, int32_t monsterId);
    static void FileCompanion(cs::Ref<Librarian_Store> store, cs::Ref<Character> c, cs::Ref<Monster> def, int32_t idLevel, cs::String place = {});
    static void FileCompanion(cs::String savesDir, cs::List<cs::Ref<Monster>> table, cs::Ref<Character> c, cs::Ref<Monster> def, int32_t idLevel, cs::String place = {});
    static cs::String PlaceOf(bool lostLevel, bool lostPosition, int32_t level, int32_t x, int32_t y);
    static void FileSighting(cs::Ref<Librarian_Store> store, cs::List<cs::Ref<Monster>> all, int32_t refIndex, int32_t knownAs, int32_t groupCount,
                             bool killed, bool sighting, cs::Ref<Character> reader, cs::String place, int32_t leadLevel, int32_t today,
                             cs::Func<cs::Ref<Item>(int16_t)> item = {});
};

}  // namespace mc
