// guilds3.cpp: implements include/mordor/guilds3.h (SRD Chapter 4, part c04p3).
// UNKNOWN markers flag values the SRD excerpt does not state.
// CONFLICT markers flag source disagreements; the engine rule is applied unless noted.
#include "mordor/guilds3.h"

namespace mordor {

namespace {

constexpr i32 kAbilityRatingCount = 6;  // Arr<f32, 6> abilityRating in GuildMembership

bool validGuild(i32 guild) {
    return guild >= 0 && guild < kGuildCount;
}

// UNKNOWN: per-guild max level is not fully in the excerpt. Carried values:
// Nomad 30 [c88][c104], Sorcerer 29 [c82][c91][c104], Wizard 41 [c82][c87][c104].
// Other guilds use a 0 placeholder.
i32 guildMaxLevel(i32 guild) {
    switch (static_cast<Guild>(guild)) {
        case Guild::Nomad:    return 30;
        case Guild::Sorcerer: return 29;  // CONFLICT: 28 [c87] vs 29; majority 29 used.
        case Guild::Wizard:   return 41;  // CONFLICT: 29 [c88] vs 41; 41 used.
        default:              return 0;   // UNKNOWN
    }
}

// UNKNOWN: AH (hits gain below ML) is per-guild engine data not in the excerpt. Placeholder 0.
i32 guildAh(i32 guild) {
    (void)guild;
    return 0;
}

// UNKNOWN: MH (hits gain at or above ML) is per-guild engine data not in the excerpt. Placeholder 0.
i32 guildMh(i32 guild) {
    (void)guild;
    return 0;
}

}  // namespace

// [c49] Cost is 0 unless the character already holds another guild.
// CONFLICT: wiki says first extra guild is free [c83][c102]; engine governs [c49].
Currency GuildJoinCost(const Ctx& ctx, const Character& c) {
    (void)ctx;
    bool heldAny = false;
    for (i32 g = 0; g < kGuildCount; ++g) {
        if (c.guilds[static_cast<u32>(g)].joined) {
            heldAny = true;
        }
    }
    if (!heldAny) {
        return 0;
    }
    // UNKNOWN: priced amount when another guild is already held is not in the excerpt. 0 used.
    return 0;
}

// UNKNOWN: worn items live in Inventory, not Character, so cursed flags cannot be read here.
// Returns false (no cursed worn gear).
bool HasCursedWorn(const Character& c) {
    (void)c;
    return false;
}

// [c49][c30] Join refused while wearing cursed gear (engine governs).
// Re-joining a held guild is free [c49].
bool JoinGuild(Ctx& ctx, Character& c, i32 guild) {
    if (!validGuild(guild)) {
        return false;
    }
    GuildMembership& m = c.guilds[static_cast<u32>(guild)];
    if (m.joined) {
        // Re-joining a held guild is free [c49].
        // RESOLVED-BY-REFERENCE: SRD ch04 R13 step 1: "It is free and only makes the guild active".
        c.activeGuild = static_cast<Guild>(guild);
        return true;
    }
    if (HasCursedWorn(c)) {
        // CONFLICT: refusal text differs between audit [c30] and port [c49]; generic text used.
        ctx.say("guild.join.cursed", "You cannot join a guild while wearing cursed items.",
                {}, {});
        return false;
    }
    Currency cost = GuildJoinCost(ctx, c);
    if (cost > 0) {
        if (c.wallet.goldOnHand < cost) {
            ctx.say("guild.join.no_gold", "You cannot afford to join this guild.", {}, {});
            return false;
        }
        c.wallet.goldOnHand -= cost;
    }
    m.joined = true;
    // RESOLVED-BY-REFERENCE: SRD ch04 R13 step 7: "insert {Guild, Level = 1, Experience = 0}".
    m.level = 1;
    m.experience = 0;
    c.activeGuild = static_cast<Guild>(guild);
    ctx.say("guild.joined", "You have joined the guild.", {static_cast<i64>(guild)}, {});
    return true;
}

// UNKNOWN: meaning of "re-acquaint" (to reactivate a held guild) and its refusal are not
// stated [c49][c104]. Simplest sensible rule: a held guild becomes active.
bool ReacquaintGuild(Ctx& ctx, Character& c, i32 guild) {
    if (!validGuild(guild)) {
        return false;
    }
    if (!c.guilds[static_cast<u32>(guild)].joined) {
        return false;
    }
    c.activeGuild = static_cast<Guild>(guild);
    ctx.say("guild.reacquainted", "You are again active in this guild.",
            {static_cast<i64>(guild)}, {});
    return true;
}

// Test only: Visit, Form_Load and end-of-level test availability [c49][c30].
// UNKNOWN: the guild XP threshold table (GuildData.cs:73-112) is not in the excerpt.
// Availability is tested as "joined" only.
bool LevelAvailable(const Ctx& ctx, const Character& c, i32 guild) {
    (void)ctx;
    if (!validGuild(guild)) {
        return false;
    }
    return c.guilds[static_cast<u32>(guild)].joined;
}

// Guildmaster re-offer. Sitting master is re-offered when level has risen above the
// recorded level [c11]. Holder identity by name is an UNKNOWN simplification.
bool GuildmasterReoffer(const Ctx& ctx, const GameState& game, i32 guild, const Character& c) {
    (void)ctx;
    if (!validGuild(guild)) {
        return false;
    }
    const GuildMaster& gm = game.guildMasters[static_cast<u32>(guild)];
    const GuildMembership& m = c.guilds[static_cast<u32>(guild)];
    if (!m.joined) {
        return false;
    }
    if (gm.holder < 0 || !(gm.name == c.name)) {
        return false;
    }
    // Strictly above recorded level; equal level gives no claim [c11].
    return m.level > gm.level;
}

// round_half_even(level/25), capped at kQuestTierCap [c19]. Wiki "rounded down" overridden [c97].
i32 QuestTier(i32 guildLevel) {
    i64 t = roundHalfEven(static_cast<double>(guildLevel) / 25.0);
    if (t > kQuestTierCap) {
        t = kQuestTierCap;
    }
    // Reference Rules.QuestTier (FRMCTYG.BAS:471-477): clamp(round_half_even(level/25), 1, 10). SRD ch04 R46 and
    // T11 say the same; R9 omits the lower clamp, which is an SRD inconsistency (see SRD_ERRATA.md).
    if (t < 1) {
        t = 1;
    }
    return static_cast<i32>(t);
}

// Let-off when the rounded draw is at most 5 [c19]. Gives 5.5% (see AssignQuest).
// CONFLICT: audit says no quest at 5 [c24]; wiki 6% [c97]; engine governs [c19][c2].
bool QuestLetOff(i32 roll) {
    return roll <= kLetOffMaxRoll;
}

// Rare monster (weight <= 9) or rare item (weight <= 4) is rejected with 75% probability,
// keeping 25% [c19][c59]. Wiki 74% [c97] overridden. Non-rare entries are never rejected.
// Returns true when the pick should be rerolled. The draw happens only for rare entries.
// CONFLICT: wording "turned down with 25%" [c59] conflicts with its formula; formula governs.
bool QuestRerollRare(Ctx& ctx, i32 weight, bool isItem) {
    const i32 maxWeight = isItem ? kRareItemMaxWeight : kRareMonsterMaxWeight;
    if (weight > maxWeight) {
        return false;
    }
    return ctx.rng.nextDouble() < 0.75;
}

// Item fetch offered when guild level is above 15 [c19]. Wiki "begin at 15" [c97] overridden.
bool ItemFetchOffered(i32 guildLevel) {
    return guildLevel > kItemFetchMinGuildLevel;
}

// Item quest band: Int(tier/1.5)+1 [c19]. Wiki unrounded form [c97] overridden.
i32 ItemQuestBand(i32 tier) {
    return static_cast<i32>(vbInt(static_cast<double>(tier) / 1.5)) + 1;
}

// Pick range first+1 .. last. Span 0 gives first+1 [c19]. The "one past the end" reading
// [c59] is rejected in favour of the formula.
i32 PickItemIndex(Ctx& ctx, i32 first, i32 last) {
    i32 span = last - first;
    if (span < 0) {
        span = 0;  // UNKNOWN: last < first is not in the excerpt
    }
    return static_cast<i32>(first + 1 + ctx.rng.below(span));
}

// Quest assignment [c2][c19][c59]. Quests are rolled at each level, not fixed [c96] overridden.
// Draw: nextDouble*100, rounded half-even. Let-off when the rounded value is at most 5 (5.5%).
// UNKNOWN: draw scale (0..100 chosen so 5.5% matches) and the defeat/fetch split (50/50 used).
// UNKNOWN: monster target (Floor <= tier) and item target need tables not in the excerpt;
//          target is placeholder 0. Companion quests UNKNOWN [c104].
bool AssignQuest(Ctx& ctx, Character& c, i32 guild) {
    if (!validGuild(guild)) {
        return false;
    }
    GuildMembership& m = c.guilds[static_cast<u32>(guild)];
    if (!m.joined) {
        return false;
    }
    const i32 tier = QuestTier(m.level);
    const i32 roll = static_cast<i32>(roundHalfEven(ctx.rng.nextDouble() * 100.0));

    GuildQuest q{};
    if (QuestLetOff(roll)) {
        q.kind = QuestKind::None;
        q.target = -1;  // let-off (-1) [c19]
        m.quest = q;
        ctx.say("guild.quest.let_off", "The guildmaster has no task for you.",
                {static_cast<i64>(guild)}, {});
        return true;
    }

    if (ItemFetchOffered(m.level) && ctx.rng.below(2) == 0) {
        q.kind = QuestKind::Fetch;
        q.level = ItemQuestBand(tier);
    } else {
        q.kind = QuestKind::Defeat;
        q.level = tier;
    }
    q.target = 0;  // UNKNOWN: target selection needs the monster or item table
    q.count = 0;
    q.state = QuestState::Active;
    q.pinned = false;
    m.quest = q;
    m.questDone = false;
    ctx.say("guild.quest.assigned", "The guildmaster has a task for you.",
            {static_cast<i64>(guild), static_cast<i64>(q.level)}, {});
    return true;
}

// Only the killer's quest counts [c65]. Kill XP recipient UNKNOWN [c3][c65].
// UNKNOWN: target keyed by creature id; kill count needed for completion (one kill assumed).
void OnMonsterKilled(Ctx& ctx, Character& killer, const Creature& creature) {
    for (i32 g = 0; g < kGuildCount; ++g) {
        GuildMembership& m = killer.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        GuildQuest& q = m.quest;
        if (q.kind != QuestKind::Defeat || q.state != QuestState::Active) {
            continue;
        }
        if (q.target != creature.id) {
            continue;
        }
        q.count += 1;
        q.state = QuestState::Done;
        m.questDone = true;
        ctx.say("guild.quest.defeat_done", "You have completed your task.",
                {static_cast<i64>(g)}, {});
    }
}

// Item use [c2][c34]. Guild-tied items require their guild joined (preferred).
// CONFLICT: port checks primary guild only [c34]; original allowed any joined guild for
// items not tied to one guild [c34]. Non-guild items: any joined guild [c2].
bool CanUse(const Ctx& ctx, const Item& item, const Character& c) {
    (void)ctx;
    if (item.isGuildItem()) {
        const i32 gi = item.guildIndex();
        if (!validGuild(gi)) {
            return false;
        }
        return c.guilds[static_cast<u32>(gi)].joined;
    }
    for (i32 g = 0; g < kGuildCount; ++g) {
        if (c.guilds[static_cast<u32>(g)].joined) {
            return true;
        }
    }
    return false;
}

// Engine: any joined guild that meets UseLevel and bitmask [c2].
// UNKNOWN: UseLevel source and bitmask are not in the excerpt. item.level used as the
// required guild level, and the bitmask is not checked.
bool MeetsUseLevel(const Ctx& ctx, const Item& item, const Character& c) {
    (void)ctx;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = c.guilds[static_cast<u32>(g)];
        if (m.joined && m.level >= item.level) {
            return true;
        }
    }
    return false;
}

// Picks one guild for an ability [c5]: the joined guild with the highest rating for that
// ability. Ties go to the lowest guild index. Returns -1 (kCarryNone) when none qualify.
// UNKNOWN: tie-break and the ability indexing are not in the excerpt.
i32 WinningGuild(const Character& c, i32 ability) {
    if (ability < 0 || ability >= kAbilityRatingCount) {
        return -1;
    }
    i32 best = -1;
    f32 bestVal = 0.0f;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = c.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        const f32 v = m.abilityRating[static_cast<u32>(ability)];
        if (best < 0 || v > bestVal) {
            best = g;
            bestVal = v;
        }
    }
    return best;
}

// Forfeit [c24][c104]. Resets XP to the new level floor and rechecks equipment [c24].
// UNKNOWN: new level floor value (0 used); removal of gained A/D and abilities [c104];
// demotion amount and the guild log entry [c64][c3]; IsPinned trigger [c49].
void ForfeitGuild(Ctx& ctx, Character& c, i32 guild) {
    if (!validGuild(guild)) {
        return;
    }
    GuildMembership& m = c.guilds[static_cast<u32>(guild)];
    if (!m.joined) {
        return;
    }
    m.experience = 0;
    m.joined = false;
    m.quest = GuildQuest{};
    // UNKNOWN: whether forfeiting the active guild falls back to Nomad.
    if (c.activeGuild == static_cast<Guild>(guild)) {
        c.activeGuild = Guild::Nomad;
    }
    ctx.say("guild.forfeited", "You have left the guild.", {static_cast<i64>(guild)}, {});
}

// Max across joined guilds [c61][c99][c102] (adopted). The wiki rules that guild A/D fall as
// stat A/D rise [c87] and raise only if above current [c104] are UNKNOWN in the engine
// excerpt and not applied.
GuildAttDefResult GuildAttDef(const Character& c) {
    // Raw awards: the best att and the best def over joined guilds, each taken independently.
    GuildAttDefResult r{};
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = c.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        if (m.att > r.att) {
            r.att = m.att;
        }
        if (m.def > r.def) {
            r.def = m.def;
        }
    }
    return r;
}

// Value shown on the Char page: the 10/10 base (Rules.BaseAD) plus the best raw guild award.
GuildAttDefResult GuildAttDefDisplay(const Character& c) {
    constexpr i32 kBaseAD = 10;
    GuildAttDefResult r = GuildAttDef(c);
    r.att += kBaseAD;
    r.def += kBaseAD;
    return r;
}

// Hits gain per level-up or advance.
// At or above ML: MH [c25]. Below ML: AH, with a pre-ML random increment at 2/3 chance,
// Int(Rnd*(AH/5)+1) [c25]. Wiki ML boundary [c88][c104] overridden.
// CON bonus applies to any guild below ML [c25] (wiki Nomad only [c88] overridden).
// Draw order: chance draw, increment draw, then CON bonus.
// UNKNOWN: whether the increment is additive to AH (assumed) or replaces it.
// AH, MH and ML values are placeholders where not known; see guildAh, guildMh, guildMaxLevel.
i32 GuildHitsGain(Ctx& ctx, const GuildMembership& m, const Character& c, i32 guild) {
    const i32 ml = guildMaxLevel(guild);
    const i32 mh = guildMh(guild);
    if (m.level >= ml) {
        return mh;
    }
    const i32 ah = guildAh(guild);
    i32 gain = ah;
    if (ctx.rng.nextDouble() < kPreMlHitsChance) {
        const double span = static_cast<double>(ah / kPreMlHitsDivisor);
        gain += static_cast<i32>(vbInt(ctx.rng.nextDouble() * span + 1.0));
    }
    const i32 con = c.abilities.stats[static_cast<u32>(StatIndex::Con)];
    gain += ConHitsBonus(ctx, con);
    return gain;
}

// CON above 16 gives a bonus of 1..CON-16 [c25]. Uniform draw assumed.
// UNKNOWN: wiki gives no formula [c88][c104]; engine range used.
i32 ConHitsBonus(Ctx& ctx, i32 con) {
    if (con <= kConHitsThreshold) {
        return 0;
    }
    return static_cast<i32>(ctx.rng.below(con - kConHitsThreshold) + 1);
}

}  // namespace mordor
