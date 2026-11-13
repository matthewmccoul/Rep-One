// death3.cpp: morgue, revival, raise and records rules (chapter 18 part c18p3).
//
// Definitions NOT in this file (declared in death3.h but owned by death1b.h per the API INDEX):
//   i64 hallStatField(i64 value);
//   i64 hallDateField(i64 daysSince1900);
// CONFLICT: death3.h redeclares these two; the API INDEX assigns them to death1b. They are not
// defined here to keep every function defined exactly once.
//
// Delegations to other modules (API INDEX):
//   morgueCounterChance(i32 naturalCon, DeathState cause)   [death1a.h]
//   spellRaiseChance(i32 naturalCon, RaiseKind, bool causeIsRock) [death1b.h]

#include "mordor/death3.h"

#include <cmath>

#include "mordor/death1a.h"
#include "mordor/death1b.h"
#include "mordor/events.h"
#include "mordor/rng.h"

namespace mordor {
namespace {

// R-rule constants local to this file.
// FIXED: renamed to avoid ambiguity with same-named declarations in mordor:: (values unchanged).
constexpr i32 kLocalRaiseChanceMin = 10;   // clamp floor for morgue thresholds (engine, c19)
constexpr i32 kLocalRaiseChanceMax = 98;   // clamp ceiling for morgue thresholds (engine, c19)
// UNKNOWN: race maximum-age table is not in the input. 100 is a placeholder (c25 notes lifespan 100).
constexpr i32 kDefaultRaceMaxAge = 100;
// UNKNOWN: unit of ageDays vs the age used by the complication test (years assumed).
// FIXED: renamed to avoid ambiguity with same-named declarations in mordor:: (value unchanged).
constexpr i32 kLocalDaysPerYear = 365;

// The reference reads the stored Constitution with no modifier term.
// FIXED: naturalCon was not declared in any header; defined here as a static helper.
// Natural CON: the stored Constitution stat, with no modifier term.
static i32 naturalCon(const Character& ch) {
    return ch.abilities.stats[static_cast<u32>(StatIndex::Con)];
}

}  // namespace

// R18 morgue threshold, own body: CON + 40, clamped (engine c19, c50).
i32 morgueOwnBodyChance(const Character& ch) {
    return clampRaiseChance(naturalCon(ch) + kMorgueOwnBodyBonus);
}

// R18 morgue threshold, counter: delegated to death1a (engine c17), then clamped.
// CONFLICT: Int() vs round_to_int rounding (c17 vs c30); the death1a owner picks the engine rounding.
i32 morgueCounterChance(const Character& ch) {
    return clampRaiseChance(morgueCounterChance(naturalCon(ch), ch.deathState));
}

// R18 morgue threshold dispatch by route.
i32 morgueChance(const Character& ch, MorgueRoute route) {
    switch (route) {
        case MorgueRoute::OwnBody:
            return morgueOwnBodyChance(ch);
        case MorgueRoute::Counter:
            return morgueCounterChance(ch);
    }
    return 0;
}

// R18 morgue chance clamp, 10..98 (engine c19; c24 says no clamp, engine preferred).
i32 clampRaiseChance(i32 raw) {
    return clampv(raw, kLocalRaiseChanceMin, kLocalRaiseChanceMax);
}

// Spell raise chance: delegated to death1b (engine c19). Rock stored value 10 is handled there.
i32 spellRaiseChance(const Character& ch, RaiseKind kind) {
    return spellRaiseChance(naturalCon(ch), kind, ch.deathState == DeathState::Rock);
}

// R18 old-age term: 5 + Ln(pct - 98) (c18). Non-old input returns 0.
// UNKNOWN: for pct == 98 Ln(0) is -infinity; the chunk gives no clamp (c18 note 13). Kept literal.
double oldAgeTerm(i32 pct) {
    if (!isOldAgePct(pct)) {
        return 0.0;
    }
    return kOldAgeBase + std::log(static_cast<double>(pct - kOldAgePct));
}

// R18 old-age test: formula uses pct >= 98 (c18; rule text says "passes 98%", note 13).
bool isOldAgePct(i32 pct) {
    return pct >= kOldAgePct;
}

// R18 revival quote: fee, guild-pays and asks-payer (engine c17, c28, c50).
RevivalPayment quoteRevival(const Character& ch) {
    RevivalPayment p;
    p.fee = revivalFee(ch);
    p.guildPays = guildPaysRevival(ch, p.fee);
    // Level 5 or under: no question asked (paid by guild or from purse). Above 5: asks payer.
    p.asksPayer = ch.topGuildLevel > kGuildPaysMaxLevel;
    return p;
}

// R18 revival fee: Int(2912.32 * (MaxHits / 1.5)), in whole gold (engine c17, note 20).
// UNKNOWN: whether the Int() result is whole gold; taken as whole gold.
Currency revivalFee(const Character& ch) {
    const double hitsDiv = static_cast<double>(ch.vitals.maxHits) / kRevivalFeeHitDivisor;
    const i64 gold = vbInt(kRevivalFeeFactor * hitsDiv);
    return currencyFromWhole(gold);
}

// R18 guild pays: TopGuildLevel <= 5 and purse short of fee (engine c50, note 6).
bool guildPaysRevival(const Character& ch, Currency fee) {
    return ch.topGuildLevel <= kGuildPaysMaxLevel && ch.purse() < fee;
}

// R18 morgue refusal (engine c49, c50). Rock is refused (note 3, engine preferred).
// UNKNOWN: level test uses active guild level; order of rock vs level tests not stated.
MorgueRefusal morgueRefusal(const Character& body) {
    if (body.deathState == DeathState::Rock) {
        return MorgueRefusal::RockCannotHelp;
    }
    const i32 lvl = body.activeLevel();
    if (lvl < kMorgueMinLevel) {
        return MorgueRefusal::LevelTooLow;
    }
    if (lvl > kMorgueMaxLevel) {
        return MorgueRefusal::LevelTooHigh;
    }
    return MorgueRefusal::None;
}

// R18 complication guaranteed when Age >= RaceMaxAge / 2 (engine c17, c24, c97; note 25).
bool complicationGuaranteed(i32 age, i32 raceMaxAge) {
    return age >= raceMaxAge / 2;
}

// R18 complicated max hits: Int(MaxHits / 1.2) (engine c17, c27; note 26).
i32 complicatedMaxHits(i32 maxHits) {
    return static_cast<i32>(vbInt(static_cast<double>(maxHits) / kComplicationHitDivisor));
}

// R18 age added by cause: 54 quiet, 84 stoned, 124 rock (c27, note 5).
// UNKNOWN: port equivalents for causes 3-7 (Destroyed, Immolated, Frozen, Melted, Annihilated);
// they take the quiet-failure value here.
i32 raiseAgeCost(DeathState cause) {
    if (cause == DeathState::Stoned) {
        return kAgeStoned;
    }
    if (cause == DeathState::Rock) {
        return kAgeRock;
    }
    return kAgeQuietFailure;
}

// R18 living recovery at morgue: CurHits <= 20% of MaxHits (engine c17).
// UNKNOWN: whether the port implements this test (note 17); engine version used.
bool livingRecoversAtMorgue(const Character& ch) {
    if (ch.isDead()) {
        return false;
    }
    return static_cast<double>(ch.vitals.curHits) <=
           kLivingRecoverFraction * static_cast<double>(ch.vitals.maxHits);
}

// R18 prune the dead: dead with CurHits <= 0 (prose/test, note 10). UNKNOWN which governs.
bool isPruneCandidate(const Character& ch) {
    return ch.isDead() && ch.vitals.curHits <= 0;
}

// R18 city rest: dead characters are not rested until raised (engine c48, note 7).
// CONFLICT: FIDELITY c31 rests the dead; engine preferred.
bool cityRestApplies(const Character& ch) {
    return !ch.isDead();
}

// R18 guild demotion. UNKNOWN: number of levels N not given; one level assumed (note list, c10).
void demoteGuildLevel(Ctx& ctx, Character& ch, Guild guild) {
    GuildMembership& g = ch.guilds[static_cast<u32>(guild)];
    if (g.level <= 0) {
        return;
    }
    g.level -= 1;
    ctx.say("death.guild.demoted", "Guild level reduced by one.", {g.level}, {});
}

// R18 raise attempt (engine c17, c19, c50; notes 3, 5, 9, 22, 27).
PlayerRaiseResult raiseCharacter(Ctx& ctx, Character& ch, RaiseKind kind, i32 chance) {
    PlayerRaiseResult res;

    // Refusals, checked before any roll.
    if (kind == RaiseKind::None) {
        res.outcome = RaiseAttemptOutcome::Refused;
        ctx.say("death.raise.refused", "No raise was offered.", {}, {});
        return res;
    }
    if (kind == RaiseKind::Morgue && morgueRefusal(ch) != MorgueRefusal::None) {
        res.outcome = RaiseAttemptOutcome::Refused;
        ctx.say("death.raise.refused", "The morgue cannot help.", {}, {});
        return res;
    }
    // Rock is lifted only by Resurrect (note 22, engine c19).
    if (ch.deathState == DeathState::Rock && kind != RaiseKind::Resurrect) {
        res.outcome = RaiseAttemptOutcome::Refused;
        ctx.say("death.raise.refused", "Only Resurrect lifts rock.", {}, {});
        return res;
    }
    // Stone accepts Restore Flesh; morgue also passes (note 9, UNKNOWN whether intended).
    if (ch.deathState == DeathState::Stoned && kind != RaiseKind::RestoreFlesh &&
        kind != RaiseKind::Morgue) {
        res.outcome = RaiseAttemptOutcome::Refused;
        ctx.say("death.raise.refused", "Stone accepts only Restore Flesh.", {}, {});
        return res;
    }

    const DeathState cause = ch.deathState;

    // Stone costs 1 CON on any raise, regardless of outcome (engine c17, note 27).
    if (cause == DeathState::Stoned) {
        ch.abilities.stats[static_cast<u32>(StatIndex::Con)] -= kStoneConLoss;
        res.conLost = kStoneConLoss;
    }

    // RESOLVED-BY-REFERENCE: Raising.cs:110 `if (c.CauseOfDeath == Rules.DeathRock) roll = 0;`
    // Being pulled into rock zeroes the chance before the roll.
    const i32 effChance = (cause == DeathState::Rock) ? 0 : chance;
    res.chanceUsed = effChance;

    // Roll against the chance.
    if (ctx.rng.percent(effChance)) {
        res.outcome = RaiseAttemptOutcome::Success;
        res.ageAdded = 0;
        ch.deathState = DeathState::Alive;
        // Raise Dead and Restore Flesh give one hit; Resurrect and the morgue give all of them.
        // REFERENCE: Raising.cs:110 `c.CurHits = kind is ByResurrect or ByMorgue ? c.MaxHits : 1;`
        ch.vitals.curHits = (kind == RaiseKind::Resurrect || kind == RaiseKind::Morgue)
                                ? ch.vitals.maxHits
                                : 1;
        ch.pendingRaise = false;
        ch.raiseKind = RaiseKind::None;
        ctx.say("death.raise.success", "The character is raised.", {}, {});
        return res;
    }

    // Failure. RESOLVED-BY-REFERENCE: Raising.cs:38 and Raising.cs:110 (Complicates).
    // Rock always complicates and draws no roll; otherwise the complication roll is
    // Int(Rnd*100) < (ageDays/365)/(raceLifespan/2)*100. Rock is checked first, as in the reference.
    // The old-age guarantee is the case where that chance reaches 100.
    const double complicationPct =
        (kDefaultRaceMaxAge > 0)
            ? (static_cast<double>(ch.ageDays) / 365.0) /
                  (static_cast<double>(kDefaultRaceMaxAge) / 2.0) * 100.0
            : 0.0;
    const bool complicates =
        (cause == DeathState::Rock) ||
        (static_cast<double>(ctx.rng.below(100)) < complicationPct);

    if (complicates) {
        res.outcome = RaiseAttemptOutcome::Complicated;
        // Age added on a complicated failure is the cause-based value (CONFIRMED by Raising.cs:110,
        // AgeBy(AgeDaysFor(CauseOfDeath)) runs on every path that is not own-body).
        res.ageAdded = raiseAgeCost(cause);
        ch.ageDays += res.ageAdded;
        ch.deathState = DeathState::Alive;
        ch.vitals.maxHits = complicatedMaxHits(ch.vitals.maxHits);
        // Hits after a complicated raise follow the same kind rule as success (Raising.cs:110).
        ch.vitals.curHits = (kind == RaiseKind::Resurrect || kind == RaiseKind::Morgue)
                                ? ch.vitals.maxHits
                                : 1;
        ch.pendingRaise = false;
        ch.raiseKind = RaiseKind::None;
        ctx.say("death.raise.complicated", "The raise is complicated.",
                {static_cast<i64>(res.ageAdded), static_cast<i64>(ch.vitals.maxHits)}, {});
        return res;
    }

    // Quiet failure: stays dead, age 54 (engine c17, note 5).
    // REFERENCE: Raising.cs:110 `outcome.Lines.Add(c.AgeBy(AgeDaysOrdinary, ...)); return outcome;`
    // The quiet failure returns before any morgue Constitution roll.
    res.outcome = RaiseAttemptOutcome::QuietFailure;
    res.ageAdded = kAgeQuietFailure;
    ch.ageDays += res.ageAdded;
    res.conRollMade = false;
    ctx.say("death.raise.failed", "The raise fails.",
            {static_cast<i64>(res.ageAdded)}, {});
    return res;
}

// hallDateField and hallStatField: see the note at the top of this file (owned by death1b).

// R18 hall wealthiest record: stored as whole gold (note 15; example "1000.5 takes 1000").
// UNKNOWN: the record's stored value is self-contradictory in the source; truncation to whole gold.
Currency hallWealthiestRecord(Currency bank) {
    return currencyFromWhole(currencyWhole(bank));
}

}  // namespace mordor
