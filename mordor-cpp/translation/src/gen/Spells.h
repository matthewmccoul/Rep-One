#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Spellbook_SpellRow {
    cs::Ref<mc::Spell> Spell;
    int32_t SpellIndex = 0;
    int32_t Guild = 0;
    int32_t Level = 0;
    int32_t Points = 0;
    cs::String GuildText();
    cs::String PointsText();
};

struct Spellbook_Castable {
    cs::Ref<mc::Spell> Spell;
    cs::Ref<InventorySlot> Slot;
    cs::Ref<mc::Item> Item;
    int32_t SpCost = 0;
    bool FromItem();
};

struct Spellbook {
    using SpellRow = Spellbook_SpellRow;
    using Castable = Spellbook_Castable;

    static constexpr int32_t Fire = 0;
    static constexpr int32_t Cold = 1;
    static constexpr int32_t Electric = 2;
    static constexpr int32_t Light = 3;
    static constexpr int32_t Wounds = 4;
    static constexpr int32_t Poison = 5;
    static constexpr int32_t Death = 6;
    static constexpr int32_t Charm = 7;
    static constexpr int32_t Control = 8;
    static constexpr int32_t Healing = 9;
    static constexpr int32_t Movement = 10;
    static constexpr int32_t Banish = 11;
    static constexpr int32_t Dispel = 12;
    static constexpr int32_t Resist = 13;
    static constexpr int32_t Sight = 14;
    static constexpr int32_t Opening = 15;
    static constexpr int32_t ClassCount = 18;
    inline static const cs::String NoSpellClasses = "NO SPELLS";
    inline static const cs::String NoSpellsYet = "No Spells Yet";
    inline static const cs::String CastOnWho = "Cast on Who?";
    inline static const cs::String CompanionIsDead = "Companion is Dead!";
    inline static int32_t TrainingGuild = 0;

    static cs::List<int32_t> SpellClasses(cs::Ref<Character> c);
    static cs::List<cs::Ref<Spellbook_SpellRow>> ClassSpells(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all, int32_t kind);
    static bool ShownBold(cs::Ref<Character> c, cs::Ref<Spell> s, cs::Func<cs::Ref<Item>(int16_t)> item);
    static bool IsHealing(cs::Ref<Spell> s);
    static bool IsCharm(cs::Ref<Spell> s);
    static bool IsCure(cs::Ref<Spell> s);
    static bool IsCureParalysis(cs::Ref<Spell> s);
    static bool IsUtility(cs::Ref<Spell> s);
    static bool IsBinding(cs::Ref<Spell> s);
    static int32_t OpensLockUpTo(cs::Ref<Spell> s);
    static bool IsOffensive(cs::Ref<Spell> s);
    static bool IsGroup(cs::Ref<Spell> s);
    static int32_t Cost(cs::Ref<Spell> s, cs::Ref<Character> c);
    static cs::List<Tup_Index_Def_Level_2e78f> ReachingGuilds(cs::Ref<Character> c, cs::Ref<Spell> s);
    static int32_t TeachingGuild(cs::Ref<Character> c, cs::Ref<Spell> s);
    static int32_t CasterSpellLevel(cs::Ref<Character> c, cs::Ref<Spell> s);
    static int32_t SpellLevelFor(cs::Ref<Character> c, cs::Ref<Spell> s);
    static int32_t FindByName(cs::List<cs::Ref<Spell>> all, cs::String typed, cs::Func<void(cs::String)> say);
    static cs::List<cs::Ref<Spell>> Known(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all);
    static cs::List<cs::Ref<Spellbook_Castable>> Castables(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all, cs::Func<cs::Ref<Item>(int16_t)> item);
    static bool IsConsumable(cs::Ref<Item> it);
    static bool SpendCharge(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it);
    static cs::Ref<Spell> BestHealing(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all);
    static int32_t MaxSpellPoints(cs::Ref<Character> c);
};

}  // namespace mc
