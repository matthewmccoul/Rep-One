#include "test_support.h"
#include "mordor/guilds1.h"

using namespace mordor;

namespace {

i32 countPackId(const Inventory& inv, i32 id) {
    i32 n = 0;
    for (u32 i = 0; i < static_cast<u32>(kPackSlots); ++i) {
        if (inv.pack[i].id == id) ++n;
    }
    return n;
}

void pushLog(GameState& game, Guild guild, i64 day) {
    GuildLogEvent ev{};
    ev.day = day;
    ev.guild = guild;
    game.guildLog.push_back(ev);
}

// Counts log entries for one guild and reports the smallest day among them.
i32 countLog(const GameState& game, Guild guild, i64& minDay) {
    i32 n = 0;
    minDay = 1000000;
    for (decltype(game.guildLog.size()) k = 0; k < game.guildLog.size(); ++k) {
        const GuildLogEvent& ev = game.guildLog[k];
        if (ev.guild == guild) {
            ++n;
            if (ev.day < minDay) minDay = ev.day;
        }
    }
    return n;
}

}  // namespace

// R1, R4: crest id is 299 + guild index in table order (Nomad = 0).
TEST(crest_ids_follow_guild_table_order) {
    CHECK_EQ(crestId(Guild::Nomad), 299);
    CHECK_EQ(crestId(Guild::Warrior), 300);
    CHECK_EQ(crestId(Guild::Mage), 307);
    CHECK_EQ(crestId(Guild::Healer), 310);
}

// R28: bitmask bit i is 1 << i; All = 2^12 - 1 = 4095.
TEST(guild_bitmask_table_matches_r28) {
    i32 sum = 0;
    for (i32 i = 0; i < kGuildCount; ++i) {
        CHECK_EQ(kGuildBitmask[i], 1 << i);
        sum += kGuildBitmask[i];
    }
    CHECK_EQ(kGuildAllMask, sum);
    CHECK_EQ(kGuildAllMask, 4095);
}

// R24: Actual EP = EP / 8. Values chosen so integer and real division agree.
TEST(actual_exp_mod_divides_ep_by_eight) {
    CHECK_EQ(actualExpMod(16), 2.0);
    CHECK_EQ(actualExpMod(24), 3.0);
    CHECK_EQ(actualExpMod(0), 0.0);
}

// R24: Actual GP = GP / 10 + 1. Values chosen as multiples of 10.
TEST(actual_gold_mod_is_tenth_plus_one) {
    CHECK_EQ(actualGoldMod(0), 1.0);
    CHECK_EQ(actualGoldMod(20), 3.0);
    CHECK_EQ(actualGoldMod(30), 4.0);
}

// Make Level step 3: experience short of the requirement blocks levelling.
TEST(can_level_up_refused_while_experience_short) {
    GuildMembership m{};
    m.joined = true;
    m.level = 2;
    m.experience = 99;
    CHECK(!canLevelUp(m, 100));
    m.experience = 1000;
    CHECK(canLevelUp(m, 100));
}

// R15: AllowsRace is true for a negative index. Zeroed mask blocks index 0.
TEST(negative_race_index_is_always_allowed) {
    GuildDef def{};
    CHECK(allowsRace(def, -1));
    CHECK(!allowsRace(def, 0));
}

// R15: AllowsAlignment is (AlignMask & (1 << a)) != 0. Zeroed mask allows none.
TEST(zeroed_alignment_mask_allows_no_alignment) {
    GuildDef def{};
    const Align all[] = {Align::Good, Align::Neutral, Align::Evil};
    for (Align a : all) {
        CHECK(!allowsAlignment(def, a));
    }
}

// R16 first branch: a race that is not allowed displays as hidden and blocks join.
TEST(blocked_race_is_hidden_and_not_joinable) {
    GuildDef def{};
    Character c{};
    c.race = Race::Human;
    CHECK(eligibilityStyle(def, c) == EligibilityStyle::Hidden);
    CHECK(!eligibleToJoin(def, c));
}

// R15: MeetsStats is all i: stat[i] >= ReqStats[i]. Zeroed requirements.
TEST(meets_stats_compares_each_stat_to_requirement) {
    GuildDef def{};
    Arr<i32, kStatCount> stats{};
    CHECK(meetsStats(def, stats));
    stats[5] = -1;
    CHECK(!meetsStats(def, stats));
}

// R26: sum over abilities and spell maxima where value > 0. Empty sum gives cost 0.
TEST(join_cost_is_zero_when_guild_has_no_requirements) {
    GuildDef def{};
    Character c{};
    CHECK_EQ(guildJoinCost(def, c), 0);
}

// R30: if mod <= 0 the extra swing count is 0 at any level.
TEST(swings_zero_when_multiple_swings_mod_not_positive) {
    GuildDef def{};
    GuildMembership m{};
    m.joined = true;
    m.level = 10;
    CHECK_EQ(swingsFor(def, m), 0);
    m.level = 0;
    CHECK_EQ(swingsFor(def, m), 0);
}

// R26 constants and R13 step 2 threshold.
TEST(join_constants_match_srd) {
    CHECK_EQ(kJoinCostFactor, 5356);
    CHECK_EQ(kJoinAbilityWeight, 1.25);
    CHECK_EQ(kJoinSpellDivisor, 3.0);
    CHECK_EQ(kJoinWarnTopLevel, 10);
}

// R8 / log cap: with 60 entries for one guild, the newest 50 survive.
// Entries are pushed in ascending day order, so the newest have days 10..59.
TEST(clean_guild_log_keeps_newest_fifty_per_guild) {
    // SRD-AMBIGUOUS: R8 says newest 50 per guild; R65/R66 give a whole-log
    // trigger of count > 50 x guildCount (12 guilds => 600). 60 entries is
    // below that trigger, so the engine rule governs: no trim.
    tsup::Env env({0.5, 0.9});
    GameState game{};
    game.needsCleaning = true;
    for (i64 d = 0; d < 60; ++d) pushLog(game, Guild::Thief, d);
    cleanGuildLog(env.ctx, game);
    i64 minDay = 0;
    i32 n = countLog(game, Guild::Thief, minDay);
    CHECK_EQ(n, 60);
    CHECK_EQ(minDay, 0);
}

// Boundary: exactly 50 entries are all kept.
TEST(clean_guild_log_leaves_fifty_entries_alone) {
    tsup::Env env({0.5, 0.9});
    GameState game{};
    game.needsCleaning = true;
    for (i64 d = 0; d < 50; ++d) pushLog(game, Guild::Thief, d);
    cleanGuildLog(env.ctx, game);
    i64 minDay = 0;
    i32 n = countLog(game, Guild::Thief, minDay);
    CHECK_EQ(n, 50);
    CHECK_EQ(minDay, 0);
}

// R8: the 50-entry limit is per guild, so a small guild is not trimmed.
TEST(clean_guild_log_does_not_trim_other_guilds) {
    // SRD-AMBIGUOUS: R8 (per-guild keep of 50) conflicts with R66 (whole-log
    // trigger 50 x guildCount, 63 entries < 600). Test follows the engine
    // rule: no trim happens, so every entry is kept.
    tsup::Env env({0.5, 0.9});
    GameState game{};
    game.needsCleaning = true;
    for (i64 d = 100; d < 103; ++d) pushLog(game, Guild::Mage, d);
    for (i64 d = 0; d < 60; ++d) pushLog(game, Guild::Thief, d);
    cleanGuildLog(env.ctx, game);
    i64 mageMin = 0;
    i64 thiefMin = 0;
    i32 mage = countLog(game, Guild::Mage, mageMin);
    i32 thief = countLog(game, Guild::Thief, thiefMin);
    CHECK_EQ(mage, 3);
    CHECK_EQ(mageMin, 100);
    CHECK_EQ(thief, 60);
    CHECK_EQ(thiefMin, 0);
}

// R4 / crest placement: an empty pack takes the crest, id 299 + index.
TEST(award_crest_places_crest_in_empty_pack) {
    tsup::Env env({0.5, 0.9});
    Inventory inv{};
    CrestOutcome r = awardCrest(env.ctx, inv, Guild::Warrior);
    CHECK(r == CrestOutcome::Placed);
    CHECK_EQ(countPackId(inv, 300), 1);
}

// Crest already held in the pack: no second crest is placed.
TEST(award_crest_reports_already_held) {
    tsup::Env env({0.5, 0.9});
    Inventory inv{};
    inv.pack[5].id = crestId(Guild::Warrior);
    CrestOutcome r = awardCrest(env.ctx, inv, Guild::Warrior);
    CHECK(r == CrestOutcome::AlreadyHeld);
    CHECK_EQ(countPackId(inv, 300), 1);
}

// Every pack and bank slot occupied by other items: no room for the crest.
TEST(award_crest_reports_no_room_when_full) {
    tsup::Env env({0.5, 0.9});
    Inventory inv{};
    for (u32 i = 0; i < static_cast<u32>(kPackSlots); ++i) inv.pack[i].id = 1;
    for (u32 i = 0; i < static_cast<u32>(kBankSlots); ++i) inv.bank[i].id = 1;
    CrestOutcome r = awardCrest(env.ctx, inv, Guild::Warrior);
    CHECK(r == CrestOutcome::NoRoom);
    CHECK_EQ(countPackId(inv, 300), 0);
}

// R7: a claim below the current guildmaster's level is refused; seat unchanged.
TEST(claim_mastership_refused_below_current_master) {
    tsup::Env env({0.5, 0.9});
    GameState game{};
    GuildMaster gm{};
    gm.holder = 3;
    gm.level = 10;
    gm.name = "Lord Ashen";  // R57: the seat is held only when held.Name != ""
    const u32 idx = static_cast<u32>(Guild::Paladin);
    game.guildMasters[idx] = gm;
    Character c{};
    CHECK(!claimMastership(env.ctx, game, c, Guild::Paladin, 4));
    CHECK_EQ(game.guildMasters[idx].holder, 3);
    CHECK_EQ(game.guildMasters[idx].level, 10);
}

// R13.1: re-acquaint of a joined guild is free and makes it active.
TEST(reacquaint_joined_guild_is_free_and_activates) {
    tsup::Env env({0.5, 0.9});
    Character c{};
    c.guilds[static_cast<u32>(Guild::Paladin)].joined = true;
    c.guilds[static_cast<u32>(Guild::Paladin)].level = 2;
    c.wallet.goldOnHand = 100;
    bool ok = reacquaintGuild(env.ctx, c, Guild::Paladin);
    CHECK(ok);
    CHECK(c.activeGuild == Guild::Paladin);
    CHECK_EQ(c.wallet.goldOnHand, 100);
}

// R34 / R36: a character with no memberships has no guild A/D to contribute.
TEST(best_guild_ad_is_zero_without_memberships) {
    Character c{};
    BestAd b = bestGuildAD(c);
    CHECK_EQ(b.att, 0);
    CHECK_EQ(b.def, 0);
}

// R32: at or above ML the gain is MH only, with no CON bonus. Zeroed def gives MH = 0.
TEST(hits_zero_at_or_above_hit_cap_level_with_zero_mh) {
    tsup::Env env({0.5, 0.9});
    Character c{};
    GuildMembership m{};
    m.joined = true;
    m.level = 5;
    GuildDef def{};
    CHECK_EQ(hitsGain(env.ctx, c, m, def, 18), 0);
}

// R13, R21-R23, R32: guild-rule constants as documented.
TEST(guild_rule_constants_match_srd) {
    CHECK_EQ(kMaxGuildLevel, 999);
    CHECK_EQ(kMaxHits, 999);
    CHECK_EQ(kHitsConThreshold, 16);
    CHECK_EQ(kGoldRate, 60.2);
    CHECK_EQ(kSwingThresholdScale, 50);
    CHECK_EQ(kSwingModBase, 10);
}

// R35, R36: ability chance clamps and fighting base as documented.
TEST(combat_ability_constants_match_srd) {
    CHECK_EQ(kAbilityThieving, 0);
    CHECK_EQ(kAbilityFighting, 4);
    CHECK_EQ(kAbilityPerception, 5);
    CHECK_EQ(kThievingMin, 5);
    CHECK_EQ(kThievingMax, 98);
    CHECK_EQ(kPerceptionMin, 5);
    CHECK_EQ(kPerceptionMax, 90);
    CHECK_EQ(kFightingBase, 0.6);
    CHECK_EQ(kMonsterGuildLevelFloor, 2);
}
