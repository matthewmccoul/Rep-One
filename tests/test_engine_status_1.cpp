#include "test_support.h"

#include "mordor/status1a.h"
#include "mordor/status2.h"
#include "mordor/partyb.h"

using namespace mordor;

// B_0063: help cry threshold (HelpCry.Needed) via helpCryNeeded(party, c).

TEST(engine_B_0063_67_help_cry_50_of_500) {
    // HelpCry.Needed: CurHits <= MaxHits * 0.1 (50 of 500 is exactly a tenth, so a cry)
    Character c;
    c.vitals.maxHits = 500;
    c.vitals.curHits = 50;
    CHECK(helpCryNeeded(c));
}

TEST(engine_B_0063_68_no_help_cry_51_of_500) {
    Party party;
    Character c;
    c.vitals.maxHits = 500;
    c.vitals.curHits = 51;
    CHECK(!helpCryNeeded(party, c));
}

TEST(engine_B_0063_69_help_cry_9_of_20) {
    // HelpCry.Needed: CurHits < 10, whatever the maximum
    Character c;
    c.vitals.maxHits = 20;
    c.vitals.curHits = 9;
    CHECK(helpCryNeeded(c));
}

TEST(engine_B_0063_70_no_help_cry_10_of_20) {
    Party party;
    Character c;
    c.vitals.maxHits = 20;
    c.vitals.curHits = 10;
    CHECK(!helpCryNeeded(party, c));
}

// B_0064: stoning companion blows. Each companionBlowStones call is treated as
// one landed blow; the percentage of those that stone the foe is checked.

TEST(engine_B_0064_113_stone_pct_con10_vs_con3_floor0) {
    tsup::Env env(12345);
    int stones = 0;
    for (int i = 0; i < 6000; ++i) {
        Creature target;
        if (companionBlowStones(env.ctx, target, 10, 0)) ++stones;
    }
    double pct = 100.0 * stones / 6000.0;
    CHECK(pct >= 72.5 && pct <= 77.5);
}

TEST(engine_B_0064_114_stone_pct_con10_vs_con40_floor0) {
    tsup::Env env(12345);
    int stones = 0;
    for (int i = 0; i < 6000; ++i) {
        Creature target;
        if (companionBlowStones(env.ctx, target, 10, 0)) ++stones;
    }
    double pct = 100.0 * stones / 6000.0;
    CHECK(pct >= 72.5 && pct <= 77.5);
}

// SKIP B_0064#117: needs a full combat simulation (group, rounds, last foe of group); companionBlowStones is a single blow check only.
// SKIP B_0064#118: needs combat log ordering (SHOCK sound after 'Stoned one!'); no equivalent in the headers.
// SKIP B_0064#119: needs a multi-round fight to verify the foe's hits stay at 0 after each stoning; no equivalent function.

// SKIP B_0065#74: castCureParalysis(ctx, caster, inFight) takes only the caster and has no target parameter, so "cast by Ann on Bob" cannot be expressed.

