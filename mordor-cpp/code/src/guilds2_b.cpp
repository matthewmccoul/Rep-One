// guilds2_b.cpp: second half of module guilds2.
// Implements: ItemQuestStarts, MonsterEligible, RollQuest, ItemPickRange, CountKill,
// Recharge, GuildCanUse, MeetsUseLevel, TestGuildmaster, AbilityLeader, AbilityChance,
// GuildLogWrite, GuildTitle.

// GuildLogKind is declared in death1b.h. guilds2.h declares GuildLogWrite with that type,
// so death1b.h must be included first.
#include "mordor/death1b.h"
#include "mordor/guilds2.h"

namespace mordor {

// Guild title for a membership level.
// Only the Nomad Master title is known (kNomadMasterLevel). Other band names are UNKNOWN,
// so an empty string is returned for them.
// UNKNOWN: the Ninja Shogun band (kNinjaShogunLevel) has no title text in this excerpt.
// RESOLVED-BY-REFERENCE: GuildTitles.cs "Bands" (src/Mordor.Core/GuildTitles.cs:16). Each band is
// (Below, Title); the first band with level < Below applies. The last band is open (kOpenBand).
// Needs <cstddef> for std::size_t.
namespace {

struct GuildBand {
    i32 below;
    const char* title;
};

const i32 kOpenBand = 2147483647;  // int.MaxValue in the reference

// UNKNOWN: the switch below assumes the Guild enum follows the guild order of Guildmasters.cs
// Seeded (0 Nomad, 1 Warrior, 2 Paladin, 3 Ninja, 4 Villain, 5 Seeker, 6 Thief, 7 Scavenger,
// 8 Mage, 9 Sorcerer, 10 Wizard, 11 Healer). Confirm against the Guild enum in the header.

const GuildBand kNomadBands[] = {
    {10, "Vagrant"}, {20, "Drifter"}, {30, "Vagabond"}, {40, "Wanderer"}, {50, "Traveller"},
    {60, "Wayfarer"}, {80, "Rover"}, {100, "Pioneer"}, {130, "Pilgrim"}, {150, "Journeyor"},
    {190, "Voyager"}, {220, "Nomad"}, {kOpenBand, "Master Nomad"},
};
const GuildBand kWarriorBands[] = {
    {10, "Bravo"}, {20, "Soldier"}, {30, "Fighter"}, {50, "Mercenary"}, {70, "Swordsman"},
    {90, "Duelist"}, {120, "Gladiator"}, {140, "Combatant"}, {160, "Hero"}, {180, "Swashbuckler"},
    {200, "Master Hero"}, {230, "Master Swordsman"}, {kOpenBand, "Warlord"},
};
const GuildBand kPaladinBands[] = {
    {10, "Guard"}, {20, "Sentry"}, {50, "Defender"}, {80, "Protector"}, {100, "Sentinel"},
    {120, "Guardian"}, {150, "Knight"}, {180, "Champion"}, {210, "Paladin"},
    {kOpenBand, "Master of the Guard"},
};
// The reference lists three 120 entries; "Ninja" and "Master Ninja" are unreachable under the
// first-match rule and are kept only for fidelity.
// CONFIRMED-BY-REFERENCE: SRD T10 ("< 120 Assassin, >= 120 Shogun") is incomplete; the reference
// ladder is used here, and Instructor is below 90 (the SRD leaves it UNKNOWN).
const GuildBand kNinjaBands[] = {
    {10, "Apprentice"}, {20, "Student"}, {40, "Mentor"}, {70, "Teacher"}, {90, "Instructor"},
    {120, "Assassin"}, {120, "Ninja"}, {120, "Master Ninja"}, {150, "Shogun"},
    {220, "Weapons Master"}, {kOpenBand, "Death Master"},
};
const GuildBand kVillainBands[] = {
    {10, "Rascal"}, {20, "Knave"}, {30, "Trickster"}, {40, "Scoundrel"}, {50, "Miscreant"},
    {70, "Criminal"}, {90, "Ruffian"}, {120, "Convict"}, {150, "Murderer"}, {180, "Villain"},
    {210, "Evil Being"}, {kOpenBand, "Master Villain"},
};
const GuildBand kSeekerBands[] = {
    {10, "Apprentice"}, {20, "Journeyman"}, {30, "Guide"}, {40, "Searcher"}, {60, "Tracer"},
    {80, "Hunter"}, {100, "Navigator"}, {120, "Explorer"}, {140, "Infiltrator"}, {160, "Adventurer"},
    {180, "Delver"}, {200, "Questor"}, {230, "Seeker"}, {kOpenBand, "Master Seeker"},
};
const GuildBand kThiefBands[] = {
    {10, "Footpad"}, {20, "Sneak"}, {30, "Pickpocket"}, {40, "Trickster"}, {50, "Cutpurse"},
    {60, "Robber"}, {80, "Swindler"}, {100, "Shyster"}, {120, "Convict"}, {140, "Criminal"},
    {160, "Thief"}, {180, "Racketeer"}, {200, "Kingpin"}, {240, "Master Thief"},
    {kOpenBand, "Master of Shadows"},
};
const GuildBand kScavengerBands[] = {
    {10, "Rogue"}, {20, "Scrounger"}, {30, "Forager"}, {40, "Acquirer"}, {70, "Plunderer"},
    {100, "Pillager"}, {140, "Ransacker"}, {180, "Marauder"}, {220, "Mugger"}, {285, "Scavenger"},
    {kOpenBand, "Master Scavenger"},
};
const GuildBand kMageBands[] = {
    {10, "Apprentice"}, {30, "Enchanter"}, {50, "Charmer"}, {80, "Conjuror"}, {120, "Illusionist"},
    {150, "Magician"}, {180, "Mage"}, {230, "Spellbinder"}, {260, "Arch-Mage"},
    {kOpenBand, "Master of Magic"},
};
const GuildBand kSorcererBands[] = {
    {10, "Alchemist"}, {30, "Warlock"}, {50, "Sorcerer"}, {70, "Necromancer"}, {90, "Element Mage"},
    {120, "Fire Master"}, {150, "Master of Electricity"}, {170, "Earth Shaker"},
    {240, "Master Sorcerer"}, {kOpenBand, "Master of Elements"},
};
const GuildBand kWizardBands[] = {
    {10, "Prophet"}, {40, "Diviner"}, {70, "SoothSayer"}, {100, "Theurgist"}, {130, "Thaumaturgist"},
    {160, "Wizard"}, {190, "Adept"}, {230, "Master Wizard"}, {kOpenBand, "Master Adept"},
};
const GuildBand kHealerBands[] = {
    {10, "Apprentice"}, {30, "Medic"}, {60, "Benefactor"}, {90, "Altruist"}, {120, "Healer"},
    {150, "Shaman"}, {200, "Magus"}, {250, "Raiser of the Dead"}, {kOpenBand, "Master Healer"},
};

template <std::size_t N>
Str bandTitle(i32 level, const GuildBand (&bands)[N]) {
    for (std::size_t i = 0; i < N; ++i) {
        if (level < bands[i].below) {
            return Str(bands[i].title);
        }
    }
    return Str("");
}

}  // namespace

Str GuildTitle(const GuildMembership& m, Guild guild) {
    switch (static_cast<i32>(guild)) {
        case 0:  return bandTitle(m.level, kNomadBands);
        case 1:  return bandTitle(m.level, kWarriorBands);
        case 2:  return bandTitle(m.level, kPaladinBands);
        case 3:  return bandTitle(m.level, kNinjaBands);
        case 4:  return bandTitle(m.level, kVillainBands);
        case 5:  return bandTitle(m.level, kSeekerBands);
        case 6:  return bandTitle(m.level, kThiefBands);
        case 7:  return bandTitle(m.level, kScavengerBands);
        case 8:  return bandTitle(m.level, kMageBands);
        case 9:  return bandTitle(m.level, kSorcererBands);
        case 10: return bandTitle(m.level, kWizardBands);
        case 11: return bandTitle(m.level, kHealerBands);
        default: return Str("");
    }
}

}  // namespace mordor
