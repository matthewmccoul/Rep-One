#include "mordor/progression2b.h"

#include <algorithm>
#include <cmath>

#include "mordor/math_util.h"

namespace mordor {
namespace {

// kCurrencyScale (i64, 1/10000 gold) comes from types.h (T7); the local duplicate
// was removed because it made unqualified references ambiguous.

// T10: hits cap 999, MaxHits = min(MaxHits, 999) [c2].
constexpr i32 kHitsCap = 999;

// T7 / gold closed form: base 30.1, guild gold factor term (1 + ggf/10) [c104].
constexpr double kGoldBase = 30.1;
constexpr double kGoldFactorDivisor = 10.0;

// T7 / T10 / test anchors: Human Nomad XP threshold polynomial in n = target level.
// Coefficients fitted to anchors 378,045 (n=66), 556,907 (n=80), 872,326 (n=100);
// constants 88.09020776 and 45.45454 are the T10 XP_Total constants [c104].
constexpr double kXpQuadratic = 88.09020776;
constexpr double kXpLinear = -85.31;
constexpr double kXpConstant = -45.45454;

// T9: rest age cost B((100 - 100*cur/max)/3.25) [c24][c25].
constexpr double kRestAgeDivisor = 3.25;

// T9: poison clear result > 185, cost 2 HP [c23].
constexpr i32 kPoisonClearAbove = 185;
constexpr i32 kPoisonCureHitCost = 2;

// T9: disease clear result > 195, cost 1 HP [c23].
constexpr i32 kDiseaseClearAbove = 195;
constexpr i32 kDiseaseCureHitCost = 1;

// T9: old-age onset at 98% of racial lifespan [c23][c25][c97].
constexpr i64 kOldAgePercent = 98;

// Natural death chance: 5% + Log(excess) [c25].
constexpr double kNaturalDeathBase = 5.0;

// Sanctuary ages 365 days in the original [c31][c34].
constexpr i32 kSanctuaryAgeDays = 365;

// T11: bonus HP while Con >= 17 for each level below ML [c106].
constexpr i32 kBonusHitsConMin = 17;
// UNKNOWN: amount of the bonus HP per level is not stated in the SRD; 0 is used until confirmed.
constexpr i32 kBonusHitsPerLevel = 0;

// T11: Nomad levels 29-30 give 5 HP more [c106].
constexpr i32 kNomadExtraLevelLo = 29;
constexpr i32 kNomadExtraLevelHi = 30;
constexpr i32 kNomadExtraHits = 5;

// T11: past ML, Nomad gives 1 HP per level, other guilds 2 HP per level [c106].
constexpr i32 kNomadPastMaxHits = 1;
constexpr i32 kOtherPastMaxHits = 2;

}  // namespace

// T7/T10: experience threshold for target level n, multiplied by guild and race
// experience factors. Engine: ExperienceCap = Fn12A1(level+1) - 1 [c3][c18].
i64 experienceCap(i32 level, double guildExpFactor, double raceExpFactor) {
    // UNKNOWN: the exact Fn12A1 closed form is not transcribed in the SRD excerpt.
    // The quadratic above reproduces the Human Nomad anchors with unit factors;
    // verify the factor product against MODULE6.BAS / MAINMOD2.BAS before relying on it.
    // RESOLVED-BY-REFERENCE: Fn12A1(0) is nought. Character.cs:1268 ExpForThisLevel:
    // "nought at level 1, where the loop does not run" (Fn12A1(Guild.Level - 1)).
    if (level == 0) {
        return 0;
    }
    const double n = static_cast<double>(level + 1);
    const double base = kXpQuadratic * n * n + kXpLinear * n + kXpConstant;
    const double scaled = guildExpFactor * raceExpFactor * base;
    i64 threshold = roundHalfEven(scaled);
    // UNKNOWN: negative results of Fn12A1 are not covered by the reference; clamp to zero.
    if (threshold < 0) {
        threshold = 0;
    }
    const i64 cap = threshold - 1;
    return cap < 0 ? 0 : cap;
}

// T7/T13 gold: running total over levels of (2i-1)*30.1*(1 + ggf/10), rounded per
// level to whole gold, then converted to currency. Engine index is the current
// level [c24][c27]. Per-level rounding is what produces the small gap noted in T7.
Currency levelUpGoldCost(i32 currentLevel, double guildGoldFactor) {
    const double mult = 1.0 + guildGoldFactor / kGoldFactorDivisor;
    i64 totalGold = 0;
    for (i32 i = 1; i <= currentLevel; ++i) {
        const double step = (2.0 * static_cast<double>(i) - 1.0) * kGoldBase * mult;
        totalGold += roundHalfEven(step);
    }
    return totalGold * kCurrencyScale;
}

// T13: guild join cost, whole gold table [c104].
// UNKNOWN: archive values depend on ability and spell inputs ("each non-zero guild
// ability", "spell class maximum level") that the SRD does not define; table used as given.
Currency guildJoinCost(Guild guild) {
    i64 gold = 0;
    switch (guild) {
        case Guild::Nomad:     gold = 1695394; break;
        case Guild::Warrior:   gold = 5009672; break;
        case Guild::Paladin:   gold = 3214939; break;
        case Guild::Ninja:     gold = 10886091; break;
        case Guild::Villain:   gold = 6191500; break;
        case Guild::Seeker:    gold = 8216267; break;
        case Guild::Thief:     gold = 7465060; break;
        case Guild::Scavenger: gold = 4569336; break;
        case Guild::Mage:      gold = 9132004; break;
        case Guild::Sorcerer:  gold = 8587434; break;
        case Guild::Wizard:    gold = 9317113; break;
        case Guild::Healer:    gold = 5730025; break;
    }
    return gold * kCurrencyScale;
}

// T10: MaxHits = min(MaxHits, 999) [c2].
i32 clampMaxHits(i32 maxHits) {
    return std::min(maxHits, kHitsCap);
}

// T7/T8 threshold transition: announce "can make a level" only when XP crosses
// the requirement during this change [c34]. The port's MaxLevel test is not used
// (the original does not test it) [c34].
void announceLevelThresholdCrossed(Ctx& ctx, const Character& ch, Guild guild, i64 xpBefore, i64 xpAfter, i64 xpRequired) {
    if (xpBefore < xpRequired && xpAfter >= xpRequired) {
        ctx.say("progression.can_make_level",
                "Enough experience to make a level.",
                {static_cast<i64>(guild), xpAfter, xpRequired},
                {ch.name});
    }
}

// Highest-guild rule: strictly greater than every other guild [c2][c21].
bool isAheadOfEveryGuild(const Character& ch, Guild guild) {
    // UNKNOWN: non-joined guilds are counted at their stored level (normally 0).
    const u32 mine = static_cast<u32>(guild);
    const i32 myLevel = ch.guilds[mine].level;
    for (u32 g = 0; g < static_cast<u32>(kGuildCount); ++g) {
        if (g == mine) {
            continue;
        }
        if (ch.guilds[g].level >= myLevel) {
            return false;
        }
    }
    return true;
}

// T11 hit gain for reaching newLevel [c106]. The engine returns zero when the
// guild is not the highest (handled by the caller) [c2].
i32 hitsGainForLevel(Guild guild, i32 newLevel, i32 maxLevel, i32 avgHits, i32 con) {
    const bool isNomad = guild == Guild::Nomad;
    if (newLevel > maxLevel) {
        return isNomad ? kNomadPastMaxHits : kOtherPastMaxHits;
    }
    i32 gain = avgHits;
    // UNKNOWN: whether "5 HP more" is per level or a total for levels 29-30 [c106].
    if (isNomad && newLevel >= kNomadExtraLevelLo && newLevel <= kNomadExtraLevelHi) {
        gain += kNomadExtraHits;
    }
    // RESOLVED-BY-REFERENCE: Character.cs:1268 LevelUp adds the constitution bonus only when
    // "Guild.Level < def.HitsCapLevel" (strict), so it is not given on the level equal to maxLevel.
    if (con >= kBonusHitsConMin && newLevel < maxLevel) {
        gain += kBonusHitsPerLevel;
    }
    return gain;
}

// T9: rest age cost = B((100 - 100*currentHP/maxHP) / 3.25) [c24][c25].
// Full HP gives 0; zero HP gives 31 [c24].
// UNKNOWN: the B() rounding rule; roundHalfEven is used here (wiki's 16 at 50% is a conflict, T8 note 11).
i32 restAgeCostDays(i32 curHits, i32 maxHits) {
    if (maxHits <= 0 || curHits >= maxHits) {
        return 0;
    }
    const i32 cur = std::max<i32>(0, curHits);
    const double pct = 100.0 * static_cast<double>(cur) / static_cast<double>(maxHits);
    const double days = (100.0 - pct) / kRestAgeDivisor;
    return static_cast<i32>(roundHalfEven(days));
}

// Rest aging. The original also rests dead members; this function does not
// distinguish them (port deviation documented in the SRD) [c34].
// UNKNOWN: whether ch.daysAged is also accumulated; only ageDays is advanced here.
void applyRestAging(Ctx& ctx, Character& ch) {
    const i32 days = restAgeCostDays(ch.vitals.curHits, ch.vitals.maxHits);
    if (days <= 0) {
        return;
    }
    ch.ageDays += days;
    ctx.say("progression.rest_aged",
            "Time passes while resting.",
            {static_cast<i64>(days)},
            {ch.name});
}

// T9: poison clear succeeds when result > 185 [c23].
bool poisonClearSucceeds(i32 result) {
    return result > kPoisonClearAbove;
}

// T9: poison cure. UNKNOWN: whether the 2 HP cost is charged on every attempt or
// only on success; charged on every attempt here.
void applyPoisonCure(Ctx& ctx, Character& ch, i32 result) {
    ch.vitals.curHits = std::max<i32>(0, ch.vitals.curHits - kPoisonCureHitCost);
    if (poisonClearSucceeds(result)) {
        ch.ailments.poisonLevel = 0;
        ctx.say("progression.poison_cured",
                "The poison is purged.",
                {static_cast<i64>(result)},
                {ch.name});
    } else {
        ctx.say("progression.poison_remains",
                "The poison remains.",
                {static_cast<i64>(result)},
                {ch.name});
    }
}

// T9: disease clear succeeds when result > 195 [c23].
bool diseaseClearSucceeds(i32 result) {
    return result > kDiseaseClearAbove;
}

// T9: disease cure. UNKNOWN: same cost-timing question as poison; charged on every attempt.
void applyDiseaseCure(Ctx& ctx, Character& ch, i32 result) {
    ch.vitals.curHits = std::max<i32>(0, ch.vitals.curHits - kDiseaseCureHitCost);
    if (diseaseClearSucceeds(result)) {
        ch.ailments.diseaseLevel = 0;
        ctx.say("progression.disease_cured",
                "The disease is cleared.",
                {static_cast<i64>(result)},
                {ch.name});
    } else {
        ctx.say("progression.disease_remains",
                "The disease remains.",
                {static_cast<i64>(result)},
                {ch.name});
    }
}

// T9: old-age onset at 98% of racial lifespan [c23][c25][c97].
bool isOldAgeOnset(i32 ageDays, i32 racialLifespanDays) {
    return static_cast<i64>(ageDays) * 100 >= static_cast<i64>(racialLifespanDays) * kOldAgePercent;
}

// Natural death chance in percent: 5 + ln(excess), where excess is days past onset [c25].
// UNKNOWN: the argument of the logarithm (days or fraction) and the roll comparison
// direction [c23][c25][c34]. Days are used here, with excess floored at 1 day so ln >= 0.
double naturalDeathChancePercent(i32 ageDays, i32 racialLifespanDays) {
    if (!isOldAgeOnset(ageDays, racialLifespanDays)) {
        return 0.0;
    }
    const double onsetDays = static_cast<double>(racialLifespanDays) * static_cast<double>(kOldAgePercent) / 100.0;
    double excess = static_cast<double>(ageDays) - onsetDays;
    if (excess < 1.0) {
        excess = 1.0;
    }
    const double chance = kNaturalDeathBase + std::log(excess);
    return std::min(100.0, std::max(0.0, chance));
}

// Rolls once with ctx.rng; dies when the roll is below the chance [c23][c34].
// UNKNOWN: comparison direction (roll below chance is used).
bool rollNaturalDeath(Ctx& ctx, i32 ageDays, i32 racialLifespanDays) {
    if (!isOldAgeOnset(ageDays, racialLifespanDays)) {
        return false;
    }
    const double chance = naturalDeathChancePercent(ageDays, racialLifespanDays);
    const double roll = static_cast<double>(ctx.rng.below(100));
    const bool dies = roll < chance;
    if (dies) {
        ctx.say("progression.natural_death",
                "Old age claims the character.",
                {static_cast<i64>(ageDays)},
                {});
    }
    return dies;
}

// Sanctuary ages the target by 365 days in the original [c31][c34].
// UNKNOWN: whether the target is the caster or the party; the port uses 28 days
// (deviation, not reproduced here) [c34].
void applySanctuary(Ctx& ctx, Character& target) {
    target.ageDays += kSanctuaryAgeDays;
    ctx.say("progression.sanctuary_aged",
            "The sanctuary spell ages the character.",
            {static_cast<i64>(kSanctuaryAgeDays)},
            {target.name});
}

}  // namespace mordor
