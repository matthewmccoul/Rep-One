#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GuildMembership {
    cs::String Guild{};
    int32_t Level = 1;
    int64_t Experience = 0;
    int32_t QuestType = 0;
    int16_t QuestTarget = 0;
    bool QuestDone = false;
};

struct InventorySlot {
    int16_t ItemId = 0;
    bool Equipped = false;
    int16_t Charges = 0;
    IdLevel Known();
    void set_Known(IdLevel value);
    IdLevel _known = IdLevel::All;
    bool _knownRead = false;

    bool Identified();
    void set_Identified(bool value);

    cs::Nullable<bool> LegacyIdentified();
    void set_LegacyIdentified(cs::Nullable<bool> value);
    bool SlotCursed = false;
    ItemAlignment Alignment = ItemAlignment::Unaligned;
};

struct Companion {
    int32_t Ref = -1;

    int16_t MonsterId = 0;
    cs::String Name{};
    int32_t Hits = 0;
    int32_t MaxHits = 0;

    ItemAlignment Alignment{};

    int32_t IdLevel = 0;

    int32_t BindingLevel = 0;

    bool IsDead();

    bool Vacant = false;
};

struct Binding {
    static constexpr int32_t Max = 12;

    static constexpr int32_t Step = 4;

    static constexpr int32_t ForLife = 100;

    static int32_t NextLevel(int32_t current);

    static cs::Decimal Cost(int32_t monsterGuildLevel, int32_t current);

    static int64_t Charge(cs::Decimal cost);

    static int32_t Initial(int32_t monsterFloor);

    static cs::String Describe(int32_t level, double chanceToLeave);

    static double ChanceToLeave(int32_t bindingLevel, int32_t monsterGuildLevel, int32_t topGuildLevel,
                                int32_t charisma, int32_t hits, int32_t maxHits);
};

struct Character : std::enable_shared_from_this<Character> {
    Character();

    cs::String Name{};
    cs::String Race = "Human";
    mc::Alignment Alignment = mc::Alignment::Neutral;
    mc::Sex Sex = mc::Sex::Male;

    cs::List<int32_t> Stats = cs::List<int32_t>::New(6);          // STR,INT,WIS,CON,CHA,DEX (current)
    cs::List<int32_t> StatMax = cs::List<int32_t>::New(6);        // per-race caps

    int32_t MaxHits = 0;
    int32_t CurHits = 0;
    int32_t MaxSpellPoints = 0;
    int32_t CurSpellPoints = 0;

    bool Poisoned = false;
    bool Diseased = false;
    bool Stoned = false;
    int32_t PoisonLevel = 0;
    int32_t DiseaseLevel = 0;
    int32_t AilmentSteps = 0;

    int32_t ParalysisLevel = 0;

    bool Paralysed();

    int32_t ADShift = 0;

    bool ADShiftUnsettled = false;

    int32_t StatsADNow(cs::Func<cs::Ref<Item>(int16_t)> item);
    int32_t StoredADShift(cs::Func<cs::Ref<Item>(int16_t)> item);
    void Paralyse(int32_t level, cs::Func<cs::Ref<Item>(int16_t)> item);
    void Unparalyse(cs::Func<cs::Ref<Item>(int16_t)> item);
    void CurePoison();
    void CureDisease();
    void EndAilmentsAndSpellResists();
    void EndEveryAilment(cs::Func<cs::Ref<Item>(int16_t)> item);
    void Sub0930Head(cs::Func<cs::Ref<Item>(int16_t)> item);

    cs::String AilmentText();

    int32_t SpellEffects = 0;

    cs::List<int32_t> SpellResist{};

    int32_t CombatSetting = 0;

    cs::List<int32_t> Buffers{};

    int32_t CarryingCompanion = -1;

    cs::String CarryingCharacter{};

    void ReleaseFollowers();

    bool IsCarrying();

    cs::String CompanionName(int32_t index);

    cs::String CarryText();

    bool Levitating = false;
    bool InvisibleNow = false;
    bool SeeingInvisible = false;
    int32_t Protection = 0;

    bool LostLevel = false;
    bool LostPosition = false;
    bool LostFacing = false;

    int32_t CauseOfDeath = 0;

    int32_t RaiseChance = 0;

    cs::String RaisedBy{};

    int32_t RaiseKind = 0;

    bool IsLost();

    void BecomeLost();

    void FoundYourself();

    void RecalcSpellPoints(bool refill = false);
    VbCurrency GoldOnHand = VbCurrency::Zero;
    VbCurrency GoldInBank = VbCurrency::Zero;

    int64_t GoldOnHandWhole();
    void set_GoldOnHandWhole(int64_t value);
    cs::Nullable<cs::Decimal> GoldOnHandExact();
    void set_GoldOnHandExact(cs::Nullable<cs::Decimal> value);
    int64_t GoldInBankWhole();
    void set_GoldInBankWhole(int64_t value);
    cs::Nullable<cs::Decimal> GoldInBankExact();
    void set_GoldInBankExact(cs::Nullable<cs::Decimal> value);
    cs::Nullable<cs::Decimal> _handExact{};
    cs::Nullable<cs::Decimal> _bankExact{};

    void OnDeserialized();

    VbCurrency Purse();

    bool Pay(int64_t cost);
    bool Receive(VbCurrency amount, bool toBank);
    bool CanReceive(VbCurrency amount, bool toBank);

    int32_t Age = 16 * 365;
    bool IsDead = false;
    int32_t Kills = 0;
    int32_t Deaths = 0;
    int32_t Complications = 0;
    int32_t MonsterQuestsDone = 0;
    int32_t ItemQuestsDone = 0;
    double PlaySeconds = 0;
    double CreatedOn = 0;

    cs::List<cs::Ref<GuildMembership>> Guilds = cs::List<cs::Ref<GuildMembership>>::New();

    cs::List<cs::Ref<InventorySlot>> Pack();
    void set_Pack(cs::List<cs::Ref<InventorySlot>> value);
    cs::List<cs::Ref<InventorySlot>> _pack = FreshPack();

    static constexpr int32_t PackSize = 40;

    static constexpr int16_t HandsItemId = 0;

    static cs::List<cs::Ref<InventorySlot>> FreshPack();
    static cs::List<cs::Ref<InventorySlot>> Resize(cs::List<cs::Ref<InventorySlot>> given);

    bool SlotEmpty(int32_t i);
    int32_t FirstFreeSlot();

    cs::List<cs::Ref<InventorySlot>> Inventory();

    int32_t SlotOf(cs::Ref<InventorySlot> slot);
    bool PutInPack(cs::Ref<InventorySlot> slot);
    void PutInSlot(int32_t i, cs::Ref<InventorySlot> slot);
    void ClearPack();

    cs::List<cs::Ref<InventorySlot>> LegacyInventory{};

    void MigrateLegacyPack();

    cs::List<cs::Ref<InventorySlot>> Bank = cs::List<cs::Ref<InventorySlot>>::New();

    static constexpr int32_t BankSlots = 40;

    bool DepositToVault(int32_t packSlot);

    cs::String Password{};

    cs::Ref<CharacterOptions> Options{};

    cs::List<cs::Ref<WindowRect>> Windows{};

    int32_t WindowsRevision = 0;

    cs::Ref<WindowRect> WindowFor(GameWindow which);
    cs::List<cs::Ref<WindowRect>> NormaliseWindows();

    inline static const cs::String NoPassword = " ";

    bool HasPassword();

    bool PasswordMatches(cs::String entered);

    cs::Ref<CompanionSlots> Companions{};
    static constexpr int32_t MaxCompanionSlots = 4;
    int32_t MaxCompanions();

    // dungeon position (0 = in town)
    int32_t DungeonLevel = 0;
    int32_t X = 0;
    int32_t Y = 0;

    int32_t Facing = 1;

    cs::Nullable<int32_t> Area{};

    int32_t SanctuaryX = 0;
    int32_t SanctuaryY = 0;
    int32_t SanctuaryLevel = 0;

    bool HasSanctuary();

    cs::Dictionary<int32_t, cs::List<int32_t>> Explored = cs::Dictionary<int32_t, cs::List<int32_t>>::New();

    int32_t AgeYears();

    cs::String GuildTitle();

    cs::String Title();

    int32_t RestInCity();

    static cs::String DescribeDays(int32_t days);

    cs::String AgeBy(int32_t days, bool inAParty);

    cs::Dictionary<int32_t, int32_t> MonstersSeen = cs::Dictionary<int32_t, int32_t>::New();
    cs::HashSet<int32_t> ItemsSeen = cs::HashSet<int32_t>::New();

    void NoteMonster(int32_t id, bool killed);

    int32_t TotalLevel();

    int32_t TopGuildLevel();
    cs::String PrimaryGuild();
    cs::Ref<GuildMembership> Guild();

    int32_t GetStat(Stat s);

    int32_t EffectiveStat(int32_t i, cs::Func<cs::Ref<Item>(int16_t)> item);

    bool RemoveFromPack(cs::Ref<InventorySlot> slot);
    bool RemoveFromPackAt(int32_t index);
    bool SwapSlots(int32_t oneBasedA, int32_t oneBasedB, bool buffersFollow = true);

    static int32_t WearClassOf(cs::Ref<Item> it);
    static int32_t MaxWornOfClass(int32_t wearClass);
    int32_t HandsUsed(cs::Func<cs::Ref<Item>(int16_t)> item);
    int32_t WornOfClass(int32_t wearClass, cs::Func<cs::Ref<Item>(int16_t)> item);
    cs::String EquipProblem(cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> item = {});

    static bool IsWearable(cs::Ref<Item> it);
    int32_t StatWithMods(int32_t i, cs::Func<cs::Ref<Item>(int16_t)> item);
    cs::String WearRefusal(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> item);

    bool MeetsUseLevel(cs::Ref<Item> it);
    static int32_t UseLevel(cs::Ref<Item> it, cs::Ref<GuildDef> g);

    bool ItemHelps(cs::Ref<InventorySlot> slot);
    static bool CanUnequip(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, mc::Alignment who);
    bool CanUnequip(cs::Ref<InventorySlot> slot, cs::Ref<Item> it);
    void ApplyEquipCurse(cs::Ref<InventorySlot> slot, cs::Ref<Item> it);
    void SyncSlotCurses(cs::Func<cs::Ref<Item>(int16_t)> lookup);

    cs::List<cs::String> LastLevelGains = cs::List<cs::String>::New();

    cs::Ref<mc::Race> RaceDef();

    cs::Ref<GuildDef> ActiveGuildDef();
    int64_t ExpForNextLevel();
    int64_t ExpForThisLevel();
    int64_t GoldForNextLevel();
    bool CanLevelUp();
    int32_t LevelUp(cs::Ref<cs::Random> rng);

    void RecalcSwings();

    int32_t Swings = 0;

    cs::List<cs::String> RollQuestForLevel(cs::List<cs::Ref<Monster>> monsters, cs::List<cs::Ref<Item>> items, cs::Ref<cs::Random> rng);
    bool QuestSatisfied(bool consume);

    int64_t ExperienceCap();

    bool IsPinned();

    int32_t ForfeitCost();
    cs::String ForfeitRefusal(cs::Func<cs::Ref<Item>(int16_t)> lookup);
    cs::List<cs::Ref<InventorySlot>> ForfeitQuest(cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::Ref<cs::Random> rng);
    int32_t SwingsFor(cs::Ref<GuildMembership> m);
    bool JoinGuild(cs::Ref<GuildDef> g);
    bool SetActiveGuild(cs::String name);

    static cs::Ref<Character> Create(cs::String name, cs::Ref<mc::Race> race, mc::Alignment alignment, mc::Sex sex, cs::List<int32_t> stats);

    int32_t AreaOr(cs::Func<int32_t(cs::Ref<Character>)> fromSquare);
    void AreaForRockDeath();

    inline static cs::Ref<cs::JsonSerializerOptions> JsonOpts = [] { auto _o = cs::New<cs::JsonSerializerOptions>(); _o->WriteIndented = true; return _o; }();

    cs::List<cs::String> Mods{};

    cs::List<cs::String> ModsWhenLoaded{};

    void Save(cs::String path);
    void CheckNames(cs::String path);
    static cs::Ref<Character> ReadRaw(cs::String path);
    static cs::Ref<Character> Load(cs::String path);
    static cs::List<cs::Ref<Character>> LoadAllFrom(cs::String savesDir);
    static cs::String SafeFileName(cs::String s);
};

}  // namespace mc
