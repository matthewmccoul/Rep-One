#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GuildTitles {
    inline static cs::Dictionary<cs::String, cs::List<Tup_Below_Title_e8866>> Bands = cs::Dictionary<cs::String, cs::List<Tup_Below_Title_e8866>>::Of({
        {"Nomad", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Vagrant"}, {20, "Drifter"}, {30, "Vagabond"}, {40, "Wanderer"}, {50, "Traveller"},
            {60, "Wayfarer"}, {80, "Rover"}, {100, "Pioneer"}, {130, "Pilgrim"}, {150, "Journeyor"},
            {190, "Voyager"}, {220, "Nomad"}, {INT32_MAX, "Master Nomad"},
        })},
        {"Warrior", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Bravo"}, {20, "Soldier"}, {30, "Fighter"}, {50, "Mercenary"}, {70, "Swordsman"},
            {90, "Duelist"}, {120, "Gladiator"}, {140, "Combatant"}, {160, "Hero"}, {180, "Swashbuckler"},
            {200, "Master Hero"}, {230, "Master Swordsman"}, {INT32_MAX, "Warlord"},
        })},
        {"Paladin", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Guard"}, {20, "Sentry"}, {50, "Defender"}, {80, "Protector"}, {100, "Sentinel"},
            {120, "Guardian"}, {150, "Knight"}, {180, "Champion"}, {210, "Paladin"},
            {INT32_MAX, "Master of the Guard"},
        })},
        // the three 120s are in the original; "Ninja" and "Master Ninja" can never be reached
        {"Ninja", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Apprentice"}, {20, "Student"}, {40, "Mentor"}, {70, "Teacher"}, {90, "Instructor"},
            {120, "Assassin"}, {120, "Ninja"}, {120, "Master Ninja"}, {150, "Shogun"},
            {220, "Weapons Master"}, {INT32_MAX, "Death Master"},
        })},
        {"Villain", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Rascal"}, {20, "Knave"}, {30, "Trickster"}, {40, "Scoundrel"}, {50, "Miscreant"},
            {70, "Criminal"}, {90, "Ruffian"}, {120, "Convict"}, {150, "Murderer"}, {180, "Villain"},
            {210, "Evil Being"}, {INT32_MAX, "Master Villain"},
        })},
        {"Seeker", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Apprentice"}, {20, "Journeyman"}, {30, "Guide"}, {40, "Searcher"}, {60, "Tracer"},
            {80, "Hunter"}, {100, "Navigator"}, {120, "Explorer"}, {140, "Infiltrator"}, {160, "Adventurer"},
            {180, "Delver"}, {200, "Questor"}, {230, "Seeker"}, {INT32_MAX, "Master Seeker"},
        })},
        {"Thief", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Footpad"}, {20, "Sneak"}, {30, "Pickpocket"}, {40, "Trickster"}, {50, "Cutpurse"},
            {60, "Robber"}, {80, "Swindler"}, {100, "Shyster"}, {120, "Convict"}, {140, "Criminal"},
            {160, "Thief"}, {180, "Racketeer"}, {200, "Kingpin"}, {240, "Master Thief"},
            {INT32_MAX, "Master of Shadows"},
        })},
        {"Scavenger", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Rogue"}, {20, "Scrounger"}, {30, "Forager"}, {40, "Acquirer"}, {70, "Plunderer"},
            {100, "Pillager"}, {140, "Ransacker"}, {180, "Marauder"}, {220, "Mugger"}, {285, "Scavenger"},
            {INT32_MAX, "Master Scavenger"},
        })},
        {"Mage", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Apprentice"}, {30, "Enchanter"}, {50, "Charmer"}, {80, "Conjuror"}, {120, "Illusionist"},
            {150, "Magician"}, {180, "Mage"}, {230, "Spellbinder"}, {260, "Arch-Mage"},
            {INT32_MAX, "Master of Magic"},
        })},
        {"Sorcerer", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Alchemist"}, {30, "Warlock"}, {50, "Sorcerer"}, {70, "Necromancer"}, {90, "Element Mage"},
            {120, "Fire Master"}, {150, "Master of Electricity"}, {170, "Earth Shaker"}, {240, "Master Sorcerer"},
            {INT32_MAX, "Master of Elements"},
        })},
        {"Wizard", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Prophet"}, {40, "Diviner"}, {70, "SoothSayer"}, {100, "Theurgist"}, {130, "Thaumaturgist"},
            {160, "Wizard"}, {190, "Adept"}, {230, "Master Wizard"}, {INT32_MAX, "Master Adept"},
        })},
        {"Healer", cs::List<Tup_Below_Title_e8866>::Of({
            {10, "Apprentice"}, {30, "Medic"}, {60, "Benefactor"}, {90, "Altruist"}, {120, "Healer"},
            {150, "Shaman"}, {200, "Magus"}, {250, "Raiser of the Dead"}, {INT32_MAX, "Master Healer"},
        })},
    });

    static cs::String Of(cs::String guild, int32_t level);
};

}  // namespace mc
