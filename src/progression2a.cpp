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

}  // namespace

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

}  // namespace mordor
