#include "test_support.h"
#include "mordor/progression2.h"

#include <cmath>
#include "mordor/shared_helpers.h"

using namespace mordor;

namespace {

GuildMembership& memb(Character& ch, Guild g) {
    return ch.guilds[static_cast<u32>(g)];
}

void setGuild(Character& ch, Guild g, i32 level, i64 exp) {
    GuildMembership& m = memb(ch, g);
    m.joined = true;
    m.level = level;
    m.experience = exp;
}

bool closeTo(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

// ---- T5: CON loss after a successful raise (chance in percent) ----

// ---- Complication chance: min(1, 2*Age/MaxAge) ----

// ---- Age in years ----

TEST(prog2_start_age_constant_is_sixteen_years) {
    CHECK_EQ(kAgeStartDays, 16 * 365);
}

// ---- Day display quirks ----

// ---- Aging events (T3) ----

TEST(prog2_age_by_adds_days) {
    tsup::Env env({0.5, 0.9});
    Character ch;
    ch.ageDays = kAgeStartDays;
    ageCharacterDays(ch, 28, &env.ctx);
    CHECK_EQ(ch.ageDays, kAgeStartDays + 28);
}

// ---- Experience thresholds: structural relations ----

// ---- Gold anchors (T7, engine convention: current level) ----

TEST(prog2_gold_for_next_level_anchors) {
    // 30.1 * L^2 * (1 + GGF/10): Warrior GGF 1 at L1 = 33.11 -> 33.
    Character warrior;
    setGuild(warrior, Guild::Warrior, 1, 0);
    CHECK_EQ(goldForNextLevel(warrior, Guild::Warrior), 33);

    // Nomad GGF 3 at L3 = 30.1 * 9 * 1.3 = 352.17 -> 352.
    Character nomad;
    setGuild(nomad, Guild::Nomad, 3, 0);
    CHECK_EQ(goldForNextLevel(nomad, Guild::Nomad), 352);
}

// ---- Totals and cross-guild level ----

// ---- Awarding experience (pin cap) ----

// ---- Monster kill XP: documented zero and relations ----

// ---- Constant tables ----

TEST(prog2_race_and_guild_constant_tables) {
    // T1 raw Exp 6 -> Ogre is not used here; Morloch raw Exp 4 maps to 1.05819132268.
    CHECK(closeTo(kRaceExpMult[static_cast<i32>(Race::Morloch)], 1.05819132268, 1e-11));
    CHECK(closeTo(kRaceExpMult[static_cast<i32>(Race::Troll)], 1.25647909518, 1e-11));
    CHECK(closeTo(kRaceExpMult[static_cast<i32>(Race::Human)], 1.0, 1e-11));

    // T2 EP values; Ninja and Villain share 2.625.
    CHECK(closeTo(kGuildExpFactor[static_cast<i32>(Guild::Wizard)], 3.75, 1e-12));
    CHECK(closeTo(kGuildExpFactor[static_cast<i32>(Guild::Healer)], 4.0, 1e-12));
    CHECK(closeTo(kGuildExpFactor[static_cast<i32>(Guild::Ninja)],
                  kGuildExpFactor[static_cast<i32>(Guild::Villain)], 1e-12));
}

TEST(prog2_documented_scalar_constants) {
    CHECK(closeTo(kCompanionShare, 0.25, 1e-12));
    CHECK(closeTo(kChestGuildShare, 0.5, 1e-12));
    CHECK(closeTo(kPartyPenaltyPerMember, 0.04, 1e-12));
    CHECK(closeTo(kGoldCoef, 30.1, 1e-12));
    CHECK(closeTo(kGoldFactorDivisor, 10.0, 1e-12));
}
