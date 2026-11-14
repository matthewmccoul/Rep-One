#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct ItemAlignments {
    inline static cs::List<ItemAlignment> All = cs::List<ItemAlignment>::Of({ItemAlignment::Unaligned, ItemAlignment::Good, ItemAlignment::Neutral, ItemAlignment::Evil});
    static char Letter(ItemAlignment a);
    static cs::String Name(ItemAlignment a);
    static cs::List<ItemAlignment> Possible(cs::Ref<Item> it);
    static cs::Nullable<Alignment> AsCharacter(ItemAlignment a);
    static int32_t AttDefWorth(int32_t raw, ItemFit fit);
    static int32_t StatModWorth(int32_t raw, ItemFit fit);
    static ItemFit FitFor(ItemAlignment item, Alignment who);
};

struct Spell {
    cs::String Name = "";
    int16_t Id = 0;
    int16_t Category = 0;
    int16_t Level = 0;
    int16_t U4 = 0;
    int16_t KillEffect = 0;
    int16_t AffectMonster = 0;
    int16_t AffectGroup = 0;
    int16_t Damage1 = 0;
    int16_t Damage2 = 0;
    int16_t SpecialEffect = 0;
    int16_t Effect = 0;
    cs::List<int16_t> Required = cs::List<int16_t>::New(7);
    int16_t ResistIndex = -1;
};

struct Item {
    cs::String Name = "";
    int16_t Id = 0;
    int16_t Att = 0;
    int16_t Def = 0;
    int32_t Price = 0;
    int16_t Floor = 0;
    int16_t Rarity = 0;
    int32_t Abilities = 0;
    int16_t Swings = 0;
    int16_t SpecialType = 0;
    int16_t SpellIndex = 0;
    int16_t SpellId = 0;
    int32_t Charges = 0;
    int32_t Guilds = 0;
    int16_t LevelScale = 0;
    float DamageMod = 0;
    int32_t AlignmentFlags = 0;
    int16_t NHands = 0;
    int16_t Type = 0;
    int32_t ResistanceFlags = 0;
    cs::List<int16_t> StatsRequired = cs::List<int16_t>::New(7);
    cs::List<int16_t> StatsMod = cs::List<int16_t>::New(7);
    int16_t Cursed = 0;
    int16_t SpellLvl = 0;
    int16_t ClassRestricted = 0;
};

struct Monster {
    int32_t Ref = -1;
    cs::String Name = "";
    int16_t Att = 0;
    int16_t Def = 0;
    int16_t Id = 0;
    int16_t Hits = 0;
    int16_t NumGroups = 0;
    int16_t PicId = 0;
    int16_t LockedChance = 0;
    int16_t LevelFound = 0;
    cs::List<int16_t> Resistances = cs::List<int16_t>::New(12);
    int32_t SpecialPropertyFlags = 0;
    int32_t SpecialAttackFlags = 0;
    int32_t SpellFlags = 0;
    int16_t Chance = 0;
    cs::List<int16_t> BoxChance = cs::List<int16_t>::New(4);
    int16_t AlignmentValue = 0;
    int16_t InGroup = 0;
    int32_t GoldFactor = 0;
    int32_t TrapFlags = 0;
    int16_t GuildLevel = 0;
    cs::List<int16_t> Stats = cs::List<int16_t>::New(7);
    int16_t Type = 0;
    float DamageMod = 0;
    int16_t CompanionType = 0;
    int16_t CompanionSpawnMode = 0;
    int16_t CompanionId = 0;
    cs::List<int16_t> Items = cs::List<int16_t>::New(11);
    int16_t Subtype = 0;
    int16_t CompanionSubtype = 0;
    int16_t Size = 0;
    int16_t ItemDropLevel = 0;
    int16_t ItemChance = 0;

    MonsterKind Kind();
};

struct Floor_Field {
    int16_t AreaId = 0;
    int64_t Flags = 0;
};

struct Floor_Area {
    int32_t SpawnMask = 0;
    int16_t LairId = 0;
};

struct Floor_Chute {
    int16_t X = 0;
    int16_t Y = 0;
    int16_t Depth = 0;
};

struct Floor_Teleporter {
    int16_t X = 0;
    int16_t Y = 0;
    int16_t X2 = 0;
    int16_t Y2 = 0;
    int16_t Z2 = 0;
};

struct Floor {
    using Field = Floor_Field;
    using Area = Floor_Area;
    using Chute = Floor_Chute;
    using Teleporter = Floor_Teleporter;

    // bit positions within Field.Flags
    static constexpr int32_t WALL_EAST = 0;
    static constexpr int32_t WALL_NORTH = 1;
    static constexpr int32_t DOOR_EAST = 2;
    static constexpr int32_t DOOR_NORTH = 3;
    static constexpr int32_t SECRET_DOOR_EAST = 4;
    static constexpr int32_t SECRET_DOOR_NORTH = 5;
    static constexpr int32_t FACE_NORTH = 6;
    static constexpr int32_t FACE_EAST = 7;
    static constexpr int32_t FACE_SOUTH = 8;
    static constexpr int32_t FACE_WEST = 9;
    static constexpr int32_t EXTINGUISHER = 10;
    static constexpr int32_t PIT = 11;
    static constexpr int32_t STAIRS_UP = 12;
    static constexpr int32_t STAIRS_DOWN = 13;
    static constexpr int32_t TELEPORTER = 14;
    static constexpr int32_t WATER = 15;
    static constexpr int32_t QUICKSAND = 16;
    static constexpr int32_t ROTATOR = 17;
    static constexpr int32_t ANTIMAGIC = 18;
    static constexpr int32_t ROCK = 19;
    static constexpr int32_t FOG = 20;
    static constexpr int32_t CHUTE = 21;
    static constexpr int32_t STUD = 22;
    static constexpr int32_t RESCUE = 23;

    int16_t Width = 0;
    int16_t Height = 0;
    int16_t LevelNum = 0;
    cs::List<Floor_Field> Fields = cs::List<Floor_Field>::New();
    cs::List<Floor_Area> Areas = cs::List<Floor_Area>::New();
    cs::List<Floor_Teleporter> Teleporters = cs::List<Floor_Teleporter>::New();
    cs::List<Floor_Chute> Chutes = cs::List<Floor_Chute>::New();

    Floor_Field At(int32_t x, int32_t y);
    bool Has(int32_t x, int32_t y, int32_t bit);
    cs::Nullable<Floor_Area> AreaAt(int32_t x, int32_t y);
    int32_t AreaIdAt(int32_t x, int32_t y);
    int32_t AreasInUse();

    int32_t _areasInUse = -1;
    int32_t HeaderAreaCount = 0;
};

struct LibraryRecord {
    int16_t Id = 0;
    int16_t Known = 0;
    int64_t NSeen = 0;
    cs::String Name = "";
    int16_t MonId = 0;
    int32_t LastSeen = 0;
    cs::String Location = "";
    int16_t NDeaths = 0;
};

struct Library {
    int16_t MonstersFound = 0;
    int16_t ItemsFound = 0;
    cs::List<cs::Ref<LibraryRecord>> Monsters = cs::List<cs::Ref<LibraryRecord>>::New();
    cs::List<cs::Ref<LibraryRecord>> Items = cs::List<cs::Ref<LibraryRecord>>::New();
};

struct HallRecord {
    int64_t Value = 0;
    cs::String Name = "";
    int32_t Date = 0;
};

}  // namespace mc
