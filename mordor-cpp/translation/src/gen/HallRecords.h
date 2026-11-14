#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct HallRecords_Record {
    cs::Nullable<cs::Decimal> _exact{};
    VbCurrency Value{};
    cs::String Holder = "";
    int64_t Day = 0;
    int64_t ValueWhole();
    void set_ValueWhole(int64_t value);
    cs::Nullable<cs::Decimal> ValueExact();
    void set_ValueExact(cs::Nullable<cs::Decimal> value);
    void OnDeserialized();
};

struct HallRecords {
    using Kind = HallRecords_Kind;
    using Record = HallRecords_Record;

    cs::Dictionary<int32_t, cs::Ref<HallRecords_Record>> Records = cs::Dictionary<int32_t, cs::Ref<HallRecords_Record>>::New();

    static cs::String LogName(HallRecords_Kind k);
    static cs::String Heading(HallRecords_Kind k);
    cs::Ref<HallRecords_Record> For(HallRecords_Kind k);
    cs::String Describe(HallRecords_Kind k, cs::Func<cs::String(VbCurrency)> valueText);
    cs::String Check(HallRecords_Kind k, cs::Ref<Character> c, VbCurrency value, cs::Func<int32_t(int32_t)> monsterPower = {}, bool alone = false);
    cs::String CheckAndLog(HallRecords_Kind k, cs::Ref<Character> c, VbCurrency value, cs::String savesDir, cs::Func<int32_t(int32_t)> monsterPower = {}, bool alone = false);
    static int32_t MonsterPower(cs::Ref<Monster> m);
    static int32_t CountBits(int64_t mask, int32_t width);
    static int32_t FightingSkill(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static int32_t ThievingSkill(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item);
    static int32_t MagicSkill(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> spells);
    static int64_t TotalExperience(cs::Ref<Character> c);
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<HallRecords> Fresh();
    static cs::Ref<HallRecords> Load(cs::String savesDir);
    void Save(cs::String savesDir);
};

}  // namespace mc
