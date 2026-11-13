// guilds2_b.cpp: second half of module guilds2.
// Implements: ItemQuestStarts, MonsterEligible, RollQuest, ItemPickRange, CountKill,
// Recharge, GuildCanUse, MeetsUseLevel, TestGuildmaster, AbilityLeader, AbilityChance,
// GuildLogWrite, GuildTitle.

// GuildLogKind is declared in death1b.h. guilds2.h declares GuildLogWrite with that type,
// so death1b.h must be included first.
#include "mordor/death1b.h"
#include "mordor/guilds2.h"

namespace mordor {

// R12: item (fetch) quests start only when guildLevel > 15.
// The engine governs; the wiki's "begin at gLevel 15" is not adopted.
bool ItemQuestStarts(i32 guildLevel) {
    return guildLevel > kItemQuestMinGuildLevel;
}

// R10: a monster is eligible while Floor <= tier, and tier <= 10.
// UNKNOWN: behaviour when tier > 10. Here tier is capped at kQuestTierMax (the wiki's min(.,10)).
bool MonsterEligible(i32 monsterFloor, i32 tier) {
    const i32 cap = tier > kQuestTierMax ? kQuestTierMax : tier;
    return monsterFloor <= cap;
}

// R6, R7, R9, R11, R12, R13: roll a guild quest for the membership's current level.
// UNKNOWN: the procedure is not fully in this SRD excerpt. Assumptions are marked below.
// UNKNOWN: MonsterDef and ItemDef field names are not in this excerpt, so the R7 weight
//          reroll and the R10 floor filter are NOT applied. The target is a uniform index.
GuildQuest RollQuest(Ctx& ctx, const Character& ch, Guild guild) {
    GuildQuest q{};  // kind None, state Active, count 0
    const i32 gi = static_cast<i32>(guild);
    const i32 level = ch.guilds[static_cast<u32>(gi)].level;

    // RESOLVED-BY-REFERENCE: Rules.cs RollQuest: "roll = (guildLevel < 5) ? 100 : Roll();
    // if (roll > guild.QuestPercent) return (0, 0)". The gate value is the per-guild QuestPercent.
    // Below level 5 no draw is taken, because the reference ternary never evaluates Roll().
    i64 gateRoll = 100;
    if (level >= 5) {
        gateRoll = roundHalfEven(ctx.rng.nextDouble() * 100.0);
    }
    if (gateRoll > static_cast<i64>(kQuestPercent[gi])) {
        return q;
    }

    // R6 / reference "if (Roll() <= 5) return (-1, tier)": let-off when the rounded draw <= 5 (5.5%).
    // CONFIRMED-BY-REFERENCE: the draw is round_half_even(Rnd*100); kLetOffMaxDraw is the constant 5.
    const i64 draw = roundHalfEven(ctx.rng.nextDouble() * 100.0);
    if (draw <= kLetOffMaxDraw) {
        return q;
    }

    // R9: quest tier = round_half_even(level/25), capped by QuestTier.
    const i32 tier = QuestTier(level);
    q.level = tier;

    // RESOLVED-BY-REFERENCE: Rules.cs RollQuest: "type = Roll() < 25 && guildLevel > 15 ? fetch : defeat".
    // The type draw is a fresh roll taken before the level test, so it is always consumed.
    const i64 typeRoll = roundHalfEven(ctx.rng.nextDouble() * 100.0);
    const bool fetch = (typeRoll < 25) && (level > 15);
    if (fetch) {
        q.kind = QuestKind::Fetch;
        // R11: item quest band.
        q.level = ItemQuestBand(tier);
        const i32 n = static_cast<i32>(ctx.data.items.size());
        if (n <= 0) {
            q.kind = QuestKind::None;
            return q;
        }
        q.target = static_cast<i32>(ctx.rng.below(n));
    } else {
        q.kind = QuestKind::Defeat;
        const i32 n = static_cast<i32>(ctx.data.monsters.size());
        if (n <= 0) {
            q.kind = QuestKind::None;
            return q;
        }
        // UNKNOWN: R7 reroll and R10 floor filter not applied (see header note above).
        q.target = static_cast<i32>(ctx.rng.below(n));
    }
    return q;
}

// R8: item pick range is first+1..last. If the span (last - first) is 0, the range is first+1.
// The engine formula is used, not the c59 reading first+span+1.
IndexRange ItemPickRange(i32 first, i32 last) {
    IndexRange r{};
    const i32 span = last - first;
    r.first = first + 1;
    r.last = (span == 0) ? first + 1 : last;
    return r;
}

// Quest kill counting. Only the killer counts (see Unknowns: non-killer holder behaviour).
// UNKNOWN: the kill-quest target is matched by creature.id.
// UNKNOWN: completion threshold and the kill tally (kills / monsterKills) are not in this excerpt,
//          so no completion is set here.
void CountKill(Ctx& ctx, Character& killer, const Creature& creature) {
    for (i32 g = 0; g < kGuildCount; ++g) {
        GuildMembership& m = killer.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        if (m.quest.kind != QuestKind::Defeat) {
            continue;
        }
        if (m.quest.state != QuestState::Active) {
            continue;
        }
        if (m.quest.target != creature.id) {
            continue;
        }
        m.quest.count += 1;
        ctx.say("guild.kill_counted",
                "Kill counted toward guild quest",
                {static_cast<i32>(g), m.quest.count},
                {});
    }
}

// UNKNOWN: the recharge amount is not in this SRD excerpt. Charges are restored to maxCharges.
void Recharge(Item& item) {
    item.charges = item.maxCharges;
}

// Guild usability of an item.
// Guild-marked items: usable only by a character who has joined that item's guild.
// UNKNOWN: this guild-item rule is not stated in this excerpt.
// Other items: defer to MeetsUseLevel (R14).
// CONFLICT: the port's CanUse checks the primary guild only (c34). R14 says any joined guild.
//           The source's any-joined rule is used here, per R14.
bool GuildCanUse(Ctx& ctx, const Character& ch, const Item& item) {
    // R13 / ch13: class-restricted (guild) items use the primary (active) guild only [c10].
    // The joined-guild loop applies only to non-restricted items; level gating is in MeetsUseLevel.
    if (item.isGuildItem()) {
        const i32 g = item.guildIndex();
        if (g < 0 || g >= kGuildCount) {
            return false;
        }
        return static_cast<i32>(ch.activeGuild) == g;
    }
    return MeetsUseLevel(ctx, ch, item);
}

// R14: any joined guild that meets the use level and the item's alignment bitmask may use the item.
// The bitmask is taken as item.alignFlags tested against the character's alignment bit, when the
// item is aligned. UNKNOWN: item.level is taken as UseLevel, and the comparison is level >= UseLevel.
// UNKNOWN: behaviour for a character holding no guild (returns false here).
bool MeetsUseLevel(Ctx& ctx, const Character& ch, const Item& item) {
    (void)ctx;  // no event is emitted by this check
    if (item.aligned) {
        const u8 alignBits[3] = {kAlignBitGood, kAlignBitNeutral, kAlignBitEvil};
        const u8 mine = alignBits[static_cast<i32>(ch.alignment)];
        if ((item.alignFlags & mine) == 0) {
            return false;
        }
    }
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = ch.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        if (m.level >= item.level) {
            return true;
        }
    }
    return false;
}

// R24, R25: guildmaster test.
// R25: no claim if held.Level >= level. Otherwise the master's level and name are set to the
// claimant's, and the holder index is set when the claimant is found in the roster.
// UNKNOWN: membership precondition (joined) is assumed.
// UNKNOWN: crest placement (R24) is NOT done here. Character has no pack, so the crest cannot be
//          placed from this function. The caller must handle it.
// UNKNOWN: whether the master's level is the sole takeover input (see Unknowns).
void TestGuildmaster(Ctx& ctx, GameState& game, Guild guild, Character& ch) {
    const u32 g = static_cast<u32>(guild);
    const GuildMembership& m = ch.guilds[g];
    if (!m.joined) {
        return;
    }
    GuildMaster& gm = game.guildMasters[g];
    // R25: no claim if held.Level >= level.
    if (gm.level >= m.level) {
        return;
    }
    gm.level = m.level;
    gm.name = ch.name;
    // UNKNOWN: holder index. The claimant is located by identity in the roster.
    // If it is not there (for example, a party member), holder is left unchanged.
    for (u32 i = 0; i < game.roster.size(); ++i) {
        if (&game.roster[i] == &ch) {
            gm.holder = static_cast<i32>(i);
            break;
        }
    }
    ctx.say("guild.guildmaster_claim",
            "Guildmaster level claimed",
            {static_cast<i32>(g), m.level},
            {ch.name});
}

// Leading joined guild for an ability: highest kGuildAbilityRating among joined guilds.
// UNKNOWN: ties go to the lower guild index. found is false when no joined guild has rating > 0.
// UNKNOWN: whether the static rating table or the membership's stored abilityRating is the
//          intended source. The static table is used here.
GuildChoice AbilityLeader(const Character& ch, GuildAbility ability) {
    GuildChoice choice{};
    const u32 a = static_cast<u32>(ability);
    f32 best = 0.0f;
    for (i32 g = 0; g < kGuildCount; ++g) {
        if (!ch.guilds[static_cast<u32>(g)].joined) {
            continue;
        }
        const f32 r = kGuildAbilityRating[g][a];
        if (r > best) {
            best = r;
            choice.found = true;
            choice.guild = static_cast<Guild>(g);
        }
    }
    return choice;
}

// Chance for an ability, taken from the leading guild's rating.
// The Fighting rating is divided by 10 ("Fighting is ÷10 in the engine").
// UNKNOWN: units and cap of skill ratings as a chance (see Unknowns: skill rating values).
f32 AbilityChance(const Character& ch, GuildAbility ability) {
    const GuildChoice lead = AbilityLeader(ch, ability);
    if (!lead.found) {
        return 0.0f;
    }
    f32 v = kGuildAbilityRating[static_cast<u32>(lead.guild)][static_cast<u32>(ability)];
    if (ability == GuildAbility::Fighting) {
        v = v / 10.0f;
    }
    return v;
}

// Appends one entry to the guild log, stamped with game.today.
// UNKNOWN: trimming to guildLogKeep and setting needsCleaning are not stated here, so they are not done.
void GuildLogWrite(GameState& game, GuildLogKind kind, Guild guild, const Str& text) {
    GuildLogEvent ev{};
    ev.day = game.today;
    ev.guild = guild;
    ev.kind = static_cast<i32>(kind);
    ev.text = text;
    game.guildLog.push_back(ev);
}

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
