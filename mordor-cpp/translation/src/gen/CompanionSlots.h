#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionSlots : std::enable_shared_from_this<CompanionSlots> {
    static constexpr int32_t Slots = 4;

    cs::List<cs::Ref<Companion>> _slots = cs::List<cs::Ref<Companion>>::New(Slots);

    cs::List<cs::Ref<Companion>> _left = cs::Enumerable::Range(0, Slots).Select([](auto&) { return Blank(); }).ToArray();

    static cs::Ref<Companion> Blank();

    cs::Ref<Companion> Index(int32_t slot);
    void SetIndex(int32_t slot, cs::Ref<Companion> value);

    cs::Ref<Companion> Left(int32_t slot);

    cs::Ref<Companion> Remains(int32_t slot);

    void SetRemains(int32_t slot, cs::Ref<Companion> record);

    void BlankEmptyNames();

    cs::List<Tup_Slot_Companion_d8dfb> Occupied();

    cs::List<cs::Ref<Companion>> All();

    int32_t OccupiedCount();

    bool Any();

    int32_t FreeSlots();

    int32_t FirstFree();

    int32_t Add(cs::Ref<Companion> c, bool keepSlotName = false);

    void Release(int32_t slot);

    void ReleaseAll();

    inline static const cs::String NoRearrangingTheDead = "No use re-arranging a dead companion!";

    cs::String Rearrange(int32_t onto, int32_t from);

    int32_t Settle(cs::List<cs::Ref<Monster>> monsters);

    int32_t SlotOf(cs::Ref<Companion> c);
};

struct CompanionSlotsJson {
    cs::Ref<CompanionSlots> Read(cs::Utf8JsonReader& reader, cs::Type typeToConvert, cs::Ref<cs::JsonSerializerOptions> options);

    void Write(cs::Utf8JsonWriter writer, cs::Ref<CompanionSlots> value, cs::Ref<cs::JsonSerializerOptions> options);
};

}  // namespace mc
