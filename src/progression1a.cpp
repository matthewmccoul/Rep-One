// progression1a.cpp
// Implements include/mordor/progression1a.h: blow and chest experience, guild
// experience grants with the pin cap, level thresholds, level-up hit grants and
// attack/defence level awards. Rule numbers refer to SRD Chapter 5 part c05p1.
//
// UNKNOWN: GuildDef.expFactor, RaceDef.expFactor and GuildHitsRef fields
// (avgHits, lateHits, hitsCapLevel) are assumed names. Check them against
// game_data.h and attributes.h before building.

#include "mordor/progression1a.h"

#include <cmath>

#include "mordor/game_data.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

constexpr double kChestGuildScaleD = static_cast<double>(kChestGuildScale);

// R5/R12: guild EP coefficient from the guild table (T1).
double guildExpFactor(const Ctx& ctx, Guild g) {
    // UNKNOWN: field name of the EP factor in GuildDef.
    return static_cast<double>(ctx.data.guilds[static_cast<u32>(g)].expFactor);
}

// R5/R12: race raw Exp factor from the race table (T2).
double raceExpFactor(const Ctx& ctx, Race r) {
    // UNKNOWN: field name of the Exp factor in RaceDef.
    return static_cast<double>(ctx.data.races[static_cast<u32>(r)].expFactor);
}

i64 minI64(i64 a, i64 b) {
    return a < b ? a : b;
}

}  // namespace

// R1: inflicted = min(dmg, max(0, target.Hits)).
i32 inflictedHits(i32 rawDamage, i32 targetHits) {
    const i32 pool = targetHits > 0 ? targetHits : 0;
    return rawDamage < pool ? rawDamage : pool;
}

// R2: per-blow award.
// Engine: xp = h*(9-Ln(h+1))*(20-Ln(h))*(monGL+50)*cf/(1000*(1+curGL/333));
//         award = Round((1-(partySize-1)*0.04) * Round(xp)).
BlowAward blowExperience(i64 inflicted, i32 monsterGuildLevel, i32 attackerGuildLevel,
                         bool byCompanion, i32 partySize, i32 companionOwnerIndex) {
    BlowAward out;
    out.creditOwner = byCompanion;
    out.ownerIndex = byCompanion ? companionOwnerIndex : -1;

    // R2 step 3: award is zero when h = 0.
    if (inflicted <= 0) {
        return out;
    }

    const double h = static_cast<double>(inflicted);
    const double companionFactor = byCompanion ? 0.25 : 1.0;  // R2 step 2
    const double guildFactor =
        1.0 / (1.0 + static_cast<double>(attackerGuildLevel) / kChestGuildScaleD);
    const double raw = h * (9.0 - std::log(h + 1.0)) * (20.0 - std::log(h)) *
                       (static_cast<double>(monsterGuildLevel) + 50.0) * companionFactor *
                       guildFactor / 1000.0;
    // CONFLICT: rounding resolved to the engine (round, party factor, round) [c18].
    const i64 xpRounded = roundHalfEven(raw);
    const double partyFactor = 1.0 - (partySize - 1) * kPartyStepPerMember;
    out.xp = roundHalfEven(partyFactor * static_cast<double>(xpRounded));
    return out;
}

// R3: clamp at the cap, plus the payout path [c37].
// gained = 0 if already pinned, else min(award, cap - total).
// total += gained; a total above the pin is pulled back to the pin.
// Amount (returned) is never below zero.
i64 giveExperience(Ctx& ctx, Character& ch, Guild g, i64 award) {
    GuildMembership& m = ch.guilds[static_cast<u32>(g)];
    const i64 cap = experienceCap(ctx, ch, g);  // R3 step 1
    const i64 before = m.experience;
    const bool wasPinned = before >= cap;

    // RESOLVED-BY-REFERENCE: GiveExperience / sub0F11 (Combat.cs:516) does
    // "Experience += xp; if (Experience >= cap) Experience = cap;". Gained is therefore
    // min(before + award, cap) - before. A lowered pin is pulled back to cap with no loss
    // counted, and a negative award still lowers the total.
    const i64 gained = minI64(before + award, cap) - before;

    m.experience = before + gained;
    const bool isPinned = m.experience >= cap;

    if (!wasPinned && isPinned) {
        // RESOLVED-BY-REFERENCE: Combat.cs:516 "You are now pinned." (singular form).
        // The party form ("<name>" is now pinned.) is omitted: no character name is available here.
        ctx.say("progression.pinned", "You are now pinned.", {}, {});
    } else if (!isPinned) {
        // R3 step 5: "Enough to make a level" when crossing the next threshold.
        const i64 needed =
            levelThresholdXp(m.level, guildExpFactor(ctx, g), raceExpFactor(ctx, ch.race));
        if (before < needed && m.experience >= needed) {
            // RESOLVED-BY-REFERENCE: Combat.cs:516 "You now have enough experience to make a level."
            // (singular form; party form omitted for the same reason as above).
            ctx.say("progression.levelReady",
                    "You now have enough experience to make a level.", {}, {});
        }
    }

    // R3: Amount = max(0, gained).
    return gained > 0 ? gained : 0;
}

// R4: chest trap disarmed. Paid only when Fight.ChestTrap > 0.
// Engine (MONSMOD.BAS:3508-3509): xp = disarm*(GL/2)/(1+firstGL/333);
// award = Round(party * Round(xp)).
// UNKNOWN: scale of disarmChance (abilityChance). Used as given.
i64 chestTrapExperience(i32 fightChestTrap, double disarmChance, i32 monsterGuildLevel,
                        i32 firstMemberGuildLevel, i32 partySize) {
    if (fightChestTrap <= 0) {
        return 0;
    }
    const double xp = disarmChance * (static_cast<double>(monsterGuildLevel) / 2.0) /
                      (1.0 + static_cast<double>(firstMemberGuildLevel) / kChestGuildScaleD);
    const i64 xpRounded = roundHalfEven(xp);
    const double partyFactor = 1.0 - (partySize - 1) * kPartyStepPerMember;
    return roundHalfEven(partyFactor * static_cast<double>(xpRounded));
}

// R5: ExperienceCap = Fn13ED(level) = Fn12A1(level+1) - 1.
i64 experienceCap(const Ctx& ctx, const Character& ch, Guild g) {
    const GuildMembership& m = ch.guilds[static_cast<u32>(g)];
    return levelThresholdXp(m.level + 1, guildExpFactor(ctx, g), raceExpFactor(ctx, ch.race)) - 1;
}

// R5 toolbar [c17]:
//   needed = ExpForNextLevel(); if exp < needed: Short(needed - exp)
//   cap = ExperienceCap(); exp at cap: Pinned; else Ready(cap - exp)
ToolbarNeed progressToolbar(const Ctx& ctx, const Character& ch, Guild g) {
    const GuildMembership& m = ch.guilds[static_cast<u32>(g)];
    ToolbarNeed out;

    const i64 needed =
        levelThresholdXp(m.level, guildExpFactor(ctx, g), raceExpFactor(ctx, ch.race));
    if (m.experience < needed) {
        out.kind = ToolbarKind::Short;
        out.amount = needed - m.experience;
        return out;
    }

    const i64 cap = experienceCap(ctx, ch, g);
    // CONFLICT: engine uses exp == cap [c17]; documentation and port use
    // "at or above cap shows Pinned" so a drained pin is not shown negative [c36].
    if (m.experience >= cap) {
        out.kind = ToolbarKind::Pinned;
        out.amount = 0;
        return out;
    }
    out.kind = ToolbarKind::Ready;
    out.amount = cap - m.experience;
    return out;
}

// R8: hit grant on a level gain. Returns the hits to add to MaxHits; the
// caller applies capMaxHits and fills CurHits.
// Engine [c2]: ahead-of-every is strict (newLevel > highestOther).
// UNKNOWN: rng(3) is taken as below(3) (0..2); AvgHits draw uses nextDouble.
// FIXED: GuildHitsRef has no members hitsCapLevel or lateHits; the only member
// present is hitsPerLevel. Every former use of hitsCapLevel and lateHits now reads
// hitsPerLevel as a stand-in so the file compiles. This is NOT the intended
// behaviour: the ML/MH comparisons and the late-hit grant are wrong until the real
// fields are supplied. Verify against attributes.h and the LoadGuilds mapping.
i32 levelHitGain(Ctx& ctx, const Character& ch, Guild g, i32 newLevel,
                 i32 highestOtherLevel, const GuildHitsRef& rates) {
    static_cast<void>(g);
    const bool aheadOfEvery = newLevel > highestOtherLevel;

    // RESOLVED-BY-REFERENCE: LevelUp (MainMod/Character.cs) does
    // "if (Level > HitsCapLevel || !aheadOfEveryOtherGuild) { if (aheadOfEveryOtherGuild) MaxHits += LateHits; }".
    // Past ML only MH is given, and only when ahead of every guild. Otherwise nothing is given.
    // Field mapping per LoadGuilds: AH = hitsPerLevel, ML = hitsCapLevel, MH = lateHits.
    // FIXED: hitsCapLevel and lateHits stand-ins, see note above.
    if (newLevel > static_cast<i32>(rates.hitsPerLevel) || !aheadOfEvery) {
        return aheadOfEvery ? static_cast<i32>(rates.hitsPerLevel) : 0;
    }

    // Below ML: AH flat, plus two times in three a roll of 1 to AH/5, plus CON-16 when CON > 16.
    i32 gain = 0;
    if (ctx.rng.below(3) + 1 <= 2) {
        gain += static_cast<i32>(vbInt(ctx.rng.nextDouble() * rates.hitsPerLevel / 5.0)) + 1;
    }
    gain += static_cast<i32>(rates.hitsPerLevel);
    const i32 con = ch.abilities.stats[static_cast<u32>(StatIndex::Con)];
    // FIXED: hitsCapLevel stand-in, see note above.
    if (con > 16 && newLevel < static_cast<i32>(rates.hitsPerLevel)) {
        gain += static_cast<i32>(vbInt(ctx.rng.nextDouble() * (con - 16))) + 1;
    }
    return gain;
}

// R8/R9: ceiling MaxHits = min(MaxHits, 999).
i32 capMaxHits(i32 maxHits) {
    return maxHits < kHitCap ? maxHits : kHitCap;
}

// R8 step 4: refill spell points on each level.
// UNKNOWN: refill amount beyond "refill" is not in SRD; full refill used.
void refillSpellPoints(Character& ch) {
    ch.vitals.curSpellPoints = ch.vitals.maxSpellPoints;
}

// R8 step 4: recalculate swings on each level.
// UNKNOWN: swing formula is not in this SRD part. No-op until supplied.
void recalculateSwings(Character& ch) {
    static_cast<void>(ch);
}

// R10: award-level step [c28]:
//   step = ((Int(cap)/2) + (acc/18)) / (AdEvery + bonus), rounded to Integer.
// UNKNOWN: acc and bonus meanings; zero divisor guarded to 0.
i32 adAwardStep(i64 experienceCapValue, i32 acc, i32 adEvery, i32 bonus) {
    const double divisor = static_cast<double>(adEvery + bonus);
    if (divisor == 0.0) {
        return 0;
    }
    const double numerator = static_cast<double>(experienceCapValue) / 2.0 +
                             static_cast<double>(acc) / 18.0;
    return static_cast<i32>(roundHalfEven(numerator / divisor));
}

// R10: the level test is "level Mod step = 0" and only through AdMaxLevel.
// Guards: a non-positive step or level never awards.
bool isAdAwardLevel(i32 level, i32 step, i32 adMaxLevel) {
    if (step <= 0 || level <= 0) {
        return false;
    }
    if (level > adMaxLevel) {
        return false;
    }
    return (level % step) == 0;
}

// R10: attack and defence grant on a qualifying level.
void grantLevelAd(Ctx& ctx, Character& ch, Guild g, i32 newLevel, i32 awardStep,
                  const LevelAdRates& rates) {
    if (!isAdAwardLevel(newLevel, awardStep, rates.adMaxLevel)) {
        return;
    }
    GuildMembership& m = ch.guilds[static_cast<u32>(g)];
    m.att += rates.attPerLevel;
    m.def += rates.defPerLevel;
    // UNKNOWN: message text not in SRD.
    ctx.say("progression.adAward", "Your attack and defence improved.", {}, {});
}

// R12: fixed-point rounding to four decimal places (half-even).
i64 curFixed(double x) {
    return roundHalfEven(x * 10000.0);
}

// R12: race multiplier f = ((Ln(r)/Ln(r+1)) + 1) - 0.7 + r/25, with r = raceExp+1.
// Original quirk: the engine leaves this Single value unnarrowed [c18]; it is
// kept in double here.
double raceExpMultiplier(double raceFactorPlusOne) {
    const double r = raceFactorPlusOne;
    return ((std::log(r) / std::log(r + 1.0)) + 1.0) - 0.7 + r / 25.0;
}

// R12: engine loop for Fn12A1(level).
// UNKNOWN: Fn12A1(0) return value; 0 is returned.
i64 levelThresholdXp(i32 level, double guildExp, double raceExp) {
    if (level <= 0) {
        return 0;
    }
    const double l038A = guildExp;
    const double l038C = raceExp + 1.0;
    i64 l0392 = 0;
    for (i32 l = 1; l <= level; ++l) {
        const i64 l038E = curFixed((2.0 * l) * l038A);
        // Real division: l/2*2 + (l-1) = 2l-1 [c24].
        const i64 l0390 = curFixed((static_cast<double>(l) / 2.0) * 2.0 + (l - 1));
        l0392 = curFixed((static_cast<double>(l038E) / 10000.0 / 12.0 * 500.0 * l +
                          344.0 * static_cast<double>(l0390) / 10000.0 - 2.0) /
                         10.0);
    }
    const double f = raceExpMultiplier(l038C);
    l0392 = curFixed(static_cast<double>(l0392) / 10000.0 * f);
    return roundHalfEven(static_cast<double>(l0392) / 10000.0);
}

}  // namespace mordor
