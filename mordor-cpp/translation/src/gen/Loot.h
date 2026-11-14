#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Loot {
    inline static cs::List<cs::String> TypeNames = cs::List<cs::String>::Of({
        "Hands", "Dagger", "Cross", "Sword", "Staff", "Mace", "Axe", "Hammer",
        "Leather Armor", "Chain Armor", "Plate Armor", "Shield", "Cap", "Helmet", "Gloves", "Gauntlets",
        "Cloak", "Bracers", "Sash", "Belt", "Boots", "Ring", "Amulet", "Potion", "Scroll", "Tome",
        "Dust", "Crystal", "Rod", "Stone", "Sphere", "Cube", "Artifact", "Miscellaneous Item", "Guild Crest",
    });

    static cs::String TypeName(cs::Ref<Item> it);
    static cs::String IdLevelName(IdLevel known);
    static cs::String NameFor(cs::Ref<Item> it, IdLevel known, ItemAlignment alignment = ItemAlignment::Unaligned, int32_t charges = 0);
    static cs::String UnknownName(cs::Ref<Item> it);
    static double IdentifyDifficulty(cs::Ref<Item> it);
    static IdLevel ReadableLevel(cs::Ref<Character> c, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static void OnReceived(cs::Ref<Character> receiver, cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static ItemAlignment RollAlignment(cs::Ref<Item> it, cs::Ref<cs::Random> rng);
    static bool AttachesItself(cs::Ref<Item> it);
    static cs::Ref<InventorySlot> NewSlot(cs::Ref<Item> it, cs::Ref<cs::Random> rng, bool identified = false);
    static bool DropsItem(cs::Ref<Monster> lead, int32_t chestClass, cs::Ref<cs::Random> rng, int32_t lockState = 0, int32_t dungeonLevel = 1);
    static bool DropsAnother(cs::Ref<Monster> lead, cs::Ref<cs::Random> rng);
    static cs::Ref<Item> RollDrop(cs::List<cs::Ref<Item>> all, cs::Ref<Monster> lead, int32_t chestClass, cs::Ref<cs::Random> rng);
    static int16_t RollCharges(cs::Ref<Item> it, cs::Ref<cs::Random> rng);
};

}  // namespace mc
