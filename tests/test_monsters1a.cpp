#include <cmath>

#include "test_support.h"
#include "mordor/monsters1a.h"

using namespace mordor;

namespace {
template <typename E>
int enumCode(E e) {
    return static_cast<int>(e);
}
}  // namespace

// R3: Asmodeus is identified by Id alone.
TEST(asmodeus_identified_by_id_alone) {
    CHECK(isAsmodeus(277));
    CHECK(!isAsmodeus(278));
    CHECK(!isAsmodeus(0));
    CHECK_EQ(kAsmodeusId, 277);
}

// R8: rock square 1432 and area 0 block; positive area with stored encounter uses it.
TEST(encounter_precheck_blocks_rock_and_area_zero) {
    CHECK_EQ(kRockSquareId, 1432);
    CHECK_EQ(enumCode(encounterPrecheck(kRockSquareId, 5, true, 1)),
             enumCode(EncounterPrecheck::Blocked));
    CHECK_EQ(enumCode(encounterPrecheck(100, kNoEncounterAreaId, false, 1)),
             enumCode(EncounterPrecheck::Blocked));
    CHECK_EQ(enumCode(encounterPrecheck(100, 5, true, 1)),
             enumCode(EncounterPrecheck::UseStored));
    CHECK_EQ(enumCode(encounterPrecheck(100, 5, false, 1)),
             enumCode(EncounterPrecheck::Roll));
}

// R10: newbie means viewer level 1 and area id below 18.
TEST(newbie_boundary_is_level_one_and_area_below_18) {
    CHECK_EQ(kNewbieLevel, 1);
    CHECK_EQ(kNewbieMaxAreaId, 18);
    CHECK(isNewbie(1, 17));
    CHECK(!isNewbie(1, 18));
    CHECK(!isNewbie(2, 5));
}

// R12: m0282 = meLevel, plus 1 on a Stud square.
TEST(target_floor_gets_plus_one_on_stud) {
    CHECK_EQ(encounterTargetFloor(5, false), 5);
    CHECK_EQ(encounterTargetFloor(5, true), 6);
    CHECK_EQ(encounterTargetFloor(1, false), 1);
}

// R12: eligible when LevelFound is in [target - spread, target].
TEST(eligibility_window_inclusive_bounds) {
    CHECK(inEligibilityWindow(5, 7, 2));
    CHECK(inEligibilityWindow(7, 7, 2));
    CHECK(!inEligibilityWindow(4, 7, 2));
    CHECK(!inEligibilityWindow(8, 7, 2));
    CHECK(inEligibilityWindow(7, 7, 0));
    CHECK(!inEligibilityWindow(6, 7, 0));
}

// R13: spread widens by 1; restart when spread goes past 3.
TEST(spread_widens_and_restarts_past_three) {
    CHECK_EQ(kInitialSpread, 0);
    CHECK_EQ(kSpreadRestartLimit, 3);
    i32 s = kInitialSpread;
    for (int k = 0; k < 3; ++k) {
        s = widenSpread(s);
        CHECK(!spreadExhausted(s));
    }
    CHECK_EQ(s, 3);
    s = widenSpread(s);
    CHECK_EQ(s, 4);
    CHECK(spreadExhausted(s));
}

// R9: lair roll only when LairId > 0 and RndF()*100 <= 75; missing lead means Failed.
TEST(lair_roll_gated_at_75_percent_and_missing_lead_fails) {
    Area area;
    area.lairId = 0;
    {
        tsup::Env env({0.0, 0.0});
        LairRoll r = rollLair(env.ctx, area);
        CHECK_EQ(enumCode(r.outcome), enumCode(LairOutcome::NotTaken));
    }
    area.lairId = 999999;
    {
        // 90 > 75: lair not taken.
        tsup::Env env({0.9, 0.9});
        LairRoll r = rollLair(env.ctx, area);
        CHECK_EQ(enumCode(r.outcome), enumCode(LairOutcome::NotTaken));
    }
    {
        // 75 <= 75: lair taken; no monster has Id 999999, so the encounter fails.
        tsup::Env env({0.75, 0.75});
        LairRoll r = rollLair(env.ctx, area);
        CHECK_EQ(enumCode(r.outcome), enumCode(LairOutcome::Failed));
    }
    CHECK_EQ(kLairChancePercent, 75);
}

// R16 worked case: groups = 4, threshold 4/8*100 = 50.
TEST(group_count_reduction_threshold_and_draw) {
    {
        // 20 < 50 reduces by Int(0.7*(4/2+1)) = Int(2.1) = 2 -> 2 groups.
        tsup::Env env({0.2, 0.7});
        CHECK_EQ(rollGroupCount(env.ctx, 4), 2);
    }
    {
        // 49 < 50 reduces; same reduction -> 2 groups.
        tsup::Env env({0.49, 0.7});
        CHECK_EQ(rollGroupCount(env.ctx, 4), 2);
    }
    {
        // 50 < 50 is false: no reduction -> 4 groups.
        tsup::Env env({0.5, 0.7});
        CHECK_EQ(rollGroupCount(env.ctx, 4), 4);
    }
}

// R16: clamp to 1..4. NumGroups 1 with a reduction gives 0, clamped to 1.
TEST(group_count_clamped_to_at_least_one) {
    tsup::Env env({0.05, 0.9});
    CHECK_EQ(rollGroupCount(env.ctx, 1), 1);
}

// R17: GroupCount = InGroup - CInt(Rnd*(InGroup/2.0)); CInt is round-half-even.
TEST(group_size_uses_round_half_even) {
    {
        // 0.75*2.0 = 1.5 -> 2 (even) -> 4-2 = 2.
        tsup::Env env({0.75, 0.0});
        CHECK_EQ(rollGroupSize(env.ctx, 4), 2);
    }
    {
        // 0.25*2.0 = 0.5 -> 0 (even) -> 4.
        tsup::Env env({0.25, 0.0});
        CHECK_EQ(rollGroupSize(env.ctx, 4), 4);
    }
    {
        // 0.5*2.0 = 1.0 -> 1 -> 3.
        tsup::Env env({0.5, 0.0});
        CHECK_EQ(rollGroupSize(env.ctx, 4), 3);
    }
    {
        // 0.7*2.5 = 1.75 -> 2 -> 5-2 = 3.
        tsup::Env env({0.7, 0.0});
        CHECK_EQ(rollGroupSize(env.ctx, 5), 3);
    }
}

// R18: Good lead with Good viewer is peaceful; Neutral lead is peaceful below 40.
TEST(lead_engagement_alignment_rules_and_forty_percent_peace) {
    {
        tsup::Env env({0.0, 0.0});
        CHECK(!rollLeadEngaged(env.ctx, 0, Align::Good));
    }
    {
        tsup::Env env({0.0, 0.0});
        CHECK(rollLeadEngaged(env.ctx, 0, Align::Neutral));
    }
    {
        // 39 < 40 -> peaceful.
        tsup::Env env({0.39, 0.39});
        CHECK(!rollLeadEngaged(env.ctx, 1, Align::Evil));
    }
    {
        // 41 >= 40 -> engaged.
        tsup::Env env({0.41, 0.41});
        CHECK(rollLeadEngaged(env.ctx, 1, Align::Neutral));
    }
    CHECK_EQ(kNeutralPeacePercent, 40);
}

// R15: lead index 4 is exempt from the newbie guard; a non-newbie is never rejected.
TEST(newbie_lead_guard_exemptions) {
    tsup::Env env({0.5, 0.9});
    CHECK(!newbieLeadRejected(env.ctx, 4, 1, 5));
    CHECK(!newbieLeadRejected(env.ctx, 0, 5, 5));
}

// R4: Id 307 resolves to Morgul (first); an Id with no creature falls back to index 0.
TEST(shared_id_307_resolves_to_morgul_and_unknown_id_to_zero) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(monsterTableIndexForId(env.ctx, 307), kMorgulTableIndex);
    CHECK_EQ(monsterTableIndexForId(env.ctx, 999999), 0);
    CHECK_EQ(kMorgulTableIndex, 278);
    CHECK_EQ(kShadowNinjaTableIndex, 389);
}

// R10: shipped class count is 18.
TEST(shipped_monster_class_count_is_eighteen) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(monsterClassCount(env.ctx), 18);
}

// R28 worked cases, plus the audit swing bound (Int(Hits*0.5/2)+1).
TEST(monster_hits_roll_sign_and_swing_and_audit_bound) {
    {
        // sign: Int(0.2*2)=0 subtract. swing: Int(0.5*(20*0.5)/2)+1 = Int(2.5)+1 = 3. 20-3 = 17.
        tsup::Env env({0.2, 0.5});
        RolledHits r = rollMonsterHits(env.ctx, 20);
        CHECK_EQ(r.hits, 17);
        CHECK_EQ(r.maxHits, 17);
    }
    {
        // sign: Int(0.7*2)=1 add. swing: Int(0.0)+1 = 1. 20+1 = 21.
        tsup::Env env({0.7, 0.0});
        RolledHits r = rollMonsterHits(env.ctx, 20);
        CHECK_EQ(r.hits, 21);
        CHECK_EQ(r.maxHits, 21);
    }
    CHECK_EQ(auditSwingBound(20), 6);
    CHECK_EQ(auditSwingBound(10), 3);
    CHECK_EQ(auditSwingBound(1), 1);
}

// R28 quirk: no floor. A 1-hit creature comes out with 0 or 2.
TEST(one_hit_creature_gives_zero_or_two) {
    {
        tsup::Env env({0.2, 0.9});
        RolledHits r = rollMonsterHits(env.ctx, 1);
        CHECK_EQ(r.hits, 0);
        CHECK_EQ(r.maxHits, 0);
    }
    {
        tsup::Env env({0.7, 0.9});
        RolledHits r = rollMonsterHits(env.ctx, 1);
        CHECK_EQ(r.hits, 2);
        CHECK_EQ(r.maxHits, 2);
    }
}

// R29: alive = !Gone && (Hits > 0 || AtNought); any write to Hits clears AtNought.
TEST(standing_at_nought_and_write_clears_flag) {
    Creature c;
    c.hits = 0;
    c.atNought = true;
    c.gone = false;
    CHECK(creatureAlive(c));

    c.atNought = false;
    CHECK(!creatureAlive(c));

    c.hits = 3;
    c.gone = true;
    CHECK(!creatureAlive(c));

    c.gone = false;
    CHECK(creatureAlive(c));

    c.hits = 0;
    c.atNought = true;
    setCreatureHits(c, 4);
    CHECK_EQ(c.hits, 4);
    CHECK(!c.atNought);
    CHECK(creatureAlive(c));
}

// R23: unrolled members get max(1, base Hits).
TEST(unrolled_member_hits_floor_of_one) {
    CHECK_EQ(unrolledMemberHits(0), 1);
    CHECK_EQ(unrolledMemberHits(1), 1);
    CHECK_EQ(unrolledMemberHits(7), 7);
}

// R30: damMod = DamageMod if > 0 else 1; +0.75 chest; +2 crit; +1 backstab; +/- sizeDiff/10.
TEST(melee_damage_modifier_rules) {
    CHECK_EQ(meleeDamageModifier(2.0, false, false, false, 0, false), 2.0);
    CHECK_EQ(meleeDamageModifier(0.0, false, false, false, 0, false), 1.0);
    CHECK_EQ(meleeDamageModifier(-3.0, false, false, false, 0, false), 1.0);
    CHECK_EQ(meleeDamageModifier(2.0, true, false, false, 0, false), 2.75);
    CHECK_EQ(meleeDamageModifier(2.0, false, true, false, 0, false), 4.0);
    CHECK_EQ(meleeDamageModifier(2.0, false, false, true, 0, false), 3.0);
    CHECK_EQ(meleeDamageModifier(2.0, false, true, true, 0, false), 5.0);
    CHECK_EQ(meleeDamageModifier(2.0, false, false, false, 20, false), 4.0);
    CHECK_EQ(meleeDamageModifier(2.0, false, false, false, -10, false), 1.0);
}

// R31: threshold = clamp(100*ln(1+M/k)*ln(floor+1) - 100*ln(1+D/999), lo, hi).
TEST(proc_threshold_clamps_and_matches_formula) {
    // All zero: raw = 0, clamped up to lo.
    CHECK_EQ(procThreshold(kCritProcSpec, 0, 0, 0), 2.0);
    CHECK_EQ(procThreshold(kBackstabProcSpec, 0, 0, 0), 4.0);
    // Very large inputs: clamped down to hi.
    CHECK_EQ(procThreshold(kCritProcSpec, 10000, 0, 10000), 50.0);

    // Mid-range case, computed by hand-written formula: M=100, D=50, floor=10.
    const double raw = 100.0 * std::log(1.0 + 100.0 / 650.0) * std::log(11.0) -
                       100.0 * std::log(1.0 + 50.0 / 999.0);
    CHECK(raw > 2.0 && raw < 50.0);
    CHECK(std::fabs(procThreshold(kCritProcSpec, 100, 50, 10) - raw) < 1e-9);
}

// R31: proc if the 1..100 draw <= threshold; draw = Int(Rnd*100)+1.
TEST(roll_proc_draw_boundary_is_inclusive) {
    {
        // Int(29.5)+1 = 30 <= 30 -> proc.
        tsup::Env env({0.295, 0.295});
        CHECK(rollProc(env.ctx, 30.0));
    }
    {
        // Int(30.5)+1 = 31 > 30 -> no proc.
        tsup::Env env({0.305, 0.305});
        CHECK(!rollProc(env.ctx, 30.0));
    }
    {
        // Draw 1 > threshold 0 -> never procs.
        tsup::Env env({0.0, 0.0});
        CHECK(!rollProc(env.ctx, 0.0));
    }
    {
        // Draw 100 <= threshold 100 -> always procs.
        tsup::Env env({0.999, 0.999});
        CHECK(rollProc(env.ctx, 100.0));
    }
}

// R35: chance 50; 80 if lead > comp + comp/4; 20 if lead < comp - comp/4. comp = 8, so bounds 10 and 6.
// Fires if Floor(rnd*100) < chance.
TEST(companion_breath_chance_and_roll_boundaries) {
    CHECK_EQ(companionBreathChance(11, 8), 80);
    CHECK_EQ(companionBreathChance(10, 8), 50);
    CHECK_EQ(companionBreathChance(6, 8), 50);
    CHECK_EQ(companionBreathChance(5, 8), 20);
    {
        // chance 80: Floor(80.1) = 80 is not < 80.
        tsup::Env env({0.801, 0.801});
        CHECK(!rollCompanionBreath(env.ctx, 11, 8));
    }
    {
        // chance 80: Floor(79.9) = 79 < 80.
        tsup::Env env({0.799, 0.799});
        CHECK(rollCompanionBreath(env.ctx, 11, 8));
    }
    {
        // chance 50: Floor(50.0) = 50 is not < 50.
        tsup::Env env({0.5, 0.5});
        CHECK(!rollCompanionBreath(env.ctx, 10, 8));
    }
    {
        // chance 20: Floor(15) = 15 < 20.
        tsup::Env env({0.15, 0.15});
        CHECK(rollCompanionBreath(env.ctx, 5, 8));
    }
}

// R27.5 / R31: companion melee defence is +4 crit, +1 backstab.
// R36: breath and acid remove only below 0; exactly 0 is kept.
TEST(companion_defence_bonus_and_breath_acid_below_zero) {
    CHECK_EQ(companionDefenceBonus(false, false), 0);
    CHECK_EQ(companionDefenceBonus(true, false), 4);
    CHECK_EQ(companionDefenceBonus(false, true), 1);
    CHECK_EQ(companionDefenceBonus(true, true), 5);
    CHECK(breathAcidRemoves(-1));
    CHECK(!breathAcidRemoves(0));
    CHECK(!breathAcidRemoves(1));
}

// R20 / R19: fogged groups (bit 20) start at identify level 1; others at 0.
TEST(initial_identify_level_follows_fog_bit) {
    CHECK_EQ(kFogFlagBit, 1u << 20);
    CHECK_EQ(enumCode(initialIdentifyLevel(0u)), enumCode(IdentifyLevel::OWN_NAME));
    CHECK_EQ(enumCode(initialIdentifyLevel(1u << 19)), enumCode(IdentifyLevel::OWN_NAME));
    CHECK_EQ(enumCode(initialIdentifyLevel(kFogFlagBit)), enumCode(IdentifyLevel::CLASS_NAME));
    CHECK_EQ(enumCode(initialIdentifyLevel(kFogFlagBit | 0x1u)),
             enumCode(IdentifyLevel::CLASS_NAME));
}

// R25: a group with a live join offer does not roll for engagement.
TEST(join_offer_pauses_engagement_roll) {
    MonsterGroup g;
    g.joinOffer = true;
    CHECK(!groupRollsForEngagement(g));
}
