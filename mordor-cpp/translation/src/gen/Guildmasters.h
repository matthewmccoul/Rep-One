#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Guildmasters_Holder {
    cs::String Name = "";
    int32_t Level = 0;
};

struct Guildmasters {
    using Holder = Guildmasters_Holder;
    using Award = Guildmasters_Award;

    static constexpr int32_t CrestIdBase = 299;

    static constexpr int32_t PackSlots = 40;

    cs::Dictionary<int32_t, cs::Ref<Guildmasters_Holder>> Held = cs::Dictionary<int32_t, cs::Ref<Guildmasters_Holder>>::New();

    inline static cs::List<Tup_Name_Level_7f827> Seeded = cs::List<Tup_Name_Level_7f827>::Of({
        Tup_Name_Level_7f827{"Crashland", 64},          // Nomad
        Tup_Name_Level_7f827{"Garth SliceMonger", 56},  // Warrior
        Tup_Name_Level_7f827{"Eli", 62},                // Paladin
        Tup_Name_Level_7f827{"Shogun", 48},             // Ninja
        Tup_Name_Level_7f827{"Maxx", 67},               // Villain
        Tup_Name_Level_7f827{"Martin", 77},             // Seeker
        Tup_Name_Level_7f827{"Nimblefingers", 56},      // Thief
        Tup_Name_Level_7f827{"Kane the Wise Axe", 54},  // Scavenger
        Tup_Name_Level_7f827{"Sparrowhawk", 52},        // Mage
        Tup_Name_Level_7f827{"Requnix", 61},            // Sorcerer
        Tup_Name_Level_7f827{"Merlin", 48},             // Wizard
        Tup_Name_Level_7f827{"Hands on Henry", 65},     // Healer
    });

    int32_t Seed(cs::String savesDir);
    static cs::Ref<Guildmasters> LoadSeeded(cs::String savesDir);
    cs::Ref<Guildmasters_Holder> For(int32_t guild);
    cs::String Describe(int32_t guild, cs::Ref<Character> viewer);
    static Tup_qualified_changedHands_6bbff Claim(cs::Ref<Guildmasters> table, int32_t guild, cs::Ref<Character> c);
    static cs::Ref<Item> CrestFor(cs::List<cs::Ref<Item>> items, int32_t guild);
    static Tup_award_message_f842a AwardCrest(cs::Ref<Character> c, int32_t guild, cs::List<cs::Ref<Item>> items);
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<Guildmasters> Load(cs::String savesDir);
    void Save(cs::String savesDir);
};

}  // namespace mc
