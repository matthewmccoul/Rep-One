// death2b.cpp: old-age death (R36) and party copy lookup (R37).
#include "mordor/death2b.h"

#include <cmath>

#include "mordor/ctx.h"
#include "mordor/events.h"
#include "mordor/rng.h"

namespace mordor {

namespace {
// UNKNOWN: year length in days. 365 is used because kAgeStartDays = 16 * 365.
inline constexpr double kDaysPerYear = 365.0;
}  // namespace

// R36: percent of max age reached. Age in years over race lifespan, times 100.
// Engine formula is R19, which is not included in this SRD part.
double oldAgePercent(const Character& ch, i32 raceLifespanYears) {
    if (raceLifespanYears <= 0) {
        return 0.0;  // guard: no lifespan means no old-age check
    }
    const double ageYears = static_cast<double>(ch.ageDays) / kDaysPerYear;
    return 100.0 * ageYears / static_cast<double>(raceLifespanYears);
}

// R36: threshold = 5 + Ln(pct - 98). The chance rises as age approaches max age.
double oldAgeThreshold(double pct) {
    if (!(pct > kOldAgeStartPct)) {
        // Port guard: Ln(0) at exactly 98% is a runtime error in the original.
        return 0.0;
    }
    return kOldAgeBaseChance + std::log(pct - kOldAgeStartPct);
}

// R36: the random old-age death begins after reaching 98% of max age, in the dungeon.
// UNKNOWN: whether an already dead character is excluded here. The SRD does not say.
bool oldAgeCheckApplies(const Character& ch, i32 raceLifespanYears, bool inDungeon) {
    if (!inDungeon) {
        return false;
    }
    return oldAgePercent(ch, raceLifespanYears) > kOldAgeStartPct;
}

// R36: roll the old-age death. Returns true when the character dies.
bool rollOldAgeDeath(Ctx& ctx, Character& ch, i32 raceLifespanYears, bool inDungeon) {
    if (!oldAgeCheckApplies(ch, raceLifespanYears, inDungeon)) {
        return false;
    }
    const double pct = oldAgePercent(ch, raceLifespanYears);
    const double threshold = oldAgeThreshold(pct);
    if (!ctx.rng.percent(threshold)) {
        return false;
    }
    // Ordinary death: DeathState::Dead (Table 1, value 0 in this port).
    ch.deathState = DeathState::Dead;
    // UNKNOWN: whether ch.deaths is incremented here or by the caller. The SRD does not say.
    // "Still [is] not a permanent one" (archive quirk): raising is handled elsewhere.
    ctx.say("death.oldAge", "A character has died of old age in the dungeon.", {}, {ch.name});
    return true;
}

// R37: resolve the picked character to the party's own copy (PartyCopyOf).
// First match by identity. UNKNOWN fallback: match by name, since the SRD does not
// say how the picked copy relates to the party copy. Returns -1 (kCarryNone) if not found.
i32 partyCopyIndexOf(const Party& party, const Character& picked) {
    i32 n = party.size;
    if (n > kPartyMaxSize) {
        n = kPartyMaxSize;
    }
    if (n < 0) {
        n = 0;
    }
    for (i32 i = 0; i < n; ++i) {
        if (&party.members[static_cast<u32>(i)] == &picked) {
            return i;
        }
    }
    for (i32 i = 0; i < n; ++i) {
        if (party.members[static_cast<u32>(i)].name == picked.name) {
            return i;
        }
    }
    return -1;
}

}  // namespace mordor
