// guilds2_a.cpp
//
// Implements: SwingsFor, HitsGain, AdGain, MaxAttackDefence, CharacterAdCap,
//             JoinCost, CanJoin, Join, Forfeit, MakeLevel, AdvanceGold,
//             QuestTier, ItemQuestBand.
//
// CAVEAT: the supplied SRD half (c04p2) contains tables and quirk notes only.
// The rule text for AdGain, JoinCost, AdvanceGold, CanJoin, Join, Forfeit and
// MakeLevel is in the other SRD half and was not supplied. Those bodies are
// reconstructions and are marked UNKNOWN below. Replace them with the SRD
// pseudo-code before relying on them.
// FIXED: AdPair member names changed from attack/defence to att/def. The AdPair
// body is truncated in the API INDEX, so verify these names against attributes.h.

#include "mordor/guilds2.h"
#include "mordor/attributes.h"  // FIXED: AdPair is declared in attributes.h
#include "mordor/math_util.h"

namespace mordor {

namespace {

// Alignment of a character as a guild-mask bit (kGuildAlignMask, T20).
u8 alignBitOf(Align a) {
    switch (a) {
        case Align::Good:
            return kAlignBitGood;
        case Align::Neutral:
            return kAlignBitNeutral;
        case Align::Evil:
            return kAlignBitEvil;
    }
    return 0;
}

}  // namespace

// T4 / T14: extra-swing thresholds. At a listed level the count is i+1, one
// level earlier it is i [c61]. Count = number of listed thresholds reached.
// Only the five guilds with Multiple Swings have rows. Others give 0.
i32 SwingsFor(Guild guild, i32 level) {
    for (i32 r = 0; r < 5; ++r) {
        if (kSwingLevelRows[r].guild != guild) {
            continue;
        }
        i32 count = 0;
        for (i32 i = 0; i < kSwingLevelRows[r].levelCount; ++i) {
            if (level >= kSwingLevelRows[r].levels[i]) {
                ++count;
            }
        }
        return count;
    }
    return 0;
}

// Hits gained when a guild level is reached (R3, R4, T14, T16).
// UNKNOWN: the level being reached is taken as m.level + 1 (HitsGain is called
// before m.level is incremented). Roll order (CON bonus, then R4) is UNKNOWN.
i32 HitsGain(Ctx& ctx, const Character& ch, const GuildMembership& m, Guild guild) {
    const u32 g = static_cast<u32>(guild);
    const i32 newLevel = m.level + 1;

    // No hits for a level another guild has already reached [c25].
    if (newLevel <= ch.topGuildLevel) {
        return 0;
    }

    // Past ML (above the hit-cap level): exactly MH, no roll and no CON bonus [R32].
    if (newLevel > kHitsCapLevel[g]) {
        return kLateHits[g];
    }

    const i32 ah = kAvgHits[g];
    i32 hits = ah;

    // R4 / R32: at 2/3 chance add Int(Rnd*(AH/5)+1).
    // Gate: Int(Rnd*3)+1 <= 2, the reference's rng.Next(3)+1 <= 2 (one draw, true when Rnd < 2/3).
    // Bonus: Int(Rnd*AH/5)+1, which equals Int(Rnd*(AH/5)+1).
    if (ctx.rng.below(3) + 1 <= 2) {
        hits += static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<double>(ah) / 5.0)) + 1;
    }

    // R3 / R32: CON bonus, range 1..CON-16 when CON > 16, only below the hit-cap level
    // (reference: Guild.Level < HitsCapLevel, strict).
    const i32 con = ch.abilities.stats[static_cast<u32>(StatIndex::Con)];
    if (con > kConBonusThreshold && newLevel < kHitsCapLevel[g]) {
        hits += ctx.rng.below(con - kConBonusThreshold) + 1;
    }
    return hits;
}

// A/D gained on reaching a level (T6, T7, T14).
// UNKNOWN: the per-level A/D rule is not in the supplied SRD half. Placeholder:
// linear interpolation from 0 to the guild maximum at kMaxAdLevel, rounded half-even
// and clamped to the guild maximum. The Nomad sheet samples (T7) and the Warrior
// example (T14, 3.3 step) are NOT reproduced by this placeholder. ctx and ch are
// unused in this placeholder.
AdPair AdGain(Ctx& ctx, Guild guild, i32 level, const Character& ch) {
    (void)ctx;
    (void)ch;
    const u32 g = static_cast<u32>(guild);

    i32 lv = level;
    if (lv < 0) {
        lv = 0;
    }
    if (lv > kMaxAdLevel[g]) {
        lv = kMaxAdLevel[g];
    }

    const double frac = static_cast<double>(lv) / static_cast<double>(kMaxAdLevel[g]);
    i32 att = static_cast<i32>(roundHalfEven(static_cast<double>(kMaxAttack[g]) * frac));
    i32 def = static_cast<i32>(roundHalfEven(static_cast<double>(kMaxDefence[g]) * frac));
    if (att > kMaxAttack[g]) {
        att = kMaxAttack[g];
    }
    if (def > kMaxDefence[g]) {
        def = kMaxDefence[g];
    }

    AdPair out{};
    out.att = att;  // FIXED: AdPair member name (was attack); verify against attributes.h
    out.def = def;  // FIXED: AdPair member name (was defence); verify against attributes.h
    return out;
}

// T6: maximum Attack and Defence for a guild. Maxed independently [c61].
// Sorcerer and Wizard values follow the sheet [c70].
AdPair MaxAttackDefence(Guild guild) {
    const u32 g = static_cast<u32>(guild);
    AdPair out{};
    out.att = kMaxAttack[g];  // FIXED: AdPair member name (was attack); verify against attributes.h
    out.def = kMaxDefence[g]; // FIXED: AdPair member name (was defence); verify against attributes.h
    return out;
}

// T6 example: Thief 394 with Nomad 197 gives 175/204 [c61]. The cap is the
// per-field maximum over the guilds the character has joined.
// UNKNOWN: whether non-joined or unlevelled guilds should count is not stated.
AdPair CharacterAdCap(const Character& ch) {
    i32 att = 0;
    i32 def = 0;
    for (u32 g = 0; g < static_cast<u32>(kGuildCount); ++g) {
        if (!ch.guilds[g].joined) {
            continue;
        }
        if (kMaxAttack[g] > att) {
            att = kMaxAttack[g];
        }
        if (kMaxDefence[g] > def) {
            def = kMaxDefence[g];
        }
    }
    AdPair out{};
    out.att = att;  // FIXED: AdPair member name (was attack); verify against attributes.h
    out.def = def;  // FIXED: AdPair member name (was defence); verify against attributes.h
    return out;
}

// UNKNOWN: the join-cost formula is not in the supplied SRD half. The T5 test
// values (Warrior 5,009,672; Ninja 10,886,091; Thief 7,465,060) cannot be
// reproduced: the signature carries no guild argument and no cost inputs are
// given here. Placeholder returns 0 (free join). Replace with the SRD formula.
Currency JoinCost(Ctx& ctx, const Character& ch) {
    (void)ctx;
    (void)ch;
    return 0;
}

// Join eligibility. Race chart (T8) via kRaceGuildMask, alignment (T20) via
// kGuildAlignMask, minimum stats (T19) via kGuildMinStat in Str/Int/Wis/Con/Cha/Dex order.
// UNKNOWN: the stat comparison is taken as >= (not >). Cost check uses goldOnHand.
// Already-joined is refused. The order of the checks is UNKNOWN.
bool CanJoin(Ctx& ctx, const Character& ch, Guild guild) {
    const u32 g = static_cast<u32>(guild);

    if (ch.guilds[g].joined) {
        ctx.say("guild.join.refused", "Already a member of this guild.", {}, {});
        return false;
    }

    const u32 raceIndex = static_cast<u32>(ch.race);
    if (((kRaceGuildMask[raceIndex] >> g) & 1u) == 0u) {
        return false;
    }

    if ((kGuildAlignMask[g] & alignBitOf(ch.alignment)) == 0) {
        return false;
    }

    for (u32 s = 0; s < static_cast<u32>(kStatCount); ++s) {
        if (ch.abilities.stats[s] < kGuildMinStat[g][s]) {
            return false;
        }
    }

    if (ch.wallet.goldOnHand < JoinCost(ctx, ch)) {
        return false;
    }
    return true;
}

// UNKNOWN: the supplied half gives only the misspelt refusal text ("Cnanot") [c3]
// and not its trigger. This placeholder refuses when the guild is not joined or is
// Nomad (base guild). On success the membership is reset to defaults, including
// guildGold, and activeGuild falls back to Nomad if it was this guild.
bool Forfeit(Ctx& ctx, Character& ch, Guild guild) {
    const u32 g = static_cast<u32>(guild);
    if (!ch.guilds[g].joined || guild == Guild::Nomad) {
        ctx.say("guild.forfeit.refused", "Cnanot forfeit this guild.", {}, {});
        return false;
    }

    ch.guilds[g] = GuildMembership{};
    if (ch.activeGuild == guild) {
        ch.activeGuild = Guild::Nomad;
    }

    ctx.say("guild.forfeit", "You have forfeited the guild.", {}, {});
    return true;
}

// Make Level. Gate and effect order are UNKNOWN. This follows the documented
// behaviour: refused when gold is short [c30][c49]. Hits are computed before the
// level is incremented, so HitsGain sees the level being reached.
// Item quests clear at Make Level [c17]. The experience gate uses ch.xpRequired.
// game is unused. The guild-log entry is not written.
bool MakeLevel(Ctx& ctx, GameState& game, Character& ch, Guild guild) {
    (void)game;
    const u32 g = static_cast<u32>(guild);
    GuildMembership& m = ch.guilds[g];

    if (!m.joined) {
        ctx.say("guild.makelevel.refused", "You are not a member of this guild.", {}, {});
        return false;
    }

    const i32 nextLevel = m.level + 1;

    // UNKNOWN: experience gate. Uses ch.xpRequired as the threshold placeholder.
    // Reference (Character.CanLevelUp) gates on Guild.Experience >= Fn12A1(level) and Level < MaxLevel;
    // Fn12A1's body is not in the excerpt, so the threshold source stays as-is.
    if (m.experience < ch.xpRequired) {
        ctx.say("guild.makelevel.refused", "You lack the experience to advance.", {}, {});
        return false;
    }

    // UNKNOWN: which gold source pays the advance cost. goldOnHand is used here.
    const Currency cost = AdvanceGold(m.guildGold, nextLevel);
    if (ch.wallet.goldOnHand < cost) {
        ctx.say("guild.makelevel.refused", "You lack the gold to advance.", {}, {});
        return false;
    }

    // Hits first (uses the level being reached), then the level is incremented.
    const i32 hits = HitsGain(ctx, ch, m, guild);
    ch.wallet.goldOnHand -= cost;
    m.level = nextLevel;
    ch.vitals.maxHits += hits;

    // R32 / Character.LevelUp: maximum hits stop at 999, and making a level fills you up.
    const i32 kMaxHitsCap = 999;
    if (ch.vitals.maxHits > kMaxHitsCap) {
        ch.vitals.maxHits = kMaxHitsCap;
    }
    ch.vitals.curHits = ch.vitals.maxHits;

    if (m.level > ch.topGuildLevel) {
        ch.topGuildLevel = m.level;
    }

    const AdPair ad = AdGain(ctx, guild, m.level, ch);
    m.att = ad.att;  // FIXED: AdPair member name (was attack); verify against attributes.h
    m.def = ad.def;  // FIXED: AdPair member name (was defence); verify against attributes.h

    // Item quests clear at Make Level; they never reach Done [c17].
    if (m.quest.kind == QuestKind::Fetch) {
        m.quest.state = QuestState::Cleared;
    }

    ctx.say("guild.level", "Your guild level has advanced.", {}, {});
    return true;
}

// Gold cost to advance a guild level (R20, Fn12B2 rate 60.2; input guildGold/10).
// UNKNOWN: the R20 formula body is not in the supplied half. The placeholder below
// uses only the header constants and does NOT reproduce the Nomad level-3 value of
// 352 [c18]. Replace with the SRD formula. Currency is in 1/10000 gold.
Currency AdvanceGold(Currency guildGold, i32 level) {
    const double base =
        kAdvanceGoldRate * static_cast<double>(level) +
        static_cast<double>(guildGold / kAdvanceGoldDivisor);
    return roundHalfEven(base);
}



}  // namespace mordor
