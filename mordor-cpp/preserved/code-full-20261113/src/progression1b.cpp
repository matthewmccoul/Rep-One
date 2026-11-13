// progression1b.cpp
// Implements include/mordor/progression1b.h.
// Rule numbers (R13-R27) refer to SRD part c05p1.
#include "mordor/progression1b.h"

#include <cmath>

#include "mordor/math_util.h"

namespace mordor {
namespace {

// R13 XP closed-form constants (test/spreadsheet/archive forms).
constexpr double kXpCoeff = 88.09020776;
constexpr double kXpLinear = 500.0 / 11.0;  // 45.45454...
// CONFLICT: research and wiki use -1; test, spreadsheet and archive use -0.264.
// Resolution per SRD: engine-aligned closed form, so -0.264 is used everywhere here.
constexpr double kXpOffset = -0.264;

// R16 gold constant (combat-audit, archive loop, closed forms).
constexpr double kGoldStep = 30.1;

// R18 guild join constants.
constexpr double kJoinAbilityFactor = 1.25;
constexpr double kJoinCostScale = 5356.0;

// R19/R20 aging constants (days).
constexpr i32 kDaysPerYear = 365;

// R21 ailment tick constants.
constexpr i32 kPoisonClearAbove = 185;
constexpr i32 kDiseaseClearAbove = 195;
constexpr i32 kPoisonHitCost = 2;
constexpr i32 kDiseaseHitCost = 1;
constexpr i64 kAilmentTickMs = 4000;
constexpr double kOldAgePercentOnset = 98.0;

// R22/R24 rest constant.
constexpr double kRestPercentDivisor = 3.25;

// R26 CON loss constants.
constexpr i32 kConLossBase = 50;

// R27 play clock: SRD counts seconds, inputs here are milliseconds.
constexpr i64 kMsPerSecond = 1000;

// Math.Round-style rounding: halves go up (archive loop text: "x.5 up").
i64 roundHalfUp(double x) {
    return vbInt(x + 0.5);
}

// R23 helper: RestInCity() for one character. The engine routine is not in the
// chunk, so this uses the R24 rest age cost and the R19/R20 aging path.
// Engine states nobody is healed by the rest described here, so no healing.
// UNKNOWN: effects recompute (Effects.Recompute) is not in the chunk and is not modelled.
i32 restInCity(Ctx& ctx, Character& ch, bool partyMany) {
    const i32 days = restAgeCostDays(ch.vitals.curHits, ch.vitals.maxHits);
    ageBy(ctx, ch, days, partyMany);
    return days;
}

}  // namespace

// R13 spreadsheet, no rounding:
//   if L <= 1: XP = 0
//   else XP = (88.09020776*(L-1)^2*D9 + 500/11*((L-1)*2-1) - 0.264) * D7
// lvl = L, guildEp = D9, raceFactor = D7.
double closedFormLevelXpUnrounded(i32 lvl, double guildEp, double raceFactor) {
    if (lvl <= 1) {
        return 0.0;
    }
    const double l = static_cast<double>(lvl - 1);
    return (kXpCoeff * l * l * guildEp + kXpLinear * (l * 2.0 - 1.0) + kXpOffset) * raceFactor;
}

// R13 research form: XP = R*[(E/8)(88.09020776*x^2) + (500/11)(2x-1) - 1], x = lvl-1.
// CONFLICT: the research source uses -1; the engine-aligned -0.264 is used (see kXpOffset).
// Zero for lvl <= 1 follows the R13 note "closed form = 0 for lvl <= 1".
double closedFormLevelXpResearch(i32 lvl, double guildGef, double raceFactor) {
    if (lvl <= 1) {
        return 0.0;
    }
    const double x = static_cast<double>(lvl - 1);
    const double guildTerm = (guildGef / kGefDivisor) * kXpCoeff * x * x;
    return raceFactor * (guildTerm + kXpLinear * (2.0 * x - 1.0) + kXpOffset);
}

// R13 wiki form: XP_required = (GUILD_MOD*a + b - 1) * RACE_MOD.
//   a = 88.09020776*(LVL-1)^2 ; b = 500/11*(2*(LVL-1)-1)
//   GUILD_MOD = EP/8 ; RACE_MOD = Actual Exp
// CONFLICT: wiki uses -1; engine-aligned -0.264 is used (see kXpOffset).
// UNKNOWN: lvl <= 1 guard; zero is returned, matching the R13 note.
double closedFormLevelXpWiki(i32 lvl, double guildGef, double raceActualExp) {
    if (lvl <= 1) {
        return 0.0;
    }
    const double l = static_cast<double>(lvl - 1);
    const double a = kXpCoeff * l * l;
    const double b = kXpLinear * (2.0 * l - 1.0);
    const double guildMod = guildGef / kGefDivisor;
    return (guildMod * a + b + kXpOffset) * raceActualExp;
}

// Wiki [c88]: XP(L) = round((i + j - 2) * (k + l + 0.3) / 10), a = raceFactor + 1.
//   i = L^2*guildFactor*250/3 ; j = (2L-1)*344 ; k = ln(a)/ln(a+1) ; l = a/25
// UNKNOWN: round is taken as the SRD round_half_even (roundHalfEven).
i64 levelXpWikiC88(i32 lvl, double guildFactor, double raceFactor) {
    const double bigL = static_cast<double>(lvl);
    const double a = raceFactor + 1.0;
    const double i = bigL * bigL * guildFactor * 250.0 / 3.0;
    const double j = (2.0 * bigL - 1.0) * 344.0;
    const double k = std::log(a) / std::log(a + 1.0);
    const double l = a / 25.0;
    return roundHalfEven((i + j - 2.0) * (k + l + 0.3) / 10.0);
}

// Wiki [c88]: XPforPin = XP(min(currentLevel + 1, 998)).
i64 xpForPin(i32 currentLevel, double guildFactor, double raceFactor) {
    const i32 target = (currentLevel + 1 < 998) ? (currentLevel + 1) : 998;
    return levelXpWikiC88(target, guildFactor, raceFactor);
}

// Archive level XP [c103]: XP(lvl) = r * g * (n/2) * (266 + (n-1)*177).
// The c103 form is unreconciled (see R13 conflicts). Implemented literally.
// n/2 is taken as a real division.
double levelXpArchive(i32 n, double raceFactor, double guildFactor) {
    const double nd = static_cast<double>(n);
    return raceFactor * guildFactor * (nd / 2.0) * (266.0 + (nd - 1.0) * 177.0);
}

// R13 / archive [c104]: XP still needed = XP_Total - current XP.
i64 xpStillNeeded(i64 xpTotal, i64 currentXp) {
    return xpTotal - currentXp;
}

// R14 T1 race multiplier by raw Exp (test table [c61]).
// Raw Exp <= 3 gives 1.0. Values above 9 are not in the table.
// UNKNOWN: rawExp > 9 falls back to the approximation in raceActualExpApprox.
double raceMultForRawExp(i32 rawExp) {
    if (rawExp <= 3) {
        return 1.0;
    }
    switch (rawExp) {
        case 4: return 1.05819132268;
        case 5: return 1.10551994263;
        case 6: return 1.14714602440;
        case 7: return 1.18544741085;
        case 8: return 1.22165868435;
        case 9: return 1.25647909518;
        default: return raceActualExpApprox(rawExp);
    }
}

// R14 approximation [c96]:
//   Actual_Exp ~= 1 + [ln(Exp)/ln(3) - 1] / [4.50 - (Exp-4)*(11/90)]
// Guard: non-positive raw Exp returns 1.0 (ln undefined).
double raceActualExpApprox(i32 rawExp) {
    if (rawExp <= 0) {
        return 1.0;
    }
    const double e = static_cast<double>(rawExp);
    const double numerator = std::log(e) / std::log(3.0) - 1.0;
    const double denominator = 4.50 - (e - 4.0) * (11.0 / 90.0);
    return 1.0 + numerator / denominator;
}

// R14 / archive [c106]: Elves and Osiri cannot be Warriors; Giants and Morlochs
// cannot be Wizards; Gnomes cannot be either.
bool raceAllowsGuild(Race race, Guild guild) {
    if (guild == Guild::Warrior) {
        return !(race == Race::Elf || race == Race::Osiri || race == Race::Gnome);
    }
    if (guild == Guild::Wizard) {
        return !(race == Race::Giant || race == Race::Morloch || race == Race::Gnome);
    }
    return true;
}

// R14 wiki [c87]: stat_max_allowed = race_natural_max(stat) + 5.
i32 statItemMaxAllowed(i32 raceNaturalMax) {
    return raceNaturalMax + 5;
}

// R14 wiki [c88]: percent = Parameter / 8 * 100.
double guildParameterPercent(i32 parameter) {
    return static_cast<double>(parameter) / kGefDivisor * 100.0;
}

// R15 [c96][c104]: PROJECTED_XP = (DESIRED_LEVEL / CURRENT_LEVEL)^2 * CURRENT_XP.
// The race factor for non-Human races is applied by the caller (R15 note).
// Guard: currentLevel <= 0 returns 0 (division undefined).
double projectedXp(i32 desiredLevel, i32 currentLevel, double currentXp) {
    if (currentLevel <= 0) {
        return 0.0;
    }
    const double ratio = static_cast<double>(desiredLevel) / static_cast<double>(currentLevel);
    return ratio * ratio * currentXp;
}

// R15 [c96][c104]: PROJECTED_LEVEL = (DESIRED_XP / CURRENT_XP)^(1/2) * CURRENT_LEVEL.
// Guard: currentXp <= 0 returns 0 (division undefined).
double projectedLevel(double desiredXp, double currentXp, i32 currentLevel) {
    if (currentXp <= 0.0) {
        return 0.0;
    }
    return std::sqrt(desiredXp / currentXp) * static_cast<double>(currentLevel);
}

// R16 combat-audit [c24][c31]:
//   total = sum_{i=1..L} (2i-1)*30.1, running total rounded each step
//   cost  = round(total * (1 + guildGold/10))
// UNKNOWN: the rounding of the running total is taken as round_half_even at each step.
i64 goldCostAuditRunning(i32 level, i32 ggf) {
    i64 total = 0;
    for (i32 i = 1; i <= level; ++i) {
        const double step = static_cast<double>(2 * i - 1) * kGoldStep;
        total = roundHalfEven(static_cast<double>(total) + step);
    }
    const double multiplier = 1.0 + static_cast<double>(ggf) / 10.0;
    return roundHalfEven(static_cast<double>(total) * multiplier);
}

// R16 archive loop [c104] and test loop [c61]:
//   LevelCost = LevelCost + Math.round(((glvl*2)-1)*30.1)   for glvl = 1..glvl
//   Gold to reach level glvl+1 = Math.round(LevelCost * (1 + GGF/10))
// Round halves up (Math.round). The loop value is cumulative (R16 conflict).
i64 goldToReachArchiveLoop(i32 glvl, i32 ggf) {
    i64 levelCost = 0;
    for (i32 g = 1; g <= glvl; ++g) {
        levelCost += roundHalfUp(static_cast<double>(g * 2 - 1) * kGoldStep);
    }
    const double multiplier = 1.0 + static_cast<double>(ggf) / 10.0;
    return roundHalfUp(static_cast<double>(levelCost) * multiplier);
}

// R16 archive closed form [c104]: Gold per level n to n+1 = 30.1 * n^2 * (1 + ggf/10).
// CONFLICT: the label says per level, but the value equals the cumulative loop value at
// glvl = n. Resolution per SRD: cumulative (engine L^2 form). Literal formula kept.
i64 goldPerLevelArchiveClosed(i32 n, i32 ggf) {
    const double nd = static_cast<double>(n);
    const double multiplier = 1.0 + static_cast<double>(ggf) / 10.0;
    return roundHalfUp(kGoldStep * nd * nd * multiplier);
}

// R16 archive closed form [c104]:
//   Total gold from level 1 to n+1 = n(n+1)(2n+1)/6 * 30.1 * (1 + ggf/10)
// The page says totals may be off by at most 999 gp; the value is rounded half up.
i64 goldTotalArchiveClosed(i32 n, i32 ggf) {
    const double nd = static_cast<double>(n);
    const double sumSquares = nd * (nd + 1.0) * (2.0 * nd + 1.0) / 6.0;
    const double multiplier = 1.0 + static_cast<double>(ggf) / 10.0;
    return roundHalfUp(sumSquares * kGoldStep * multiplier);
}

// R16 research [c82]: Gold = P * floor(30.1*x^2 + x/10), P = 1 + GGF/10.
// UNKNOWN: operator between P and floor is assumed multiplication (per SRD).
// UNKNOWN: final rounding of the product is taken as round half up.
i64 goldNextLevelResearch(i32 x, i32 ggf) {
    const double xd = static_cast<double>(x);
    const double p = 1.0 + static_cast<double>(ggf) / 10.0;
    const i64 floored = vbInt(kGoldStep * xd * xd + xd / 10.0);
    return roundHalfUp(p * static_cast<double>(floored));
}

// R16 wiki [c88]:
//   i = currentLevel^2 * 30.1 ; j = currentLevel / 10.0 ; k = goldFactor / 10.0
//   GoldForNextLevel = floor( floor(i + j) * (k + 1) )
// Engine index is the current level (R16 conflict resolution).
i64 goldNextLevelWikiL2(i32 level, i32 goldFactor) {
    const double lv = static_cast<double>(level);
    const double i = lv * lv * kGoldStep;
    const double j = lv / 10.0;
    const double k = static_cast<double>(goldFactor) / 10.0;
    const i64 inner = vbInt(i + j);
    return vbInt(static_cast<double>(inner) * (k + 1.0));
}

// R16 wiki [c96]: Gold_required = floor[(LVL-1)^2*30.1 + (LVL-1)/10] * GP ; GP = goldFactor/10 + 1.
// UNKNOWN: the final rounding of the product is taken as round half up.
i64 goldNextLevelWikiMinus1(i32 level, i32 goldFactor) {
    const double l = static_cast<double>(level - 1);
    const i64 floored = vbInt(l * l * kGoldStep + l / 10.0);
    const double gp = static_cast<double>(goldFactor) / 10.0 + 1.0;
    return roundHalfUp(static_cast<double>(floored) * gp);
}

// R16 wiki [c88]: TotalGold(L) = sum over n = 1..L of GoldForNextLevel(n).
i64 goldTotalWiki(i32 level, i32 goldFactor) {
    i64 total = 0;
    for (i32 n = 1; n <= level; ++n) {
        total += goldNextLevelWikiL2(n, goldFactor);
    }
    return total;
}

// R17 [c49]: to = Math.Max(1, Guild.Level - cost).
i32 forfeitTargetLevel(i32 fromLevel, i32 cost) {
    const i32 target = fromLevel - cost;
    return (target > 1) ? target : 1;
}

// R17 archive [c106]: losing levels removes only AH hits.
// UNKNOWN: the AH hits per level are not in the chunk, so no hit change is applied here.
// The loss is reported as an event only.
void forfeitLoseLevelHits(Ctx& ctx, Character& ch, Guild guild, i32 levelsLost) {
    (void)guild;
    if (levelsLost <= 0) {
        return;
    }
    ctx.say("progression1b.forfeit_lose_hits",
            "Levels lost. Hit points from the lost levels are removed (amount not modelled).",
            {static_cast<i64>(levelsLost)},
            {ch.name});
}

// R17 archive [c106]: regaining levels below the maximum level grants extra bonus hits.
// UNKNOWN: the bonus hit amount is not in the chunk, so no hit change is applied here.
void forfeitRegainBonusHits(Ctx& ctx, Character& ch, Guild guild, i32 levelsGained) {
    (void)guild;
    if (levelsGained <= 0) {
        return;
    }
    ctx.say("progression1b.forfeit_regain_hits",
            "Levels regained. Bonus hit points below the maximum level (amount not modelled).",
            {static_cast<i64>(levelsGained), static_cast<i64>(ch.topGuildLevel)},
            {ch.name});
}

// R18 [c104]: the first guild after Nomad is free. True if no guild other than Nomad is joined.
bool firstGuildJoinIsFree(const Character& ch) {
    for (i32 k = 1; k < kGuildCount; ++k) {
        if (ch.guilds[static_cast<u32>(k)].joined) {
            return false;
        }
    }
    return true;
}

// R18 archive [c104]:
//   Cost = CInt((CCur((Each non-zero guild ability + 1) * 1.25)
//                + CCur(Each spell class maximum level / 3))^2 * 5356)
// UNKNOWN: "Each non-zero guild ability" is read as the sum over non-zero ratings of (rating + 1).
// UNKNOWN: spellClassMaxLevel is the single spell class maximum passed in.
// CCur rounds to 4 decimals (ccurRound4); CInt is round_half_even.
i64 guildJoinCost(const Arr<f32, 6>& abilityRatings, i32 spellClassMaxLevel) {
    double abilitySum = 0.0;
    for (u32 k = 0; k < 6; ++k) {
        const double rating = static_cast<double>(abilityRatings[k]);
        if (rating != 0.0) {
            abilitySum += rating + 1.0;
        }
    }
    const double term1 = ccurRound4(abilitySum * kJoinAbilityFactor);
    const double term2 = ccurRound4(static_cast<double>(spellClassMaxLevel) / 3.0);
    const double sum = term1 + term2;
    return roundHalfEven(sum * sum * kJoinCostScale);
}

// R18 CCur: rounds after the fourth decimal (1/3 becomes 0.3333) [c104].
double ccurRound4(double value) {
    return static_cast<double>(roundHalfEven(value * 10000.0)) / 10000.0;
}

// R20 [c4]: AgeBy(days, party). Adds days to the character's age and prints the aging message.
// Zero or negative days print nothing (R23: nothing said if no time passed).
// UNKNOWN: daysAged is not updated; only ageDays is.
// partyMany is passed to the message (party.Count > 1).
void ageBy(Ctx& ctx, Character& ch, i32 days, bool partyMany) {
    if (days <= 0) {
        return;
    }
    ch.ageDays += days;
    ctx.say("progression1b.aged",
            "The character ages.",
            {static_cast<i64>(days), static_cast<i64>(partyMany ? 1 : 0)},
            {ch.name});
}

// R21 [c23]: the ailment tick runs every 4 seconds of real time.
bool ailmentTickDue(i64 nowMs, i64 lastTickMs) {
    return nowMs - lastTickMs >= kAilmentTickMs;
}

// R21 [c23]: one ailment tick for a character.
//   Poison roll: draw 0-199 ; result = draw + 2*(effCON - 10) - 2*poisonLevel
//     clears poison if result > 185 ; otherwise costs 2 HP.
//   Disease roll: same formula using the remaining poisonLevel.
//     clears disease if result > 195 ; otherwise costs 1 HP.
//   Old age: if agePercent >= 98, roll 0-99 against oldAgeRollThreshold.
// UNKNOWN: the poison and disease rolls are gated on having that ailment (level > 0).
// UNKNOWN: the direction of the old-age comparison; the outcome is reported as an event only.
// Returns the hit points lost this tick (applied to curHits).
i32 ailmentTick(Ctx& ctx, Character& ch, i32 effectiveCon, double agePercent) {
    i32 hitsLost = 0;

    if (ch.ailments.poisonLevel > 0) {
        const i32 draw = ctx.rng.below(200);
        const i32 result = draw + 2 * (effectiveCon - 10) - 2 * ch.ailments.poisonLevel;
        if (result > kPoisonClearAbove) {
            ch.ailments.poisonLevel = 0;
            ctx.say("progression1b.poison_cleared", "The poison wears off.", {}, {ch.name});
        } else {
            hitsLost += kPoisonHitCost;
            ctx.say("progression1b.poison_hurts", "Poison costs hit points.",
                    {static_cast<i64>(kPoisonHitCost)}, {ch.name});
        }
    }

    if (ch.ailments.diseaseLevel > 0) {
        const i32 draw = ctx.rng.below(200);
        const i32 result = draw + 2 * (effectiveCon - 10) - 2 * ch.ailments.poisonLevel;
        if (result > kDiseaseClearAbove) {
            ch.ailments.diseaseLevel = 0;
            ctx.say("progression1b.disease_cleared", "The disease clears.", {}, {ch.name});
        } else {
            hitsLost += kDiseaseHitCost;
            ctx.say("progression1b.disease_hurts", "Disease costs hit points.",
                    {static_cast<i64>(kDiseaseHitCost)}, {ch.name});
        }
    }

    if (agePercent >= kOldAgePercentOnset) {
        const i32 roll = ctx.rng.below(100);
        const double threshold = oldAgeRollThreshold(agePercent);
        if (static_cast<double>(roll) < threshold) {
            // UNKNOWN: direction of comparison and death effect; event only.
            ctx.say("progression1b.old_age_roll", "An old-age roll succeeds.",
                    {static_cast<i64>(roll)}, {ch.name});
        }
    }

    ch.vitals.curHits -= hitsLost;
    return hitsLost;
}

// R21 [c23][c25]: old-age threshold = 5 + ln(max(agePercent - 98, 1e-9)).
double oldAgeRollThreshold(double agePercent) {
    const double excess = agePercent - kOldAgePercentOnset;
    const double guarded = (excess > 1e-9) ? excess : 1e-9;
    return 5.0 + std::log(guarded);
}

// R23 [c48]: arrival rest.
//   1. Place the party at the leader's dungeon level and square.
//   2. Set each member's Area = -1.
//   3. For each member in order: d = RestInCity(); Effects.Recompute(m).
//   4. If m is Members[0], record days = d.
//   5. If d > 0, report it.
//   6. Nothing is said if no time passed.
// Original quirk: dead members are rested as well as living ones [c34].
// Engine choice: nobody is healed by the rest [c48] (CONFLICT with [c24], not applied).
// UNKNOWN: Effects.Recompute is not in the chunk and is not modelled.
i32 restOnArrival(Ctx& ctx, Party& party) {
    if (party.size <= 0) {
        return 0;
    }
    const Character& leader = party.members[0];
    party.dungeonLevel = leader.location.dungeonLevel;
    party.x = leader.location.x;
    party.y = leader.location.y;

    for (i32 k = 0; k < party.size; ++k) {
        party.members[static_cast<u32>(k)].location.area = -1;
    }

    const bool partyMany = party.size > 1;
    i32 leaderDays = 0;
    for (i32 k = 0; k < party.size; ++k) {
        Character& member = party.members[static_cast<u32>(k)];
        const i32 d = restInCity(ctx, member, partyMany);
        if (k == 0) {
            leaderDays = d;
        }
        if (d > 0) {
            ctx.say("progression1b.arrival_rest", "The party rests in town.",
                    {static_cast<i64>(d)}, {member.name});
        }
    }
    return leaderDays;
}



// R26 [c103]: CON-loss roll on a successful raise.
bool rollRaiseConLoss(Ctx& ctx, i32 con) {
    return ctx.rng.percent(conLossChancePercent(con));
}

// R27 [c59] (sub16F8): PlaySeconds accumulates only when not paused.
// Returns the whole seconds to add for this step, computed from the ms inputs.
// The caller must refresh lastTickMs on every tick, paused or not.
// UNKNOWN: sub-second remainder is dropped (integer division); the SRD keeps the Timer value.
i64 playClockStep(i64 nowMs, i64 lastTickMs, bool paused) {
    if (paused) {
        return 0;
    }
    return (nowMs - lastTickMs) / kMsPerSecond;
}

}  // namespace mordor
