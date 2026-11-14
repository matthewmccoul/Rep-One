#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct ItemUse {
    static constexpr int32_t GuildIssuedBase = 500;
    static constexpr int32_t ItemKind25AgeDays = 28;
    inline static const cs::String SoundPrefix = "sound:";

    static bool IsGuildIssued(cs::Ref<Item> it);
    static cs::String RechargeGuildItems(cs::Ref<Character> c, bool party, cs::Func<cs::Ref<Item>(int16_t)> item);
    static bool MustBeEquippedToUse(cs::Ref<Item> it);
    static bool ConsumedWhenEmpty(cs::Ref<Item> it);
    static cs::String Problem(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> lookup);
    static cs::Ref<Spell> SpellOf(cs::Ref<Item> it, cs::List<cs::Ref<Spell>> spells);
    static int32_t CastLevel(cs::Ref<Item> it, cs::Ref<Character> c);
    static cs::List<cs::String> ApplySpecial(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it,
                                             cs::Ref<HallRecords> hall = nullptr, cs::String savesDir = {},
                                             bool inParty = false, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static bool ChangesStats(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it);
    static cs::String Sound(cs::String name);
    static cs::String SoundIn(cs::String line);
    static cs::String Younger(cs::Ref<Character> c, int32_t years, cs::String howMany);
    static cs::List<cs::String> ApplyStatMods(cs::Ref<Character> c, cs::Ref<Item> it, bool reversed,
                                              cs::Ref<HallRecords> hall, cs::String savesDir, bool inParty);
    static HallRecords_Kind StatRecord(int32_t i);
    static cs::String StatLine(cs::Ref<Character> c, int32_t stat, int32_t delta, bool inParty = false);
    static cs::String DropProblem(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, int32_t packSlot);
    static cs::String CursedRefusal(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, int32_t packIndex);
};

}  // namespace mc
