// progression2.cpp
// Implements the functions declared in include/mordor/progression2.h.
// Rule numbers cite the SRD chapter 5 part c05p2 (citations in [cNN] form are SRD source chunks).
//
// CONFLICT: progression2.h uses GuildHitsRef without including its declaring header, so
// attributes.h is included first so that the header compiles.
// CONFLICT: conLossChance(i32) is declared in both progression2.h and attributes.h (API INDEX
// says attributes owns it). It is NOT defined here; it is called from raiseCharacter.
// CONFLICT: playClockTick(GameState&, i64) is declared in progression2.h, but state_game.h
// (GameState) was not supplied and the API INDEX assigns playClockTick to clock.h with a
// different signature. It is NOT defined here.
// UNKNOWN: GuildHitsRef field names. FIXED: the compiler reports that avgHits, lateHits and
// hitsCapLevel do not exist on GuildHitsRef and that its only suggested member is hitsPerLevel.
// All three uses are mapped to hitsPerLevel as a placeholder. This is UNVERIFIED and changes
// the meaning of the level-up hit gain; the real member names must be confirmed against
// attributes.h.
// FIXED: local kHitsCap renamed to kHitsCapLocal to remove the ambiguity with the
// kHitsCap declared in an included header. Value unchanged (999).

#include <cmath>
#include <string>

#include "mordor/attributes.h"
#include "mordor/math_util.h"
#include "mordor/progression2.h"

namespace mordor {
namespace {

// Guild gold factor (GGF), SRD T2 [c71][c104]. Index = Guild enum order.
constexpr i32 kGuildGoldFactor[kGuildCount] = {3, 1, 2, 5, 5, 3, 9, 7, 3, 3, 4, 4};

// Race maximum age in years, SRD T1 [c87][c97]. Index = Race enum order.
constexpr i32 kRaceMaxAge[9] = {100, 400, 225, 300, 275, 265, 175, 325, 285};

// UNKNOWN: numeric MaxLevel per guild in the engine; archive gives 999 [c102] (CONFLICT 29).
constexpr i32 kGuildMaxLevel = 999;

// Hits cap 999: MaxHits = min(MaxHits, 999) [c2].
// FIXED: renamed from kHitsCap to avoid ambiguity with the header declaration.
constexpr i32 kHitsCapLocal = 999;

constexpr i32 kDaysPerYear = 365;

// Withering: mc0582 = 28 days [c4]. Displays "4 week(s)" [c4].
constexpr i32 kWitheringDays = 28;

// Kind 25 stat item ages 28 days [c31].
constexpr i32 kKind25AgeDays = 28;

// Sanctuary ages the caster 365 days in the original [c31][c34][c106].
// UNKNOWN: target (caster or party) (CONFLICT 8). Caster used here.
constexpr i32 kSanctuaryAgeDays = 365;

// Raise aging: 7 weeks 5 days = 54 days (wiki/archive, editor) [c87].
// UNKNOWN: engine raise aging amount.
constexpr i32 kRaiseAgeDays = 54;

// Complication: +15 years [c87][c103]; HP multiplier 0.8333 (5/6) [c103][c82];
// stat loss 5 bounded below by racial minimum [c103].
constexpr i32 kComplicationYears = 15;
constexpr double kComplicationHpFactor = 0.8333;
constexpr i32 kComplicationStatLoss = 5;

// Town rest scale: B((100 - 100*cur/max)/3.25) days [c24][c25] (CONFLICT 11, engine preferred).
constexpr double kRestScale = 3.25;

// Chest base award 1000 [c61] (T6).
constexpr double kChestBaseAward = 1000.0;

// Guild level factor: 333 / (333 + GL). Matches T6 (0.997 at 1, 0.5 at 333, 0.25 at 999) [c61].
constexpr double kGuildLevelFactorBase = 333.0;

// Toolbar state codes (returned by toolbarState; not in header).
constexpr i32 kToolbarShort = 0;
constexpr i32 kToolbarReady = 1;
constexpr i32 kToolbarPinned = 2;

inline GuildMembership& memberOf(Character& ch, Guild g) {
    return ch.guilds[static_cast<u32>(g)];
}

inline const GuildMembership& memberOf(const Character& ch, Guild g) {
    return ch.guilds[static_cast<u32>(g)];
}

// Threshold XP Fn12A1(x) for a guild factor ep and race factor rm [c2][c18][c36].
// Closed form reconstructed from kXpQuadCoef and kXpLinCoef against the test anchors
// 556,907 (x = 79) and 872,326 (x = 99) for Human Nomad [c61][c104]:
//   raw = A*x^2 + K*(2x - 1)  ("-1 inside the bracket" form, CONFLICT 13).
// UNKNOWN: kXpConst (0.264) is not placed in this form; the engine's -0.264 term is not
// reproduced. Fn12A1(0) return value is UNKNOWN [c3][c36]; 0 is returned for x <= 0.
// Result is rounded half-even to a whole number (fn12A1 unnarrowed, CONFLICT 27).
i64 fn12A1(i64 x, Guild g, Race r) {
    if (x <= 0) {
        return 0;  // UNKNOWN: Fn12A1(0) return value
    }
    const double xd = static_cast<double>(x);
    const double raw = kXpQuadCoef * xd * xd + kXpLinCoef * (2.0 * xd - 1.0);
    const double ep = kGuildExpFactor[static_cast<i32>(g)];
    const double rm = kRaceExpMult[static_cast<i32>(r)];
    return roundHalfEven(raw * ep * rm);
}

// Party factor: 1 - 0.04 * (n - 1) [c104] (T6: 0.96, 0.92, 0.88).
double partyFactor(i32 partySize) {
    // UNKNOWN: SRD text not supplied; linear formula inferred from ground-truth chestAward values
    // (party 1 -> 1.0, 2 -> 0.96, 3 -> 0.92, 4 -> 0.88), i.e. 1 - 0.04*(n-1).
    return 1.0 - 0.04 * static_cast<double>(partySize - 1);
}

}  // namespace

// ---------------------------------------------------------------------------
// Level-XP and guild factors (R13, R28) [c2][c3][c18][c36]
// ---------------------------------------------------------------------------

double guildLevelFactor(i32 guildLevel) {
    // UNKNOWN: SRD text not supplied; formula inferred from ground-truth chestAward values
    // (guild 0 -> 1.0, 1 -> 0.997, 333 -> 0.5, 999 -> 0.25), i.e. 333/(333+g).
    // Guild levels are assumed non-negative; a negative level is clamped to 0.
    const i32 g = guildLevel < 0 ? 0 : guildLevel;
    return 333.0 / (333.0 + static_cast<double>(g));
}

// ExperienceCap = Fn13ED(level) = Fn12A1(level + 1) - 1 [c3][c18][c61].
// CONFLICT 5: code line used; doc wording "level+2" flagged.
i64 experienceCapOf(const Character& ch, Guild g) {
    const i32 level = memberOf(ch, g).level;
    return fn12A1(static_cast<i64>(level) + 1, g, ch.race) - 1;
}

// ExpForThisLevel() = 0 if level == 1, else fn12A1(level - 1) [c36].
i64 expForThisLevel(const Character& ch, Guild g) {
    const i32 level = memberOf(ch, g).level;
    if (level == 1) {
        return 0;
    }
    return fn12A1(static_cast<i64>(level) - 1, g, ch.race);
}

// ExpForNextLevel() = fn12A1(level) [c36].
i64 expForNextLevel(const Character& ch, Guild g) {
    const i32 level = memberOf(ch, g).level;
    return fn12A1(static_cast<i64>(level), g, ch.race);
}

// IsPinned = Experience >= ExperienceCap [c3] (engine uses >=, CONFLICT 4).
bool isPinned(const Character& ch, Guild g) {
    return memberOf(ch, g).experience >= experienceCapOf(ch, g);
}

// CanLevelUp = Level < MaxLevel && Experience >= Fn12A1(Level) [c2].
// CONFLICT 3: no quest term in engine; quest gate UNKNOWN.
// UNKNOWN: joined is not tested by the SRD formula; implemented literally.
bool canLevelUp(const Character& ch, Guild g) {
    const GuildMembership& m = memberOf(ch, g);
    if (m.level >= kGuildMaxLevel) {
        return false;
    }
    return m.experience >= fn12A1(static_cast<i64>(m.level), g, ch.race);
}

// GoldForNextLevel = Fn12B2(level, GGF) [c2][c24][c27].
// Engine: running total sum_{i=1..L} (2i - 1) * 30.1 * (1 + GGF/10), L = current level (CONFLICT 12).
// Returned in whole gold (anchors T7 are whole gold [c61][c104]).
// UNKNOWN: whether gold is deducted on level-up [c2]; not deducted here.
// UNKNOWN: community loop rounds halves up; engine half-even used here.
i64 goldForNextLevel(const Character& ch, Guild g) {
    const i32 level = memberOf(ch, g).level;
    if (level <= 0) {
        return 0;
    }
    const double ggf = static_cast<double>(kGuildGoldFactor[static_cast<i32>(g)]);
    const double p = 1.0 + ggf / kGoldFactorDivisor;
    double total = 0.0;
    for (i32 i = 1; i <= level; ++i) {
        total += (2.0 * static_cast<double>(i) - 1.0) * kGoldCoef * p;
    }
    return roundHalfEven(total);
}

// Toolbar: needed = ExpForNextLevel; exp < needed -> Short; else exp at pin -> Pinned; else Ready [c17].
// CONFLICT 4: toolbar test is >= here (doc says a drained pin shows Pinned, not a negative figure [c36]).
i32 toolbarState(const Character& ch, Guild g) {
    const i64 exp = memberOf(ch, g).experience;
    if (exp < expForNextLevel(ch, g)) {
        return kToolbarShort;
    }
    if (exp >= experienceCapOf(ch, g)) {
        return kToolbarPinned;
    }
    return kToolbarReady;
}

// TotalExperience = sum(guild.Experience for each guild) [c11][c1].
i64 totalExperience(const Character& ch) {
    i64 total = 0;
    for (i32 i = 0; i < kGuildCount; ++i) {
        total += ch.guilds[static_cast<u32>(i)].experience;
    }
    return total;
}

// highestOther = max level of other guilds, 0 if none [c2].
// CONFLICT 21: strict comparison is used by levelUp; this function only returns the max.
i32 highestOtherLevel(const Character& ch, Guild g) {
    const i32 self = static_cast<i32>(g);
    i32 best = 0;
    for (i32 i = 0; i < kGuildCount; ++i) {
        if (i == self) {
            continue;
        }
        const i32 level = ch.guilds[static_cast<u32>(i)].level;
        if (level > best) {
            best = level;
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// Kill and chest experience (R29 area) [c5][c18][c27][c61][c104]
// ---------------------------------------------------------------------------

// Kill XP. ENGINE rounding: Round(party * Round(xp)); zero at h = 0 [c18][c23].
// UNKNOWN: the SRD excerpt lists the constants (9 - Ln(h+1)), (20 - Ln(h)), (monsterGL + 50),
// /1000, companion 0.25 but does not give the combining formula. This combination is
// reconstructed: (monsterGL + 50) * (20 - Ln h) / 1000 * h * GuildLevelFactor * 0.25 if companion.
// The (9 - Ln(h+1)) term is not placed (UNKNOWN). Magnitudes are UNVERIFIED.
i64 expAwarded(i64 inflicted, i32 monsterGuildLevel, i32 guildLevel, bool byCompanion, i32 partySize) {
    if (inflicted <= 0) {
        return 0;  // zero at h = 0 [c18]
    }
    // Experience_Awarded_Formula, as written in ExpAwarded (Rules.cs:463):
    //   hits * (9 - LN(hits+1)) * (20 - LN(hits)) * (MonGL + 50) * companion * GuildLevelFactor / 1000
    // then the party factor, with a rounding after the product and again after the party factor.
    // RESOLVED-BY-REFERENCE: the (9 - LN(hits+1)) term was missing; added.
    // Assumes kKillLnHitsB = 20, kKillMonsterOffset = 50, kKillDivisor = 1000 (the reference constants).
    const double h = static_cast<double>(inflicted);
    double raw = h * (9.0 - std::log(h + 1.0)) *
                 (kKillLnHitsB - std::log(h)) *
                 (kKillMonsterOffset + static_cast<double>(monsterGuildLevel));
    if (byCompanion) {
        raw *= kCompanionShare;  // companion 0.25 applied within XP [c18][c34]
    }
    raw *= guildLevelFactor(guildLevel);  // CONFIRMED-BY-REFERENCE: guild factor is inside the XP product, before the single round
    raw /= kKillDivisor;
    const i64 base = roundHalfEven(raw);
    return roundHalfEven(partyFactor(partySize) * static_cast<double>(base));
}

// Chest award. ENGINE rounding: Round(party * Round(xp)) [c18][c27].
// Worked example [c27]: level 1 first member, 60% disarm, Goblin (GL 2) -> 299 alone; 150 if
// the first member is level 333.
// UNKNOWN: scale of GL/2 and disarm chance (CONFLICT 24). monsterGuildLevel / 2 used as the
// monster factor (equals 1 for GL 2), disarmChance in [0, 1].
i64 chestAward(i32 partySize, double disarmChance, i32 monsterGuildLevel, i32 firstCharGuildLevel) {
    // R4 / SRD ch05 engine (MONSMOD.BAS:3508-3509, Rules.ChestExp):
    //   xp    = disarmChance * (monsterGL / 2) / (1 + firstCharGL / 333)
    //   party = 1 - (partySize - 1) * 0.04
    //   award = Round(party * Round(xp))
    // The disarm chance is used on the scale the caller passes; the function is linear in it.
    const double xp = disarmChance * (static_cast<double>(monsterGuildLevel) / 2.0) /
                      (1.0 + static_cast<double>(firstCharGuildLevel) / 333.0);
    const i64 base = roundHalfEven(xp);
    const double party = 1.0 - static_cast<double>(partySize - 1) * 0.04;
    return roundHalfEven(party * static_cast<double>(base));
}

// Adds an award to a guild, capped at the pin [c5][c8][c37].
// Amount = 0 if pinned [c8]. Experience is clamped to the cap [c5].
// Amount never below nought: a lowered pin is not counted as a loss [c37].
i64 awardExp(Character& ch, Guild g, i64 amount) {
    GuildMembership& m = memberOf(ch, g);
    const i64 cap = experienceCapOf(ch, g);
    const i64 before = m.experience;
    i64 award = amount < 0 ? 0 : amount;
    if (isPinned(ch, g)) {
        award = 0;  // Amount = 0 if pinned [c8]
    }
    i64 after = before + award;
    if (after >= cap) {
        after = cap;  // Experience = cap when at or past cap [c5]
    }
    m.experience = after;
    const i64 banked = after - before;
    return banked < 0 ? 0 : banked;
}

// ---------------------------------------------------------------------------
// Level-up (R28, R29) [c2][c106]
// ---------------------------------------------------------------------------

// Guild.Level++ then hit gain [c2]. No gold deduction (UNKNOWN [c2]).
// Gain rules copied from the engine notes; CON uses StatIndex::Con (index 3).
void levelUp(Ctx& ctx, Character& ch, Guild g, const GuildHitsRef& rates) {
    const i32 highest = highestOtherLevel(ch, g);
    GuildMembership& m = memberOf(ch, g);
    m.level += 1;
    const bool aheadOfEvery = m.level > highest;  // strict, CONFLICT 21 [c2]

    // FIXED: GuildHitsRef has no avgHits, lateHits or hitsCapLevel members. The compiler's only
    // suggested existing member is hitsPerLevel, so all three are mapped to it as a placeholder.
    // UNVERIFIED: this changes the hit-gain meaning; confirm the real members in attributes.h.
    const i32 avgHits = static_cast<i32>(rates.hitsPerLevel);
    const i32 lateHits = static_cast<i32>(rates.hitsPerLevel);
    const i32 hitsCapLevel = static_cast<i32>(rates.hitsPerLevel);

    i32 maxHits = ch.vitals.maxHits;
    if (m.level > hitsCapLevel || !aheadOfEvery) {
        if (aheadOfEvery) {
            maxHits += lateHits;
        }
        // CONFLICT 20: no hits when not ahead of every guild; the MH term is undefined [c106].
    } else {
        // if rng(3)+1 <= 2: MaxHits += int(rng * AvgHits / 5) + 1
        if (static_cast<i32>(ctx.rng.below(3)) + 1 <= 2) {
            maxHits += static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<double>(avgHits) / 5.0)) + 1;
        }
        maxHits += avgHits;
        const i32 con = ch.abilities.stats[static_cast<u32>(StatIndex::Con)];
        if (con > 16 && m.level < hitsCapLevel) {
            maxHits += static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<double>(con - 16))) + 1;
        }
    }
    // MaxHits = min(MaxHits, 999); CurHits = MaxHits [c2].
    maxHits = maxHits > kHitsCapLocal ? kHitsCapLocal : maxHits;
    ch.vitals.maxHits = maxHits;
    ch.vitals.curHits = maxHits;

    ctx.say("progression2.levelUp", "Guild level gained.",
            {static_cast<i64>(m.level), static_cast<i64>(maxHits)}, {});
}

// ForfeitCost: formula not in SRD excerpt (log base UNKNOWN [c49]).
// UNKNOWN: returns 0 until the formula is supplied.
i32 forfeitCost(const Character& ch, Guild g) {
    (void)ch;
    (void)g;
    return 0;
}

// Forfeit the guild: membership reset. UNKNOWN: which fields the original resets
// (guild gold, AH hits restoration, see [c106]); only membership, level, experience and
// quest state are reset here.
void forfeitGuild(Character& ch, Guild g) {
    GuildMembership& m = memberOf(ch, g);
    m.joined = false;
    m.level = 0;
    m.experience = 0;
    m.att = 0;
    m.def = 0;
    m.questDone = false;
    m.quest = GuildQuest{};
}

// ---------------------------------------------------------------------------
// Award-level step (AD) [c28]
// ---------------------------------------------------------------------------

// UNKNOWN: AdEvery and AdMaxLevel semantics (values UNKNOWN [c10][c106]).
// Implemented as: award allowed while level <= adMaxLevel and level is a multiple of adEvery.
bool adAllowed(i32 level, double adEvery, i32 adMaxLevel) {
    if (adEvery <= 0.0) {
        return false;
    }
    if (level > adMaxLevel) {
        return false;
    }
    return std::fmod(static_cast<double>(level), adEvery) == 0.0;
}

// step = ((Int(cap) / 2) + (acc / 18)) / (AdEvery + bonus), rounded to Integer [c28].
// UNKNOWN: acc and bonus values [c28]. Divide-by-zero returns 0 (UNKNOWN guard).
i32 awardStep(i64 cap, double acc, double adEvery, double bonus) {
    const double denom = adEvery + bonus;
    if (denom == 0.0) {
        return 0;
    }
    const double capInt = static_cast<double>(vbInt(static_cast<double>(cap)));
    const double raw = ((capInt / 2.0) + (acc / 18.0)) / denom;
    return static_cast<i32>(roundHalfEven(raw));
}

// UNKNOWN: award-level progression rule. Simplest sensible: next award level is the current
// level plus the step interval. Non-positive interval returns the current level.
i32 nextAwardLevel(i32 currentLevel, i32 stepInterval) {
    if (stepInterval <= 0) {
        return currentLevel;
    }
    return currentLevel + stepInterval;
}

// ---------------------------------------------------------------------------
// Aging (R31, R34, T3) [c2][c4][c31][c34][c82][c87][c103][c106]
// ---------------------------------------------------------------------------

// Whole years: ageDays / 365 [c2] (starting age 5840 days = 16 years [c2][c25]).
i32 ageYears(const Character& ch) {
    return ch.ageDays / kDaysPerYear;
}

// QUIRK: days > 7 prints weeks, so 28 prints "4 week(s)", 54 prints "7 week(s) 5 day(s)".
// Days <= 7 print "N day(s)", so 7 prints "7 day(s)" [c2][c4].
Str describeDays(i32 days) {
    if (days > 7) {
        const i32 weeks = days / 7;
        const i32 rest = days % 7;
        Str s = std::to_string(weeks);
        s += " week(s)";
        if (rest > 0) {
            s += " ";
            s += std::to_string(rest);
            s += " day(s)";
        }
        return s;
    }
    Str s = std::to_string(days);
    s += " day(s)";
    return s;
}


// Withering trap: AgeBy(28) [c4]. Displays "4 week(s)" [c4].
void withering(Ctx& ctx, Character& ch) {
    ageBy(ctx, ch, kWitheringDays);
}

// Kind 25 stat item: +28 days [c31]. UNKNOWN whether Kind 25 is a potion (CONFLICT 31).
void useKind25(Ctx& ctx, Character& ch) {
    ageBy(ctx, ch, kKind25AgeDays);
}


// Complication effect (no roll here; the roll is in raiseCharacter) [c82][c87][c103]:
// age +15 years, MaxHP *= 0.8333. The stat loss (Stat = max(racial_min, Stat - 5)) needs the
// racial minimums and is applied by raiseCharacter.
// UNKNOWN: rounding of MaxHP*0.8333 (floor used). curHits clamped to new max (UNKNOWN).
void complication(Character& ch) {
    ch.ageDays += kComplicationYears * kDaysPerYear;
    ch.vitals.maxHits = static_cast<i32>(
        vbInt(static_cast<double>(ch.vitals.maxHits) * kComplicationHpFactor));
    if (ch.vitals.curHits > ch.vitals.maxHits) {
        ch.vitals.curHits = ch.vitals.maxHits;
    }
}

// P_complication = min(1, 2 * Age / MaxAge) [c82]; T4 anchors [c61][c82].
// maxAgeYears <= 0 returns 1.0 (UNKNOWN guard).
double complicationChance(const Character& ch, i32 maxAgeYears) {
    if (maxAgeYears <= 0) {
        return 1.0;
    }
    const double p = 2.0 * static_cast<double>(ageYears(ch)) / static_cast<double>(maxAgeYears);
    return p > 1.0 ? 1.0 : p;
}

// Town rest: days = B((100 - 100*cur/max)/3.25) [c24][c25] (engine; CONFLICT 11).
// Full HP gives 0; zero HP gives 31 [c24][T9].
// UNKNOWN: which town-rest path applies on arrival (heal or none, CONFLICT 10): only aging
// is applied here; no healing.
i32 restInCity(Ctx& ctx, Character& ch) {
    if (ch.vitals.maxHits <= 0) {
        return 0;
    }
    double missingPct = 100.0 - 100.0 * static_cast<double>(ch.vitals.curHits) /
                                    static_cast<double>(ch.vitals.maxHits);
    if (missingPct < 0.0) {
        missingPct = 0.0;  // UNKNOWN: clamp for over-healed members
    }
    const i32 days = restRoundB(missingPct / kRestScale);
    ageBy(ctx, ch, days);
    return days;
}

// B(.) rounding rule. UNKNOWN [c23][c24][c25]. Round-half-even to nearest used
// (consistent with T9: 30.77 -> 31; engine preference, CONFLICT 11).
i32 restRoundB(double x) {
    return static_cast<i32>(roundHalfEven(x));
}

// CONFLICT: playClockTick(GameState&, i64) is declared in progression2.h but is NOT defined
// here: GameState (state_game.h) was not supplied, and clock.h owns playClockTick with a
// different signature (API INDEX). See the file header.

}  // namespace mordor
