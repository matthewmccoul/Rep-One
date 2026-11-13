// guilds1_a.cpp: guild join, eligibility, levelling, spell-access, swing and
// ability-chance rules from SRD c04p1 (R1-R37).
//
// ASSUMED FIELD NAMES. The foundation text does not give the field names of
// game_data.h, so these are assumed and must be reconciled with the generated file:
//   GuildDef:    abilities (Arr<i32,6>, R26/R30), spellMax (Arr<i32,18>, R26/R29),
//                reqStats (Arr<i32,6>, R15), alignMask (i32, R15), raceMask (i32, R15),
//                maxLevel (i32, R20.2), experiencePenalty (i32, EP, R21),
//                goldParameter (i32, GP, R23), itemUseFactor (f64, R27)
//   RaceDef:     expMod (f64, R21 RACE_MOD)
//   ItemDef:     levelScale (f64, R27), classRestricted (bool, R27), guilds (i32 bitmask, R27)
//   SpellDef:    level (i32, L in R29), spellMax (Arr<i32,18>, R29), rate (Arr<f64,18>, R29)
//   GuildLog kind values (GuildLogKind) are not given; kind 0 is used. UNKNOWN.
//
// FIXED: the compiler reports that GuildDef, RaceDef and SpellDef do not have the
// members experiencePenalty, expMod, goldParameter, rate and spellMax under these names.
// The generated game_data.h is not in the index, so the real names are unknown. The
// accessors below are placeholders that compile. They must be replaced with the real
// members once game_data.h is reconciled.
#include "mordor/guilds1.h"
#include "mordor/casting1.h"
#include "mordor/math_util.h"

#include <cmath>

namespace mordor {

namespace {
constexpr i32 kRaceRecordCount = 9;        // R2: nine races; indices outside 0..8 treated as having no RaceData record. UNKNOWN
constexpr i32 kSpellGuildSlots = 18;       // R26/R29: SpellMax[k], k = 0..17
constexpr i32 kAbilitySlots = 6;           // R26: ability 0..5
constexpr i32 kAbilityMultipleSwings = 3;  // R30: Abilities[3] is Multiple Swings
constexpr f64 kUseLevelExtra = 2.97;       // R27: (f + f * 2.97)

// FIXED: placeholder for GuildDef::experiencePenalty (EP, R21). Returns 0 until reconciled.
i32 guildExperiencePenalty(const GuildDef& guildDef) {
    // R21/R22: EP, the raw Experience Penalty from the guild data.
    // Reference: GuildDef.ExpFactor (GuildData.cs:58). The packet's game_data.h field is expFactor.
    // GUILD_MOD = EP / 8 is applied by the caller (expForLevel), not here.
    return guildDef.expFactor;
}

// FIXED: placeholder for RaceDef::expMod (RACE_MOD, R21). Returns neutral 1.0 until reconciled.
f64 raceExpMod(const RaceDef& raceDef) {
    // R21: RACE_MOD = Actual Exp from the race table.
    // Reference: RaceDef.ExpFactor (RaceData.cs:27), read as float (MdrLoader.cs:246).
    // UNKNOWN: game_data.h stores expFactor as i32, so any fractional part of the race value
    // is not recoverable here. The stored integer is returned as-is.
    return static_cast<f64>(raceDef.expFactor);
}

// FIXED: placeholder for GuildDef::goldParameter (GP, R23). Returns 0 until reconciled.
i32 guildGoldParameter(const GuildDef& guildDef) {
    // R23: GP, the raw Gold Parameter from the guild data.
    // Reference: GuildDef.GoldFactor (GuildData.cs:33), "field 15, the guild gold factor used by
    // the cost of making a level". The packet's game_data.h field is goldFactor (GGF, ch05).
    // GP (Actual) = GP / 10 + 1 is applied by the caller, not here.
    return guildDef.goldFactor;
}

// FIXED: placeholder for SpellDef::rate[k] (R29). Returns neutral 1.0 until reconciled.
// A value <= 0 is already treated as 1 by spellAccess.
f64 spellRate(const SpellDef&, u32) {
    return 1.0;
}

// FIXED: placeholder for SpellDef::spellMax[k] (R29). Returns 0 until reconciled,
// so no spell is castable until the real member is wired in.
i32 spellMaxFor(const SpellDef&, u32) {
    return 0;
}
}  // namespace

// FIXED: removed 'static'. unmet already has a non-static declaration in a header,
// so the forward declaration and the definition below must both be non-static.
Vec<Str> unmet(const GuildDef& guildDef, const Character& character);

// R4: crest item id = 299 + guild index (table order, Nomad = 0).
i32 crestId(Guild guild) {
    return kCrestItemIdBase + static_cast<i32>(guild);
}

// R13: join procedure at the guild hall.
bool joinGuild(Ctx& ctx, GameState& game, Character& character, Inventory& inventory, Guild guild) {
    const u32 g = static_cast<u32>(guild);

    // R13.1: already joined -> re-acquaint (free, only makes the guild active).
    if (character.guilds[g].joined) {
        return reacquaintGuild(ctx, character, guild);
    }

    const GuildDef& def = ctx.data.guilds[g];

    // R13.2: TopGuildLevel < 10 -> warning is shown first; the join then continues.
    if (character.topGuildLevel < kJoinWarnTopLevel) {
        ctx.say("guild.join.warn", "Warning: your top guild level is below 10.", {}, {});
    }

    // R13.3: cost = GuildJoinCost(def) if the character already belongs to another guild, else 0.
    // RESOLVED-BY-REFERENCE: GuildHall.cs:104 JoinQuestion: "Priced whenever you already belong to a
    // guild other than the current one, even at a price of nought; asked plainly otherwise".
    bool belongsElsewhere = false;
    for (i32 i = 0; i < kGuildCount; ++i) {
        if (static_cast<u32>(i) != g && character.guilds[static_cast<u32>(i)].joined) {
            belongsElsewhere = true;
        }
    }
    const Currency cost = belongsElsewhere ? guildJoinCost(def, character) : 0;

    // R13.3 / R10: the question is shown; priced (cost in nums) when belongsElsewhere, plain otherwise.
    // The guild's display name is not in this chunk, so the text omits it.
    ctx.say("guild.join.question",
            belongsElsewhere ? "Cost to join the guild is:" : "Do you wish to join the guild?",
            {cost}, {});

    // R13.4: refuse if worn cursed gear is present.
    // CONFLICT: whether this also refuses a re-acquaint is open; re-acquaint (R13.1) returns before this check.
    if (!gearCheck(inventory)) {
        ctx.say("guild.join.cursed", "You cannot join while wearing cursed gear.", {}, {});
        return false;
    }

    // R13.5: refuse if purse < cost.
    if (character.purse() < cost) {
        ctx.say("guild.join.gold", "Not enough gold!", {}, {});
        return false;
    }

    // R13.6 / R14: refuse if Unmet(character) is non-empty.
    if (!eligibleToJoin(def, character)) {
        ctx.say("guild.join.unmet", "You do not meet the requirements for this guild.", {}, {});
        return false;
    }

    // R13.7: pay the cost. UNKNOWN: on-hand gold is spent first, then the bank.
    Currency remaining = cost;
    const Currency fromHand = remaining < character.wallet.goldOnHand ? remaining : character.wallet.goldOnHand;
    character.wallet.goldOnHand -= fromHand;
    remaining -= fromHand;
    character.wallet.goldInBank -= remaining;

    // R13.7: insert {Guild, Level = 1, Experience = 0} as the front entry.
    // UNKNOWN: a fresh membership record is created, so previous guildGold and quest state are reset.
    GuildMembership fresh{};
    fresh.joined = true;
    fresh.level = 1;
    fresh.experience = 0;
    character.guilds[g] = fresh;

    // R13.7: the visited guild becomes active.
    setActiveGuild(ctx, character, guild);
    // R13.7: RecalcSpellPoints(refill: true).
    recalcSpellPoints(ctx, character, true);
    // R13.7: log the join. UNKNOWN: kind value.
    addGuildLog(ctx, game, guild, 0, "Joined the guild.");
    // UNKNOWN: "first class is reselected" has no engine counterpart in c04p1; skipped.
    ctx.say("guild.join.ok", "You have joined the guild.", {}, {});
    return true;
}

// R13.1: re-acquaint. Free; only makes the guild active.
// UNKNOWN: the meaning of "re-acquainting" beyond activation (R13.1, citing c104) is not given.
bool reacquaintGuild(Ctx& ctx, Character& character, Guild guild) {
    if (!character.guilds[static_cast<u32>(guild)].joined) {
        return false;
    }
    return setActiveGuild(ctx, character, guild);
}

// R26: guild join cost (engine).
//   sum = sum over abilities a>0 of Round((a+1)*1.25, 4)
//       + sum over spellMax m>0 of Round(m/3, 4)
//   cost = Round(sum^2 * 5356)
// UNKNOWN: unit of the result (whole gold vs 1/10000 gold). Returned as Currency directly.
// UNKNOWN: the character parameter is not used by the engine formula (R26).
Currency guildJoinCost(const GuildDef& guildDef, const Character& character) {
    (void)character;
    // R26: sum over abilities 0..5 (a > 0) and spell classes 0..17 (m > 0).
    constexpr i32 kJoinAbilityCount = 6;
    constexpr i32 kJoinSpellCount = 18;
    auto round4 = [](f64 x) -> f64 {
        return static_cast<f64>(roundHalfEven(x * 10000.0)) / 10000.0;
    };
    f64 sum = 0.0;
    for (i32 i = 0; i < kJoinAbilityCount; ++i) {
        // Ability ratings are fractional (e.g. Warrior 1.2, Thief 0.6). Keep them as f64; do not truncate.
        const f64 a = guildDef.abilities[static_cast<u32>(i)];
        if (a > 0.0) {
            sum += round4((a + 1.0) * kJoinAbilityWeight);
        }
    }
    for (i32 k = 0; k < kJoinSpellCount; ++k) {
        const i32 m = guildDef.spellMax[static_cast<u32>(k)];
        if (m > 0) {
            sum += round4(static_cast<f64>(m) / kJoinSpellDivisor);
        }
    }
    return roundHalfEven(sum * sum * static_cast<f64>(kJoinCostFactor));
}

// R13.4: returns true when no worn item is cursed (refusal when cursed gear is worn).
bool gearCheck(const Inventory& inventory) {
    for (const Item& it : inventory.worn) {
        if (it.cursed != CurseNone) {
            return false;
        }
    }
    return true;
}

// R14 / R13.6: engine join test; a join is refused if Unmet is non-empty.
bool eligibleToJoin(const GuildDef& guildDef, const Character& character) {
    return unmet(guildDef, character).empty();
}

// R14 / R15: the reasons a join is refused, in order race, alignment, stats.
Vec<Str> unmet(const GuildDef& guildDef, const Character& character) {
    Vec<Str> reasons;
    if (!allowsRace(guildDef, static_cast<i32>(character.race))) {
        reasons.push_back("Race");
    }
    if (!allowsAlignment(guildDef, character.alignment)) {
        reasons.push_back("Alignment");
    }
    if (!meetsStats(guildDef, character.abilities.stats)) {
        reasons.push_back("Stats");
    }
    return reasons;
}

// R15: MeetsStats: all i: stat[i] >= ReqStats[i].
bool meetsStats(const GuildDef& guildDef, const Arr<i32, kStatCount>& stats) {
    for (i32 i = 0; i < kStatCount; ++i) {
        const u32 u = static_cast<u32>(i);
        if (stats[u] < guildDef.reqStats[u]) {
            return false;
        }
    }
    return true;
}

// R15: AllowsAlignment: (AlignMask & (1 << a)) != 0. Bit0 Good, bit1 Neutral, bit2 Evil.
bool allowsAlignment(const GuildDef& guildDef, Align align) {
    const i32 a = static_cast<i32>(align);
    return (guildDef.alignMask & (1 << a)) != 0;
}

// R15: AllowsRace: i < 0 || (RaceMask & (1 << i)) != 0.
// Quirk (R15): a race with no RaceData record is allowed. Indices outside the nine
// races are treated as having no record and are allowed. UNKNOWN: exact RaceData presence test.
bool allowsRace(const GuildDef& guildDef, i32 raceIndex) {
    if (raceIndex < 0) {
        return true;
    }
    if (raceIndex >= kRaceRecordCount) {
        return true;
    }
    return (guildDef.raceMask & (1 << raceIndex)) != 0;
}

// R16: creation eligibility display: hidden / italic / bold / plain.
EligibilityStyle eligibilityStyle(const GuildDef& guildDef, const Character& character) {
    if (!allowsRace(guildDef, static_cast<i32>(character.race))) {
        return EligibilityStyle::Hidden;
    }
    if (!allowsAlignment(guildDef, character.alignment)) {
        return EligibilityStyle::Italic;
    }
    if (meetsStats(guildDef, character.abilities.stats)) {
        return EligibilityStyle::Bold;
    }
    return EligibilityStyle::Plain;
}

// R13.7: RecalcSpellPoints(refill). The spell-point formula is not in c04p1.
// UNKNOWN: delegated to the casting1.h overload recalcSpellPoints(Character&, bool).
void recalcSpellPoints(Ctx& ctx, Character& character, bool refill) {
    (void)ctx;
    mordor::recalcSpellPoints(character, refill);
}

// R8: the guild log records (day, guild, kind, text). The day is taken from game.today.
// UNKNOWN: trimming to the newest 50 per guild (R8, R44) is left to cleanGuildLog.
void addGuildLog(Ctx& ctx, GameState& game, Guild guild, i32 kind, Str text) {
    GuildLogEvent ev{};
    ev.day = game.today;
    ev.guild = guild;
    ev.kind = kind;
    ev.text = text;
    game.guildLog.push_back(ev);
    ctx.say("guild.log.added", text, {}, {});
}

// R17: SetActiveGuild (engine). i = index of the guild in the ordered list.
//   i <= 0 -> return (i == 0); otherwise move the entry to the front.
// A guild not joined has no index (i = -1) and returns false.
bool setActiveGuild(Ctx& ctx, Character& character, Guild guild) {
    (void)ctx;
    if (!character.guilds[static_cast<u32>(guild)].joined) {
        return false;
    }
    if (character.activeGuild == guild) {
        return true;
    }
    character.activeGuild = guild;
    return true;
}

// R18: wiki gate (town-only; refuse while wearing cursed or opposite-aligned items).
// CONFLICT: the engine excerpt has no such gate; per R18 the engine governs, so no gate is applied.
// inventory and inTown are therefore unused.
bool setActiveGuildGated(Ctx& ctx, Character& character, const Inventory& inventory, Guild guild, bool inTown) {
    (void)inventory;
    (void)inTown;
    return setActiveGuild(ctx, character, guild);
}

// R20: Make Level procedure at the guild hall.
void makeLevel(Ctx& ctx, GameState& game, Character& character, Inventory& inventory, Guild guild) {
    const u32 g = static_cast<u32>(guild);
    GuildMembership& m = character.guilds[g];
    if (!m.joined) {
        ctx.say("guild.makelevel.notjoined", "You have not joined this guild.", {}, {});
        return;
    }
    const GuildDef& def = ctx.data.guilds[g];

    // R20.2: Level >= MaxLevel (999): log "nothing left to teach" and stop.
    // RESOLVED-BY-REFERENCE: SRD R20.2 and Character.cs:1420 test ActiveGuildDef. This code still
    // reads the guild parameter. Switching needs the active-guild accessor, which is not in this
    // chunk, so the lookup is not changed here.
    if (m.level >= def.maxLevel) {
        addGuildLog(ctx, game, guild, 0, "Nothing left to teach.");
        ctx.say("guild.makelevel.max", "There is nothing left to teach you.", {}, {});
        return;
    }

    // R20.3: if !CanLevelUp(): not-enough-experience box, then Exp. Info.
    if (!canLevelUp(m, character.xpRequired)) {
        ctx.say("guild.makelevel.exp", "You do not have enough experience.", {}, {});
        ctx.say("guild.makelevel.expinfo", "Experience information.", {}, {});
        return;
    }

    // R20.4: QuestSatisfied(consume: true) runs before gold is asked.
    // Quirk (R20.4): a fetch item is handed over here even if step 5 refuses.
    if (!questSatisfied(ctx, character, inventory, guild, true)) {
        // CONFIRMED: Character.cs:1420 "Only a pinned character is offered the forfeit".
        // The choice itself (accept or decline) is a player action and is not modelled.
        if (m.quest.pinned) {
            ctx.say("guild.quest.forfeitOffer", "Forfeit the pinned quest?", {}, {});
            return;
        }
        ctx.say("guild.quest.incomplete", "Quest has not been completed!", {}, {});
        return;
    }

    // R20.5: fee = GoldForNextLevel(). If unaffordable, show the not-enough-gold box.
    const Currency fee = goldForNextLevel(m);
    if (character.purse() < fee) {
        ctx.say("guild.makelevel.gold", "Not enough gold!", {}, {});
        return;
    }

    // R20.6: pay (UNKNOWN: on-hand gold first, then bank), then LevelUp.
    Currency remaining = fee;
    const Currency fromHand = remaining < character.wallet.goldOnHand ? remaining : character.wallet.goldOnHand;
    character.wallet.goldOnHand -= fromHand;
    remaining -= fromHand;
    character.wallet.goldInBank -= remaining;

    levelUp(ctx, m);

    // R20.6: messages in order: title, spell, pack, mastership, skill-record, quest-roll.
    // UNKNOWN: spell, pack and skill-record messages have no engine text in c04p1; skipped.
    ctx.say("guild.makelevel.title", guildTitle(guild, m.level), {}, {});

    // R20.8: mastership claim occurs within step 6.
    if (claimMastership(ctx, game, character, guild, m.level)) {
        mastershipProcedure(ctx, game, character, inventory, guild);
    }

    // R20.8: the quest roll (P-Quest) occurs within step 6, after mastership.
    assignQuest(ctx, m, def, m.level);

    // R20.7 / R33: hits on the new level. UNKNOWN: current hits also rise by the gain.
    const i32 con = character.abilities.stats[static_cast<u32>(StatIndex::Con)];
    const i32 gain = hitsGain(ctx, character, m, def, con);
    i32 newMax = character.vitals.maxHits + gain;
    if (newMax > kMaxHits) {
        newMax = kMaxHits;
    }
    character.vitals.maxHits = newMax;
    character.vitals.curHits += gain;
    if (character.vitals.curHits > character.vitals.maxHits) {
        character.vitals.curHits = character.vitals.maxHits;
    }

    // R20.9: stat increase on level is UNKNOWN (no engine description in c04p1); not applied.
}

// R20.3: CanLevelUp: experience has reached the level threshold.
// UNKNOWN: xpRequired is taken as the threshold the experience must reach.
bool canLevelUp(const GuildMembership& membership, i64 xpRequired) {
    return membership.experience >= xpRequired;
}

// R25: gold to advance from the membership's current level, using its guildGold factor.
Currency goldForNextLevel(const GuildMembership& membership) {
    return goldForLevel(membership.guildGold, membership.level);
}

// R20.6: LevelUp. UNKNOWN: the engine effect beyond the level increment (experience handling,
// ability ratings) is not described in c04p1; only the level is incremented.
void levelUp(Ctx& ctx, GuildMembership& membership) {
    (void)ctx;
    membership.level += 1;
}

// R21 (wiki): xpRequired = (GUILD_MOD * a + b - 1) * RACE_MOD.
//   a = 88.09020776 * (LVL-1)^2
//   b = 500/11 * (2*(LVL-1) - 1)
//   GUILD_MOD = actualExpMod(EP), RACE_MOD = RaceDef.expMod
// UNKNOWN: rounding of the final value (round half to even chosen).
// Quirk: at level 1 the result is negative because b is negative; kept as written. UNKNOWN.
i64 expForLevel(const GuildDef& guildDef, const RaceDef& raceDef, i32 level) {
    const f64 l1 = static_cast<f64>(level - 1);
    const f64 a = 88.09020776 * l1 * l1;
    const f64 b = (500.0 / 11.0) * (2.0 * l1 - 1.0);
    const f64 guildMod = actualExpMod(guildExperiencePenalty(guildDef));
    const f64 raceMod = raceExpMod(raceDef);
    return roundHalfEven((guildMod * a + b - 1.0) * raceMod);
}

// R24: Actual EP = EP / 8.
f64 actualExpMod(i32 ep) {
    return static_cast<f64>(ep) / 8.0;
}

// R24: Actual GP = GP / 10 + 1.
f64 actualGoldMod(i32 gp) {
    return static_cast<f64>(gp) / 10.0 + 1.0;
}

// R23 (wiki): goldRequired = floor[(LVL-1)^2 * 30.1 + (LVL-1)/10] * GP.
// CONFLICT: the source says GP here means the raw Gold Parameter but also says to use Actual GP.
// The formula is used as written: the multiplier is the raw GP (actualGoldMod is not applied).
i64 goldForLevelWiki(const GuildDef& guildDef, i32 level) {
    const f64 l1 = static_cast<f64>(level - 1);
    const i64 base = vbInt(l1 * l1 * 30.1 + l1 / 10.0);
    return base * static_cast<i64>(guildGoldParameter(guildDef));
}

// R25 (engine): gold to advance from level L.
//   rate = 60.2
//   acc = 0
//   for l = 1..L:
//     term = Round_half_even((L/2)*2 + (l-1))
//     acc = Round_half_even(acc + (L/2)*term*rate / L)
//   result = Round(acc + acc*(guildGold/10))
// UNKNOWN: L/2 is read as floating division (VB '/'). Integer division would change the
// test values; the Nomad-from-level-3 value of 352 (R25 quirk) should be checked against T-tests.
// Race Exp is read but unused in the engine (R25).
Currency goldForLevel(Currency guildGold, i32 level) {
    if (level <= 0) {
        return 0;
    }
    const f64 L = static_cast<f64>(level);
    const f64 half = L / 2.0;
    f64 acc = 0.0;
    for (i32 l = 1; l <= level; ++l) {
        const i64 term = roundHalfEven(half * 2.0 + static_cast<f64>(l - 1));
        acc = static_cast<f64>(roundHalfEven(acc + half * static_cast<f64>(term) * kGoldRate / L));
    }
    const f64 gold = acc + acc * (static_cast<f64>(guildGold) / 10.0);
    return roundHalfEven(gold);
}

// R27 (engine): UseLevel(it, g) = max(1, round_half_even((f + f*2.97) * it.LevelScale)),
// where f = g.ItemUseFactor.
i32 itemUseLevel(const ItemDef& item, const GuildDef& guildDef) {
    const f64 f = guildDef.itemUseFactor;
    const i64 raw = roundHalfEven((f + f * kUseLevelExtra) * item.levelScale);
    return raw < 1 ? 1 : static_cast<i32>(raw);
}

// R27 (engine): MeetsUseLevel.
//   ClassRestricted: Guild.Level >= UseLevel(it, currentGuild)
//   else: any joined m with Level > 0, allowed by it.Guilds bitmask, and m.Level >= UseLevel(it, g)
// CONFLICT: the wiki says item use is restricted by the active guild, but skills learned in one guild
// remain usable elsewhere (R27, c83). The engine rule governs, so the non-class-restricted path is kept.
// UNKNOWN: item lookup assumes ctx.data.items is indexed by Item.id.
bool meetsUseLevel(Ctx& ctx, const Character& character, const Item& item) {
    if (item.id < 0 || item.id >= static_cast<i32>(ctx.data.items.size())) {
        return false;
    }
    const ItemDef& itemDef = ctx.data.items[static_cast<u32>(item.id)];

    if (itemDef.classRestricted) {
        const u32 ag = static_cast<u32>(character.activeGuild);
        return character.activeLevel() >= itemUseLevel(itemDef, ctx.data.guilds[ag]);
    }

    for (i32 i = 0; i < kGuildCount; ++i) {
        const u32 g = static_cast<u32>(i);
        const GuildMembership& m = character.guilds[g];
        if (!m.joined || m.level <= 0) {
            continue;
        }
        if ((itemDef.guilds & kGuildBitmask[g]) == 0) {
            continue;
        }
        if (m.level >= itemUseLevel(itemDef, ctx.data.guilds[g])) {
            return true;
        }
    }
    return false;
}

// R29 (engine): canCast = SpellMax[k] > 0 && L <= SpellMax[k] && L*rate <= guildLevel/2.0 + 0.5
//               shownLevel = floor(L*rate)
// A rate <= 0 counts as 1. The bound is compared unrounded; the shown level is floored.
// UNKNOWN: per-guild spell arrays (spellMax, rate) are assumed indexed by guildIndex k.
SpellAccessResult spellAccess(const GuildMembership& membership, const SpellDef& spell, i32 guildIndex) {
    SpellAccessResult result{};
    if (guildIndex < 0 || guildIndex >= kSpellGuildSlots) {
        return result;
    }
    const u32 k = static_cast<u32>(guildIndex);
    const f64 rawRate = spellRate(spell, k);
    const f64 rate = rawRate <= 0.0 ? 1.0 : rawRate;
    const i32 spellLevel = spell.level;
    const i32 spellMax = spellMaxFor(spell, k);
    const f64 shown = static_cast<f64>(spellLevel) * rate;

    result.canCast = spellMax > 0
                  && spellLevel <= spellMax
                  && shown <= static_cast<f64>(membership.level) / 2.0 + 0.5;
    result.shownLevel = static_cast<i32>(vbInt(shown));
    return result;
}

// R30 (engine): extra swings.
//   mod = Abilities[3]
//   if mod <= 0 or m.Level <= 0: return 0
//   step = ln(10 - mod + 1)
//   threshold = round_half_even(step * 50)
//   for level = 1..m.Level: if level > threshold: extra++;
//       threshold = round_half_even(step * (threshold * 2))   (uses the old threshold, R30)
// UNKNOWN: when 10 - mod + 1 <= 0 the log is undefined; 0 is returned.
i32 swingsFor(const GuildDef& guildDef, const GuildMembership& membership) {
    const i32 mod = guildDef.abilities[static_cast<u32>(kAbilityMultipleSwings)];
    if (mod <= 0 || membership.level <= 0) {
        return 0;
    }
    const f64 arg = static_cast<f64>(kSwingModBase - mod + 1);
    if (arg <= 0.0) {
        return 0;
    }
    const f64 step = std::log(arg);
    i64 threshold = roundHalfEven(step * static_cast<f64>(kSwingThresholdScale));
    i32 extra = 0;
    for (i32 level = 1; level <= membership.level; ++level) {
        if (static_cast<i64>(level) > threshold) {
            ++extra;
            threshold = roundHalfEven(step * static_cast<f64>(threshold * 2));
        }
    }
    return extra;
}

// R34.2 / R35: the best ability chance across the character's joined guilds wins.
// The per-guild formula, including the thieving clamp for DL > 0, is in abilityChance.
i32 chanceForAbility(const Character& character, i32 ability, i32 dungeonLevel, i32 monsterGuildLevel) {
    bool any = false;
    i32 best = 0;
    for (i32 i = 0; i < kGuildCount; ++i) {
        const GuildMembership& m = character.guilds[static_cast<u32>(i)];
        if (!m.joined) {
            continue;
        }
        const i32 c = abilityChance(m, ability, character, dungeonLevel, monsterGuildLevel);
        if (!any || c > best) {
            best = c;
            any = true;
        }
    }
    return best;
}

}  // namespace mordor
