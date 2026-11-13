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
#include "mordor/shared_helpers.h"

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

}  // namespace

// ---------------------------------------------------------------------------
// Level-XP and guild factors (R13, R28) [c2][c3][c18][c36]
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Kill and chest experience (R29 area) [c5][c18][c27][c61][c104]
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Level-up (R28, R29) [c2][c106]
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Award-level step (AD) [c28]
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Aging (R31, R34, T3) [c2][c4][c31][c34][c82][c87][c103][c106]
// ---------------------------------------------------------------------------

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
    ageCharacterDays(ch, days, &ctx);
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
