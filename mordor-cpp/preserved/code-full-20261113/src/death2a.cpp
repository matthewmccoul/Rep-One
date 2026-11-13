// death2a.cpp - Death module part A: complication roll, CON-loss rolls, raise age.
// SRD: R25 (complication chance), R26 (CON-loss roll), Table 4 (age added by cause).
#include "mordor/death2a.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

constexpr i32 kDaysPerYear = 365;        // R25 days per year
constexpr i32 kComplicationDrawRange = 100;  // R25 rng.Next(100): integer draw 0..99
constexpr i32 kConLossBase = 50;         // R26 constant 50
constexpr i32 kConLossPerCon = 2;        // R26 constant 2
constexpr i32 kConLossDrawRange = 100;   // R26 Int(Rnd*100): integer draw 0..99
constexpr i32 kConLossStep = 1;          // CONFLICT 27 (stone costs 1 CON); UNKNOWN: loss amount for other paths
constexpr i32 kAgeStoned = 84;           // Table 4 (stoned)
constexpr i32 kAgeRock = 124;            // Table 4 (rock)
constexpr i32 kAgeOther = 54;            // Table 4 (other causes)

}  // namespace

// R25: complicates if rng.Next(100) < chance%, where
// chance% = (Age_days / 365) / (RaceMaxAge / 2) * 100 = 200 * ageYears / lifespan.
// Guaranteed when Age >= RaceMaxAge / 2.
bool rollComplication(Ctx& ctx, i32 ageDays, i32 raceLifespanYears) {
    // RESOLVED-BY-REFERENCE: Raising.cs ComplicationChance returns 0 when raceLifespanYears <= 0, and
    // Complicates still evaluates rng.Next(100) < chance, so a draw is taken with chance 0 (never true).
    // RESOLVED-BY-REFERENCE: Raising.cs Complicates has no guaranteed branch. At or past half lifespan the
    // chance is >= 100 and the draw is still taken, so the port takes the draw there too.
    // Assumes kComplicationDrawRange == 100, matching the reference's rng.Next(100).
    // Rock (DeathRock) complicates with no draw in the reference (short-circuit `||`); the caller must
    // handle rock before calling this function.
    double chance = 0.0;
    if (raceLifespanYears > 0) {
        // R25 chance% = (Age_days/365)/(RaceMaxAge/2)*100 = 200*age/(365*lifespan), uncapped.
        chance = 200.0 * static_cast<double>(ageDays) /
                 (static_cast<double>(kDaysPerYear) * static_cast<double>(raceLifespanYears));
    }
    const double draw = static_cast<double>(ctx.rng.below(kComplicationDrawRange));
    return draw < chance;
}

// R26: chance = max(0, 50 - CON*2) using natural CON.
i32 conLossChancePercent(i32 naturalCon) {
    const i32 value = kConLossBase - naturalCon * kConLossPerCon;
    return value < 0 ? 0 : value;
}

// R26 morgue roll: roll = 50 - Con*2.
// Constitution loss is unfloored outside complications (MAINMOD2.BAS:353, 359),
// so the value may be zero or negative (never loses at CON 25 and above).
i32 morgueConLossThreshold(i32 con) {
    return kConLossBase - con * kConLossPerCon;
}

// R26: loss if Int(Rnd*100) < threshold. The draw is always made, even when
// the threshold is zero or negative, because the original always rolls.
// UNKNOWN: loss removes one point of natural CON with no floor, since no floor is stated.
bool rollConLoss(Ctx& ctx, Character& ch, i32 threshold) {
    const i64 draw = static_cast<i64>(ctx.rng.below(kConLossDrawRange));
    if (!(draw < static_cast<i64>(threshold))) {
        return false;
    }
    const u32 con = static_cast<u32>(StatIndex::Con);
    ch.abilities.stats[con] -= kConLossStep;  // CONFLICT 27: 1 CON per loss
    // UNKNOWN: statMax and statMods are not changed here; the event carries no numbers.
    ctx.say("death2a.con_loss", "Constitution was lost in a raise", {}, {});
    return true;
}

// Stone path (every raise): natural-CON loss chance (R26), then the roll.
bool stoneConLossRoll(Ctx& ctx, Character& ch) {
    const i32 naturalCon = ch.abilities.stats[static_cast<u32>(StatIndex::Con)];
    return rollConLoss(ctx, ch, conLossChancePercent(naturalCon));
}

// Morgue path (kind 23 only; the caller applies the M58DC guard).
// UNKNOWN: whether the morgue path uses natural or modified CON; natural CON is used here.
bool morgueConLossRoll(Ctx& ctx, Character& ch) {
    const i32 naturalCon = ch.abilities.stats[static_cast<u32>(StatIndex::Con)];
    return rollConLoss(ctx, ch, morgueConLossThreshold(naturalCon));
}

// Table 4: age added by cause. Stoned 84, Rock 124, Other 54.
// Dead (0 and 1) and causes 3..7 fall under Other.
// UNKNOWN: effects of causes 3..7 are not given; they are treated as Other.
// CONFLICT 5: the engine's quiet failure adds 54 regardless of cause; the caller
// must apply that for failed raises. This function gives the cause-based value
// used for all other raises.
i32 raiseAgeDays(DeathState cause) {
    switch (cause) {
        case DeathState::Stoned:
            return kAgeStoned;
        case DeathState::Rock:
            return kAgeRock;
        default:
            return kAgeOther;
    }
}

}  // namespace mordor
