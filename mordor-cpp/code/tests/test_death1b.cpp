// FIXED: the 15 -Wnarrowing errors all come from `tsup::Env({<double>})`, where the
// Env initializer list takes u64 draws. Each double draw is cast with static_cast<u64>,
// which truncates it to 0. This compiles, but it does not preserve the test intent
// (e.g. 0.555 becomes 0). Correct the Env parameter type in test_support.h to accept
// double draws, then restore the original literals.
#include "test_support.h"
#include "mordor/death1b.h"

#include <cmath>

using namespace mordor;

// FIXED: kGoldToCurrency is not declared in any header. Per the test comments, the
// returned Currency is in 1/10000 gold, so the scale is 10000.
static constexpr Currency kGoldToCurrency = 10000;

// R12 step 7 and R12 quirks: quiet-failure ages by cause.
TEST(raise_fail_age_by_state) {
    CHECK_EQ(raiseFailAgeDays(DeathState::Dead), 54);
    CHECK_EQ(raiseFailAgeDays(DeathState::Stoned), 84);
    CHECK_EQ(raiseFailAgeDays(DeathState::Rock), 124);
}

// R12 step 10 and CurHits rule: only kinds 22 and 23 restore full hits.
TEST(raised_hits_by_kind) {
    CHECK_EQ(raisedCurHits(RaiseKind::RaiseDead, 30), 1);
    CHECK_EQ(raisedCurHits(RaiseKind::RestoreFlesh, 30), 1);
    CHECK_EQ(raisedCurHits(RaiseKind::Resurrect, 30), 30);
    CHECK_EQ(raisedCurHits(RaiseKind::Morgue, 30), 30);
}

// R13: tier = max(1, effect - 19).
TEST(spell_raise_tier_from_effect) {
    CHECK_EQ(spellRaiseTier(RaiseKind::RaiseDead), 1);
    CHECK_EQ(spellRaiseTier(RaiseKind::RestoreFlesh), 2);
    CHECK_EQ(spellRaiseTier(RaiseKind::Resurrect), 3);
}

// R13: chance = clamp(round(35*CON/4.5 + tier*10), 10, 98).
TEST(spell_raise_chance_formula) {
    // CON 5: 38.89 + 10 = 48.89 -> 49; +20 -> 58.89 -> 59; +30 -> 68.89 -> 69
    CHECK_EQ(spellRaiseChance(5, RaiseKind::RaiseDead, false), 49);
    CHECK_EQ(spellRaiseChance(5, RaiseKind::RestoreFlesh, false), 59);
    CHECK_EQ(spellRaiseChance(5, RaiseKind::Resurrect, false), 69);
    // CON 4: 31.11 + 10 = 41.11 -> 41
    CHECK_EQ(spellRaiseChance(4, RaiseKind::RaiseDead, false), 41);
    // CON 0: 0 + tier 1 * 10 = 10
    CHECK_EQ(spellRaiseChance(0, RaiseKind::RaiseDead, false), 10);
}

// R13: upper clamp at 98; rock is always 10 on the spell path.
TEST(spell_raise_chance_clamp_and_rock) {
    // CON 13: 101.11 + 10 = 111.11 -> clamped to 98
    CHECK_EQ(spellRaiseChance(13, RaiseKind::RaiseDead, false), 98);
    CHECK_EQ(spellRaiseChance(13, RaiseKind::Resurrect, false), 98);
    CHECK_EQ(spellRaiseChance(13, RaiseKind::Resurrect, true), 10);
    CHECK_EQ(spellRaiseChance(5, RaiseKind::RaiseDead, true), 10);
}

// R16 step 5: companion morgue chance = min(98, 75 + CON).
TEST(companion_morgue_chance_capped_at_98) {
    CHECK_EQ(companionMorgueChance(10), 85);
    CHECK_EQ(companionMorgueChance(23), 98);
    CHECK_EQ(companionMorgueChance(24), 98);
}

// R16 step 5: cost = round_half_even(MaxHits * 1244.34 * Floor).
TEST(companion_morgue_cost_rounding) {
    // R16.5: the cost is gold; the returned Currency is 1/10000 gold.
    // 10 * 1244.34 * 1 = 12443.4 -> 12443 gold
    CHECK_EQ(companionMorgueCost(10, 1), 12443 * kGoldToCurrency);
    // 1 * 1244.34 * 2 = 2488.68 -> 2489 gold
    CHECK_EQ(companionMorgueCost(1, 2), 2489 * kGoldToCurrency);
}

// R15: counter uses round-half-even, own-body offer uses Int (floor).
// 2912.32 / 1.5 = 1941.547 -> counter 1942, own body 1941.
TEST(morgue_fee_rounding_counter_vs_own_body) {
    // R15: the counter path rounds half to even; the own-body path floors.
    // The returned Currency is 1/10000 gold, so expected values are scaled.
    CHECK_EQ(morgueFeeCounter(1), 1942 * kGoldToCurrency);
    CHECK_EQ(morgueFeeOwnBody(1), 1941 * kGoldToCurrency);
    // MaxHits 15: 2912.32 * 10 = 29123.2 -> 29123 on both paths
    CHECK_EQ(morgueFeeCounter(15), 29123 * kGoldToCurrency);
    CHECK_EQ(morgueFeeOwnBody(15), 29123 * kGoldToCurrency);
}

// R15: guildPays = TopGuildLevel <= 5 && Purse < cost (strict).
TEST(morgue_guild_pays_rule) {
    Character ch;
    ch.topGuildLevel = 5;
    ch.wallet.goldOnHand = 40;
    ch.wallet.goldInBank = 60;               // purse 100
    CHECK(morgueGuildPays(ch, 101));         // 100 < 101
    CHECK(!morgueGuildPays(ch, 100));        // equal is not less
    ch.topGuildLevel = 6;
    CHECK(!morgueGuildPays(ch, 101));        // level too high
}

// R12 step 5: fail if Int(Rnd*100) > roll; roll is 0 for rock.
TEST(raise_roll_fail_boundary) {
    // Scripted Rnd values are passed as doubles (no u64 cast, which truncated them to 0).
    // Int(0.555 * 100) = 55 for the draws below.
    { tsup::Env env({0.555}); CHECK(raiseRollFails(env.ctx, 50, false)); }   // 55 > 50
    { tsup::Env env({0.555}); CHECK(!raiseRollFails(env.ctx, 55, false)); }  // 55 == 55 succeeds
    { tsup::Env env({0.555}); CHECK(raiseRollFails(env.ctx, 98, true)); }    // rock: 55 > 0
    { tsup::Env env({0.005}); CHECK(!raiseRollFails(env.ctx, 98, true)); }   // rock: draw 0 is not > 0
}

// R12 step 6: complication only when the roll falls under the chance.
TEST(raise_complication_roll_threshold) {
    // Scripted Rnd values are passed as doubles (no u64 cast).
    { tsup::Env env({0.705}); CHECK(!rollRaiseComplication(env.ctx, 60)); }  // roll 70
    { tsup::Env env({0.105}); CHECK(rollRaiseComplication(env.ctx, 60)); }   // roll 10
}

// R12 step 8: stats -5 each, age +5475, MaxHits = int(MaxHits/1.2), CurHits = MaxHits.
TEST(raise_complication_effects) {
    tsup::Env env({static_cast<u64>(0.5)});
    Character ch;
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) {
        ch.abilities.stats[i] = 30;
    }
    ch.ageDays = 1000;
    ch.vitals.maxHits = 10;
    ch.vitals.curHits = 3;
    applyRaiseComplication(env.ctx, ch);
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) {
        CHECK_EQ(ch.abilities.stats[i], 25);
    }
    CHECK_EQ(ch.ageDays, 1000 + 5475);
    CHECK_EQ(ch.vitals.maxHits, 8);   // int(10 / 1.2) = int(8.33) = 8
    CHECK_EQ(ch.vitals.curHits, 8);
}

// R12 step 8: MaxHits floor of 1 after complication.
TEST(raise_complication_min_max_hits) {
    tsup::Env env({static_cast<u64>(0.5)});
    Character ch;
    ch.vitals.maxHits = 1;            // int(1 / 1.2) = 0, lifted to 1
    ch.vitals.curHits = 0;
    applyRaiseComplication(env.ctx, ch);
    CHECK_EQ(ch.vitals.maxHits, 1);
    CHECK_EQ(ch.vitals.curHits, 1);
}

// R12 step 9 and step 12: success clears death, sets hits per kind, touches no ailment.
TEST(raise_success_clears_death_and_sets_hits) {
    tsup::Env env({static_cast<u64>(0.5)});
    Character ch;
    ch.deathState = DeathState::Stoned;
    ch.raiseKind = RaiseKind::Resurrect;
    ch.vitals.maxHits = 20;
    ch.vitals.curHits = 0;
    ch.ailments.poisonLevel = 2;
    applyRaiseSuccess(env.ctx, ch);
    CHECK(!ch.isDead());
    CHECK(ch.deathState == DeathState::Alive);
    CHECK_EQ(ch.vitals.curHits, 20);
    CHECK_EQ(ch.ailments.poisonLevel, 2);

    Character rd;
    rd.deathState = DeathState::Dead;
    rd.raiseKind = RaiseKind::RaiseDead;
    rd.vitals.maxHits = 20;
    applyRaiseSuccess(env.ctx, rd);
    CHECK(!rd.isDead());
    CHECK_EQ(rd.vitals.curHits, 1);
}

// R12 step 9 and quirk: stone costs exactly 1 CON.
TEST(stone_con_loss_is_one_point) {
    Character ch;
    ch.abilities.stats[static_cast<u32>(StatIndex::Con)] = 20;
    ch.abilities.stats[static_cast<u32>(StatIndex::Str)] = 20;
    applyStoneConLoss(ch);
    CHECK_EQ(ch.abilities.stats[static_cast<u32>(StatIndex::Con)], 19);
    CHECK_EQ(ch.abilities.stats[static_cast<u32>(StatIndex::Str)], 20);
}

// R17 steps 2-3: level above 0 but off the map stores bodies at (level, x, y); cause rock; area 0.
TEST(teleport_rock_off_map_stores_bodies) {
    tsup::Env env({static_cast<u64>(0.5)});
    Party party;
    party.size = 2;
    Vec<Companion> comps;
    Vec<FloorState> levels;          // empty map: level 5 is off the map
    OffMapStore off;
    TeleportRockResult r = teleportIntoRock(env.ctx, party, comps, levels, off, 5, 3, 4);
    CHECK(r.outcome == TeleportRockOutcome::BodiesOffMap);
    CHECK_EQ(r.area, 0);
    CHECK_EQ(off.level, 5);
    CHECK_EQ(off.x, 3);
    CHECK_EQ(off.y, 4);
    CHECK_EQ(off.bodies.size(), 2);
    CHECK(party.members[0].deathState == DeathState::Rock);
    CHECK(party.members[1].deathState == DeathState::Rock);
}

// R17 step 5: falling out of the sky kills every member with an ordinary death.
TEST(fell_out_of_the_sky_kills_all) {
    tsup::Env env({static_cast<u64>(0.5)});
    Party party;
    party.size = 2;
    Vec<Companion> comps;
    fellOutOfTheSky(env.ctx, party, comps);
    CHECK(party.members[0].deathState == DeathState::Dead);
    CHECK(party.members[1].deathState == DeathState::Dead);
    CHECK(party.members[0].isDead());
}

// R18: leader under 1 hit. Opener in slot 0 dies; other slots set no one dead; standing leader is untouched.
TEST(chest_opener_trap_death_rules) {
    tsup::Env env({static_cast<u64>(0.5)});
    Vec<Companion> comps;
    SquareState chest;
    chest.chestTrap = ChestTrap::NotTrapped;
    chest.chestGold = 500;

    Party p1;
    p1.size = 2;
    p1.members[0].vitals.curHits = 0;
    p1.members[1].vitals.curHits = 5;
    OpenerTrapOutcome o = chestOpenerDeathCheck(env.ctx, p1, comps, chest, 0);
    CHECK(o == OpenerTrapOutcome::OpenerKilled);
    CHECK(p1.members[0].isDead());
    CHECK(!p1.members[1].isDead());
    CHECK(chest.chestTrap == ChestTrap::NotTrapped);
    CHECK_EQ(chest.chestGold, 500);

    Party p2;
    p2.size = 2;
    p2.members[0].vitals.curHits = 0;
    o = chestOpenerDeathCheck(env.ctx, p2, comps, chest, 1);
    CHECK(o == OpenerTrapOutcome::NobodyKilled);
    CHECK(!p2.members[0].isDead());
    CHECK(!p2.members[1].isDead());

    Party p3;
    p3.size = 2;
    p3.members[0].vitals.curHits = 3;
    o = chestOpenerDeathCheck(env.ctx, p3, comps, chest, 0);
    CHECK(o == OpenerTrapOutcome::LeaderStanding);
    CHECK(!p3.members[0].isDead());
}

// R19: pct = (ageDays / 365) / lifespan * 100.
TEST(old_age_percent_of_lifespan) {
    CHECK(std::fabs(oldAgePercent(18250, 100) - 50.0) < 1e-9);    // 50 years of 100
    CHECK(std::fabs(oldAgePercent(36500, 100) - 100.0) < 1e-9);   // 100 years of 100
}

// R19: no death below 98%; at pct 99, Ln(1) = 0, so threshold is 5 (roll < 5).
TEST(old_age_roll_threshold) {
    // Scripted Rnd values are passed as doubles (no u64 cast).
    { tsup::Env env({0.0}); CHECK(!rollOldAgeDeath(env.ctx, 97.0)); }
    { tsup::Env env({0.045}); CHECK(rollOldAgeDeath(env.ctx, 99.0)); }   // roll 4 < 5
    { tsup::Env env({0.055}); CHECK(!rollOldAgeDeath(env.ctx, 99.0)); }  // roll 5 is not < 5
}

// R21 step 1: recordable only if group identified; invisible monster needs killer to see invisible.
TEST(kill_recordable_rules) {
    Character killer;
    CHECK(!isKillRecordable(false, false, killer));
    CHECK(isKillRecordable(true, false, killer));
    CHECK(!isKillRecordable(true, true, killer));
    killer.ailments.seeInvisible = true;
    CHECK(isKillRecordable(true, true, killer));
}

// R21 step 2: ChestFindFrom = lead.Id when indexes match and lead visible or taker sees invisible; else -2.
TEST(chest_find_credit_rules) {
    Character taker;
    CHECK_EQ(chestFindCredit(2, 2, 77, false, taker), 77);
    CHECK_EQ(chestFindCredit(1, 2, 77, false, taker), -2);
    CHECK_EQ(chestFindCredit(2, 2, 77, true, taker), -2);
    taker.ailments.seeInvisible = true;
    CHECK_EQ(chestFindCredit(2, 2, 77, true, taker), 77);
}

// R16 step 2: candidates are dead, pending-free, on this level and area, listed by slot.
TEST(companion_raise_candidates_filter) {
    Party party;
    party.size = 1;
    party.dungeonLevel = 4;
    Vec<Companion> comps;
    for (i32 i = 0; i < 4; ++i) {
        Companion c;
        c.occupied = true;
        c.slot = i;
        c.area = 2;
        c.dungeonLevel = 4;
        c.hits = 0;
        c.isDead = true;
        c.deathState = CompanionDeathState::Dead;
        comps.push_back(c);
    }
    comps[1].isDead = false;                       // alive: not a candidate
    comps[1].deathState = CompanionDeathState::Normal;
    comps[1].hits = 5;
    comps[2].pendingRaise = true;                  // already pending: excluded
    comps[3].area = 7;                             // wrong area: excluded
    Vec<i32> out = companionRaiseCandidates(party, comps, kCarryNone, 2);
    CHECK_EQ(out.size(), 1);
    CHECK_EQ(out[0], 0);
}

// R23: Retrieve Soul needs dead-in-rock, same level, area 0; typed errors otherwise.
TEST(retrieve_soul_status_rules) {
    Character ch;
    ch.deathState = DeathState::Rock;
    ch.location.dungeonLevel = 3;
    ch.location.area = 0;
    CHECK(retrieveSoulStatus(ch, 3) == SoulCheck::Ok);
    ch.location.dungeonLevel = 2;
    CHECK(retrieveSoulStatus(ch, 3) == SoulCheck::NotOnThisLevel);
    ch.location.dungeonLevel = 3;
    ch.deathState = DeathState::Dead;
    CHECK(retrieveSoulStatus(ch, 3) == SoulCheck::NotInRock);
}

// Morgue input: Val skips leading spaces, rounds, stops at first non-numeric character.
TEST(val_like_original_parsing) {
    CHECK_EQ(valLikeOriginal(Str("3.7")), 4);
    CHECK_EQ(valLikeOriginal(Str("1-2")), 1);
    CHECK_EQ(valLikeOriginal(Str("x")), 0);
}
