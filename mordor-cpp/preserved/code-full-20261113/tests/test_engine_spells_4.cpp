#include "test_support.h"

#include "mordor/spells1b.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

using namespace mordor;

namespace {

struct FateRunStats {
    i32 minHits = 0;
    i32 maxHits = 0;
    double meanHits = 0.0;
    i32 minSpellPoints = 0;
    i32 maxSpellPoints = 0;
};

// Puts one character with 40 max hits and 40 max spell points through the
// Fate vitals reroll 8000 times and records the extremes and mean.
FateRunStats runFateTrapDraws(tsup::Env& env, i32 draws) {
    Character who;
    who.vitals.maxHits = 40;
    who.vitals.curHits = 40;
    who.vitals.maxSpellPoints = 40;
    who.vitals.curSpellPoints = 40;

    FateRunStats s;
    s.minHits = 1000000;
    s.maxHits = -1000000;
    s.minSpellPoints = 1000000;
    s.maxSpellPoints = -1000000;
    i64 hitSum = 0;

    for (i32 i = 0; i < draws; ++i) {
        rerollFateVitals(env.ctx, who);
        const i32 hits = who.vitals.curHits;
        const i32 sp = who.vitals.curSpellPoints;
        if (hits < s.minHits) s.minHits = hits;
        if (hits > s.maxHits) s.maxHits = hits;
        if (sp < s.minSpellPoints) s.minSpellPoints = sp;
        if (sp > s.maxSpellPoints) s.maxSpellPoints = sp;
        hitSum += hits;
    }
    s.meanHits = static_cast<double>(hitSum) / static_cast<double>(draws);
    return s;
}

}  // namespace

// B_0063#32-36: chest trap 6 (Fate), 8000 draws, seed 20260922.
TEST(engine_B_0063_32_fate_trap_lowest_hits) {
    tsup::Env env(20260922);
    FateRunStats s = runFateTrapDraws(env, 8000);
    CHECK_EQ(s.minHits, 1);
}

TEST(engine_B_0063_33_fate_trap_highest_hits) {
    tsup::Env env(20260922);
    FateRunStats s = runFateTrapDraws(env, 8000);
    CHECK_EQ(s.maxHits, 40);
}

TEST(engine_B_0063_34_fate_trap_mean_hits) {
    tsup::Env env(20260922);
    FateRunStats s = runFateTrapDraws(env, 8000);
    CHECK(s.meanHits >= 20.0);
    CHECK(s.meanHits <= 21.0);
}

TEST(engine_B_0063_35_fate_trap_lowest_spell_points) {
    tsup::Env env(20260922);
    FateRunStats s = runFateTrapDraws(env, 8000);
    CHECK_EQ(s.minSpellPoints, 1);
}

TEST(engine_B_0063_36_fate_trap_highest_spell_points) {
    tsup::Env env(20260922);
    FateRunStats s = runFateTrapDraws(env, 8000);
    CHECK_EQ(s.maxSpellPoints, 40);
}

// B_0063#37: character at 1 current hit (40 max) receives Fate 2000 times.
// Each cast starts from 1 current hit; count casts that rise above 1.
TEST(engine_B_0063_37_fate_casts_rising_above_one) {
    tsup::Env env(20260922);
    Character who;
    who.vitals.maxHits = 40;

    i32 rose = 0;
    for (i32 i = 0; i < 2000; ++i) {
        who.vitals.curHits = 1;
        rerollFateVitals(env.ctx, who);
        if (who.vitals.curHits > 1) {
            ++rose;
        }
    }
    CHECK(rose >= 1900);
    CHECK(rose <= 1990);
}

// SKIP B_0063#101: DungeonNav.StepThrough has no equivalent; rock layout behind the move cannot be set up from the headers.
// SKIP B_0063#102: DungeonNav.StepThrough has no equivalent; rock layout behind the move cannot be set up from the headers.
// SKIP B_0063#104: Teleporting.ForSpell has no equivalent; a rock destination square cannot be set up from the headers.
// SKIP B_0063#105: SpellEffect.TeleportTo body placement has no equivalent; a rock destination square cannot be set up from the headers.
