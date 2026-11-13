// progression2a.cpp: experience thresholds, gold projection, archive route/HP tables,
// aging, sanctuary, pinned level-up announcement, XP factors, roster slot lookup.
#include "mordor/progression2a.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"

namespace mordor {

namespace {

// T2 [c71]: XP factor (EP) per guild, indexed by Guild enum order.
// CONFLICT: Wizard EP is 3.75 in T2 [c71] but its printed Parameter 27 implies 3.375 (T2 note).
// Chosen: 3.75 (EP from the factor column).
constexpr double kGuildExpFactor[kGuildCount] = {
    1.0, 2.0, 2.375, 2.625, 2.625, 2.25, 1.875, 2.25, 3.375, 3.375, 3.75, 4.0};

// R33/T6 [c104]: party penalty of 0.04 per extra member (1.00, 0.96, 0.92, 0.88).
constexpr double kPartyPenaltyPerMember = 0.04;

// Currency is i64 in 1/10000 gold.
constexpr i64 kCurrencyPerGold = 10000;

i32 guildIndexOf(Guild g) {
    return static_cast<i32>(g);
}

// T1 [c61][c71]: RaceMult is looked up by the race's raw experience value.
// kRaceMultByRawExp is indexed by raw exp (3 -> 1.0, 4 -> 1.05819..., ...).
double raceExpMult(Race race) {
    const i32 raw = kRaceRawExp[static_cast<i32>(race)];
    return kRaceMultByRawExp[raw];
}

// Fn12A1 [c2][c36]. UNKNOWN: the SRD text does not give the Fn12A1 formula.
// Assumed: XP_Total(n) = 88.09020776*n^2 + 45.45454*n + 0.264 [c104], scaled by the
// guild EP and the race RaceMult. Archive constants 266/177 [c103] are not used (archive tier, R32).
double fn12A1Raw(i32 level, double guildEp, double raceMult) {
    const double n = static_cast<double>(level);
    const double total = kXpTotalSquare * n * n + kXpTotalLinear * n + kXpTotalOffset;
    return total * guildEp * raceMult;
}

// fn12B2 [c27]: l03A8 + (((l039E / 2) * l03A6) * 60.2) / l039E
// UNKNOWN: l03A8 and l039E are not identified in the SRD. Assumed l03A8 = 0,
// l039E = guild level, l03A6 = GoldFactor. Under this reading the level cancels
// and the result is 30.1 * GGF per level. Gold is in whole gold here.
double fn12B2Gold(i32 level, i32 goldFactor) {
    if (level <= 0) {
        return 0.0;
    }
    const double l039E = static_cast<double>(level);
    const double l03A6 = static_cast<double>(goldFactor);
    return ((l039E / 2.0) * l03A6 * 60.2) / l039E;
}

}  // namespace

// Fn12A1 [c2][c36]: ExpForThisLevel() is 0 at level 1, else fn12A1(level - 1).
// Returned value is the XP total needed to stand at `level`.
i64 levelXpAtTarget(i32 level, Guild g, Race race) {
    if (level <= 1) {
        return 0;
    }
    const double raw = fn12A1Raw(level - 1, kGuildExpFactor[guildIndexOf(g)], raceExpMult(race));
    // UNKNOWN: rounding of the scaled threshold; assumed round_half_even (currency rounding R33).
    return roundHalfEven(raw);
}

// R15/R16/R28 [c2]: gold loop. Sums GoldForNextLevel from the current guild level
// up to targetLevel - 1. Each step is rounded half up in whole gold [R33].
// Result is in Currency (1/10000 gold).
Currency projectGoldToLevel(const Character& ch, Guild g, i32 targetLevel) {
    const i32 idx = guildIndexOf(g);
    const i32 start = ch.guilds[static_cast<u32>(idx)].level;
    const i32 ggf = kGuildGoldFactor[idx];
    i64 wholeGold = 0;
    for (i32 lvl = start; lvl < targetLevel; ++lvl) {
        wholeGold += goldRoundHalfUp(fn12B2Gold(lvl, ggf));
    }
    return wholeGold * kCurrencyPerGold;
}

// R29 / T12 [c106]: archive route targets. Warrior 28, Nomad 30, Wizard 41.
// UNKNOWN: no archive route target is given for the other guilds; returns 0.
i32 routeTargetLevel(Guild g) {
    switch (g) {
        case Guild::Warrior:
            return kRouteWarriorTarget;
        case Guild::Nomad:
            return kRouteNomadTarget;
        case Guild::Wizard:
            return kRouteWizardTarget;
        default:
            return 0;
    }
}

// R29 / T11 [c106]: archive HP gained on reaching `level` in guild g.
// Warrior: 6 HP per level up to 28.
// Nomad: levels 29-30 give 5 HP more. UNKNOWN: per-level or total; chosen per-level.
//        Past level 30: 1 HP per level.
// Wizard: ML 41; past ML: 2 HP per level (other guilds past ML rule).
// UNKNOWN: Nomad base HP per level and HitsCapLevel of other guilds are not given; returns 0.
// UNKNOWN: Warrior HP past level 28 is not given; returns 0.
i32 archiveHitsForLevel(Guild g, i32 level) {
    switch (g) {
        case Guild::Warrior:
            return level <= kRouteWarriorTarget ? kWarriorAvgHits : 0;
        case Guild::Nomad:
            if (level > kRouteNomadMaxLevel) {
                return kNomadPostHits;
            }
            if (level >= 29) {
                return kNomadExtraHits;
            }
            return 0;
        case Guild::Wizard:
            return level > kRouteWizardTarget ? kOtherPostHits : 0;
        default:
            return 0;
    }
}

// R29 / T11 [c106] and SRD c2: bonus HP applies when CON > 16 (i.e. >= 17)
// and the guild level is below the hits cap level (ML).
bool archiveBonusHpApplies(i32 con, i32 level, i32 hitsCapLevel) {
    return con >= kRouteConBonusMin && level < hitsCapLevel;
}

// R29 [c106]: race restrictions (archive tier, R32, not engine behaviour).
// Elves and Osiri cannot be Warriors. Giants and Morlochs cannot be Wizards.
// Gnomes cannot be either.
bool raceMayJoinGuild(Race race, Guild g) {
    switch (g) {
        case Guild::Warrior:
            return race != Race::Elf && race != Race::Osiri && race != Race::Gnome;
        case Guild::Wizard:
            return race != Race::Giant && race != Race::Morloch && race != Race::Gnome;
        default:
            return true;
    }
}

// R22 / T3 [c87]: general aging. Adds `days` to the character's age.
// UNKNOWN: negative aging (Potion of Youth, Dragon's Blood) is clamped so that age is never
// below 0; the SRD gives no floor.
void ageBy(Ctx& ctx, Character& ch, i32 days) {
    i32 next = ch.ageDays + days;
    if (next < 0) {
        next = 0;
    }
    ch.ageDays = next;
    ctx.say("progression.age", "The character has aged.", {days, ch.ageDays}, {});
}

// R33 [c34]: pinned level-up announcement. The port tests MaxLevel; the original does not.
// UNKNOWN: the announcement condition is not stated in the SRD. Assumed: the guild is pinned,
// i.e. Experience >= ExperienceCap, where ExperienceCap = Fn13ED(level) = Fn12A1(level+1) - 1
// [c3][c18]. Fn12A1(level+1) equals levelXpAtTarget(level + 2).
bool shouldAnnouncePinnedLevelUp(const Character& ch, Guild g, i32 maxLevel, bool portTestsMaxLevel) {
    const GuildMembership& m = ch.guilds[static_cast<u32>(guildIndexOf(g))];
    const i64 cap = levelXpAtTarget(m.level + 2, g, ch.race) - 1;
    const bool pinned = m.experience >= cap;
    if (!pinned) {
        return false;
    }
    if (portTestsMaxLevel) {
        return m.level < maxLevel;
    }
    return true;
}

// R33 [c33][c34]: community gold loop rounds halves up. Implemented as floor(value + 0.5).
i64 goldRoundHalfUp(double value) {
    return vbInt(value + 0.5);
}

// R34 [c18]: companion 0.25 is applied within XP via the companion flag (engine resolution).
// Returns the experience factor for a kill struck by a companion or by a character.
double xpStrikerFactor(bool byCompanion) {
    return byCompanion ? kCompanionXpFactor : 1.0;
}

// R33/T6 [c104]: party XP factor, 1 - 0.04 * (extra members).
// UNKNOWN: out-of-range party sizes are clamped to 1..kPartyMaxSize; the SRD does not say.
double partyXpFactor(i32 partySize) {
    i32 n = partySize;
    if (n < 1) {
        n = 1;
    }
    if (n > kPartyMaxSize) {
        n = kPartyMaxSize;
    }
    return 1.0 - kPartyPenaltyPerMember * static_cast<double>(n - 1);
}

// R34 [c31]: roster order is creation order; a deleted character's slot is reused.
// Returns the first unoccupied index, or -1 if every slot is occupied.
// UNKNOWN: the no-free-slot result is not given in the SRD; -1 is chosen so the caller appends.
i32 firstFreeRosterSlot(const Vec<bool>& occupied) {
    const i32 n = static_cast<i32>(occupied.size());
    for (i32 i = 0; i < n; ++i) {
        if (!occupied[static_cast<u32>(i)]) {
            return i;
        }
    }
    return -1;
}

}  // namespace mordor
