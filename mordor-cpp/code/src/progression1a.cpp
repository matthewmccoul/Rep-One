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

// R5: ExperienceCap = Fn13ED(level) = Fn12A1(level+1) - 1.
i64 experienceCap(const Ctx& ctx, const Character& ch, Guild g) {
    const GuildMembership& m = ch.guilds[static_cast<u32>(g)];
    return levelThresholdXp(m.level + 1, guildExpFactor(ctx, g), raceExpFactor(ctx, ch.race)) - 1;
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
