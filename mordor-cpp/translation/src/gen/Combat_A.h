#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CombatMonster {
    int32_t _hits = 0;
    cs::Ref<Monster> Def;
    int32_t MaxHits = 0;
    bool Unrolled = false;
    bool AtNought = false;
    int32_t Alignment = -1;
    bool Gone = false;
    cs::Ref<Monster> Believed;
    bool Hostile = true;
    int32_t JoinOffer = 0;
    bool Identified = true;
    int32_t Group = -1;

    int32_t Hits();
    void set_Hits(int32_t value);
    ItemAlignment CompanionAlignment();
    cs::Ref<Monster> Seen();
    bool Alive();
    void LoadRecord(cs::Ref<cs::Random> rng);
    void TakeRecord(int32_t hits, int32_t maxHits);
    static bool SameGroup(cs::Ref<CombatMonster> a, cs::Ref<CombatMonster> b);
    static cs::List<cs::List<cs::Ref<CombatMonster>>> Runs(cs::List<cs::Ref<CombatMonster>> monsters);
    static cs::List<cs::List<cs::Ref<CombatMonster>>> BySlot(cs::List<cs::Ref<CombatMonster>> monsters);
};

struct MonsterProperty {
    static constexpr int32_t Invisible = 1;
    static constexpr int32_t NeedsGoodWeapon = 4;
    static constexpr int32_t ImmuneToWeapons = 5;
    static bool Has(cs::Ref<Monster> m, int32_t bit);
};

struct SpecialAttack {
    static constexpr int32_t Fire = 1 << 3;
    static constexpr int32_t Cold = 1 << 4;
    static constexpr int32_t AcidSpit = 1 << 5;
    static constexpr int32_t Poison = 1 << 0;
    static constexpr int32_t Disease = 1 << 1;
    static constexpr int32_t Paralyze = 1 << 2;
    static constexpr int32_t Electric = 1 << 6;
    static constexpr int32_t Drain = 1 << 7;
    static constexpr int32_t Stone = 1 << 8;
    static constexpr int32_t DeathTouch = 1 << 9;
    static constexpr int32_t Critical = 1 << 10;
    static constexpr int32_t StealGold = 1 << 11;
    static constexpr int32_t Age = 1 << 12;
    static constexpr int32_t StealItem = 1 << 13;
};

struct FoeSlot {
    int32_t Ref = 0;
    int32_t KnownAs = 0;
    int32_t GroupCount = 0;
    int32_t Att = 0;
    int32_t Def = 0;
    int32_t Hits = 0;
    int32_t MaxHits = 0;
    int32_t Alignment = 0;
    int32_t IdLevel = 0;
    int32_t Engaged = 0;
    int32_t JoinOffer = 0;
    double Stamp = 0;

    cs::Ref<FoeSlot> Copy();
};

struct RoomRecord {
    cs::List<cs::Ref<FoeSlot>> Foes = cs::List<cs::Ref<FoeSlot>>::Of({cs::New<FoeSlot>(), cs::New<FoeSlot>(), cs::New<FoeSlot>(), cs::New<FoeSlot>()});
    int32_t TreasureClass = 0;
    int32_t Lead = 0;
    cs::Decimal Gold{};
    int32_t ChestTrap = 0;
    int32_t ChestLock = 0;

    bool AnyStanding();
    cs::Ref<RoomRecord> Copy();
};

struct Encounter : cs::List<cs::Ref<CombatMonster>> {
    Encounter() : cs::List<cs::Ref<CombatMonster>>(cs::List<cs::Ref<CombatMonster>>::New()) {}

    cs::List<cs::Ref<FoeSlot>> Foes = cs::List<cs::Ref<FoeSlot>>::Of({cs::New<FoeSlot>(), cs::New<FoeSlot>(), cs::New<FoeSlot>(), cs::New<FoeSlot>()});
    int32_t TreasureClass = 0;
    int32_t ChestTrap = 0;
    int32_t ChestLock = 0;
    cs::Decimal Gold{};
    int32_t Lead = -1;
    cs::Ref<Monster> LeadMonster;
    Tup_Level_Area_5bf4b Room = {-1, -1};
    bool Failed = false;
    bool Fresh = false;
    cs::List<cs::String> Messages = cs::List<cs::String>::New();
    int32_t AsmodeusCues = 0;
    int32_t Restarts = 0;

    cs::Ref<RoomRecord> ToRecord();
    void Take(cs::Ref<RoomRecord> r, cs::List<cs::Ref<Monster>> all);
};

struct EncounterGenerator {
    inline static const cs::String GoldOverflowLine = "Base Error 'Overflow' Generating Monsters!";
    inline static cs::Dictionary<cs::List<cs::Ref<Monster>>, cs::List<cs::List<int32_t>>> _ranges = cs::Dictionary<cs::List<cs::Ref<Monster>>, cs::List<cs::List<int32_t>>>::New();
    inline static int32_t _below = 0;
    inline static int32_t _above = 0;
    static constexpr int32_t ProbeAreaId = 18;
    static constexpr int32_t FreeWill = -10;
    static constexpr int32_t MaxGroups = 4;

    static Tup_Below_Above_4f99c StaleWindow();
    static void set_StaleWindow(Tup_Below_Above_4f99c value);

    static cs::Ref<Encounter> InRoom(cs::List<cs::Ref<Monster>> all, cs::Ref<Floor> floor, int32_t level, int32_t x, int32_t y, cs::Ref<DungeonState> state, cs::DateTime now,
                                     cs::Ref<cs::Random> rng, cs::Ref<Character> viewer = {}, cs::Ref<Character> idReader = {}, cs::Func<cs::Ref<Item>(int16_t)> item = {});
    static cs::Ref<Encounter> InRoom(cs::List<cs::Ref<Monster>> all, cs::Ref<Floor> floor, int32_t level, int32_t x, int32_t y, cs::Ref<DungeonState> state, cs::DateTime now,
                                     cs::Ref<cs::Random> rng, bool& written, cs::Ref<Character> viewer = {}, cs::Ref<Character> idReader = {},
                                     cs::Func<cs::Ref<Item>(int16_t)> item = {});
    static cs::Ref<RoomRecord> Capture(cs::Ref<RoomRecord> stored, cs::List<cs::Ref<CombatMonster>> members, cs::Ref<CombatEngine> fight, cs::List<cs::Ref<Monster>> all);
    static int32_t Int(double d);
    static int32_t CInt(double d);
    static int64_t CLng(double d);
    static void SetGold(cs::Ref<Encounter> e, double d);
    static int32_t ClassCount(cs::List<cs::Ref<Monster>> all);
    static cs::List<cs::List<int32_t>> ClassRanges(cs::List<cs::Ref<Monster>> all);
    static cs::Ref<Encounter> Generate(cs::List<cs::Ref<Monster>> all, int32_t level, cs::Ref<cs::Random> rng, cs::Ref<Floor> floor = {}, int32_t x = -1, int32_t y = -1,
                                       cs::Ref<Character> viewer = {}, cs::Ref<Character> idReader = {}, cs::Func<cs::Ref<Item>(int16_t)> item = {}, int32_t forceLead = -1, double timer = 0);
    static void FillGroup(cs::Ref<Encounter> e, int32_t g, int32_t m0284, cs::List<cs::Ref<Monster>> all, cs::Ref<cs::Random> rng, bool fog,
                          cs::Ref<Character> reader, cs::Func<cs::Ref<Item>(int16_t)> item, int32_t meAlign, cs::Ref<Character> me, cs::List<cs::List<int32_t>> range, double timer);
    static int32_t KnownAsIndex(cs::List<cs::Ref<Monster>> all, int32_t refIndex, cs::Ref<Character> reader, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Ref<cs::Random> rng, cs::List<cs::List<int32_t>> range);
    static double IdSkill(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item);
    static cs::Ref<Encounter> Finish(cs::Ref<Encounter> e, cs::List<cs::Ref<Monster>> all, cs::Ref<cs::Random> rng);
    static cs::Ref<CombatEngine> ClearedByJoining(cs::Ref<Party> party, cs::List<cs::Ref<CombatMonster>> emptied, cs::Ref<DungeonState> state, int32_t level, int32_t areaId,
                                                  int32_t headerAreas, cs::DateTime now, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Ref<cs::Random> rng,
                                                  cs::List<cs::Ref<Monster>> all);
    static cs::Ref<Monster> BelievedToBe(cs::List<cs::Ref<Monster>> all, cs::Ref<Monster> def, cs::Ref<Character> viewer, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item = {});
    static cs::String AcceptJoin(cs::Ref<Character> who, cs::List<cs::Ref<CombatMonster>> group, cs::Func<cs::Ref<Item>(int16_t)> item,
                                 cs::Ref<cs::Random> rng = {}, int32_t target = -1);
    static cs::String NameFor(cs::Ref<Monster> def, cs::Ref<Character> viewer, int32_t count, bool identified = true);
    static cs::String NameFor(cs::Ref<Monster> def, cs::Ref<Character> viewer, int32_t count, int32_t idLevel);
    static cs::String Pluralise(cs::String name, int32_t count);
    static cs::String Describe(cs::List<cs::Ref<CombatMonster>> group, cs::Ref<Character> viewer = {});
    static cs::String Plural(cs::String name);
};

}  // namespace mc
