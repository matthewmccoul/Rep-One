#include "test_support.h"
#include "mordor/guilds3.h"

using namespace mordor;

namespace {

// Guild membership slot for a guild id, indexed the same way the header does.
GuildMembership& gm_slot(Character& c, Guild g) {
    return c.guilds[static_cast<u32>(g)];
}

}  // namespace

// SRD c04p3 documents these values directly.
TEST(documented_constants_match_srd) {
    CHECK_EQ(kQuestTierCap, 10);
    CHECK_EQ(kLetOffMaxRoll, 5);
    CHECK_EQ(kRareMonsterMaxWeight, 9);
    CHECK_EQ(kRareItemMaxWeight, 4);
    CHECK_EQ(kItemFetchMinGuildLevel, 15);
    CHECK_EQ(kNinjaSwingThreshold, 56);
    CHECK_EQ(kWarriorSwingThreshold, 91);
    CHECK_EQ(kConHitsThreshold, 16);
    CHECK_EQ(kPreMlHitsDivisor, 5);
    CHECK_EQ(kNinjaGuild, 3);
    CHECK_EQ(kWarriorGuild, 1);
    CHECK_EQ(kAdvanceGuildGoldDivisor, 10);
    CHECK(kPreMlHitsChance > 0.66 && kPreMlHitsChance < 0.67);
}

// Tier = round_half_even(level / 25). Level/25 never lands on an exact .5
// for integer levels, so these boundaries are unambiguous.
TEST(quest_tier_rounds_level_over_25) {
    CHECK_EQ(QuestTier(0), 1);    // 0 -> clamped up to 1 (reference Rules.QuestTier; SRD R46/T11)
    CHECK_EQ(QuestTier(12), 1);   // 0.48 -> 0 -> clamped up to 1
    CHECK_EQ(QuestTier(13), 1);   // 0.52 -> 1
    CHECK_EQ(QuestTier(37), 1);   // 1.48 -> 1
    CHECK_EQ(QuestTier(38), 2);   // 1.52 -> 2
    CHECK_EQ(QuestTier(50), 2);   // 2.00 -> 2
}

// SRD: tier <= 10.
TEST(quest_tier_stays_within_cap) {
    CHECK_EQ(QuestTier(250), 10);  // 10.00 exactly
    CHECK(QuestTier(500) <= kQuestTierCap);
}

// SRD: rounded draw <= 5 gives let-off (5.5%).
TEST(quest_let_off_for_roll_five_or_less) {
    CHECK(QuestLetOff(1));
    CHECK(QuestLetOff(5));
    CHECK(!QuestLetOff(6));
}

// Non-rare candidates are never rerolled, whatever the draw.
TEST(reroll_never_rejects_non_rare) {
    tsup::Env env({0.1, 0.1});
    CHECK(!QuestRerollRare(env.ctx, 10, false));  // monster weight 10 is not rare
    CHECK(!QuestRerollRare(env.ctx, 5, true));    // item weight 5 is not rare
}

// Rare monsters (weight <= 9) are rejected with 75% probability:
// draws below 0.75 reject, draws at or above 0.75 keep.
TEST(reroll_rare_monster_rejected_three_quarters_of_the_time) {
    tsup::Env low({0.1, 0.1});
    CHECK(QuestRerollRare(low.ctx, 9, false));
    tsup::Env high({0.9, 0.9});
    CHECK(!QuestRerollRare(high.ctx, 9, false));
}

// Rare items are weight <= 4. Weight 4 is rare, weight 5 is not.
TEST(reroll_rare_item_threshold_is_weight_four) {
    tsup::Env rare({0.1, 0.1});
    CHECK(QuestRerollRare(rare.ctx, 4, true));
    tsup::Env notRare({0.1, 0.1});
    CHECK(!QuestRerollRare(notRare.ctx, 5, true));
    tsup::Env monsterWeightAsItem({0.1, 0.1});
    CHECK(!QuestRerollRare(monsterWeightAsItem.ctx, 9, true));  // rare only as a monster
}

// Item fetch quests start only when guild level is greater than 15.
TEST(item_fetch_offered_only_above_level_15) {
    CHECK(!ItemFetchOffered(1));
    CHECK(!ItemFetchOffered(15));
    CHECK(ItemFetchOffered(16));
}

// Band = Int(tier / 1.5) + 1, with Int truncating toward zero.
TEST(item_quest_band_is_int_tier_over_1_5_plus_one) {
    CHECK_EQ(ItemQuestBand(0), 1);   // 0.00 -> 0 -> 1
    CHECK_EQ(ItemQuestBand(1), 1);   // 0.67 -> 0 -> 1
    CHECK_EQ(ItemQuestBand(2), 2);   // 1.33 -> 1 -> 2
    CHECK_EQ(ItemQuestBand(3), 3);   // 2.00 -> 2 -> 3
    CHECK_EQ(ItemQuestBand(4), 3);   // 2.67 -> 2 -> 3
    CHECK_EQ(ItemQuestBand(10), 7);  // 6.67 -> 6 -> 7
}

// Pick range is first+1 .. last (span = last - first = 3 here).
TEST(pick_item_index_ranges_from_first_plus_one_to_last) {
    tsup::Env lo({0.0, 0.0});
    CHECK_EQ(PickItemIndex(lo.ctx, 2, 5), 3);   // first+1
    tsup::Env mid({0.5, 0.5});
    CHECK_EQ(PickItemIndex(mid.ctx, 2, 5), 4);  // Int(1.5)=1 -> first+2
    tsup::Env hi({0.999, 0.999});
    CHECK_EQ(PickItemIndex(hi.ctx, 2, 5), 5);   // last, never last+1
}

// Documented quirk: span 0 returns first+1 regardless of the draw.
TEST(pick_item_index_zero_span_returns_first_plus_one) {
    tsup::Env env({0.7, 0.7});
    CHECK_EQ(PickItemIndex(env.ctx, 7, 7), 8);
}

// Guild A/D is the maximum across guilds, taken independently for attack and defence.
TEST(guild_att_def_takes_max_over_joined_guilds) {
    Character c;
    GuildMembership& w = gm_slot(c, Guild::Warrior);
    w.joined = true;
    w.att = 6;
    w.def = 1;
    GuildMembership& p = gm_slot(c, Guild::Paladin);
    p.joined = true;
    p.att = 4;
    p.def = 3;
    GuildAttDefResult r = GuildAttDef(c);
    CHECK_EQ(r.att, 6);
    CHECK_EQ(r.def, 3);
}

// A character with no guild memberships has zero guild A/D.
TEST(guild_att_def_zero_for_character_without_guilds) {
    Character c;
    GuildAttDefResult r = GuildAttDef(c);
    CHECK_EQ(r.att, 0);
    CHECK_EQ(r.def, 0);
}

// Joining is free when the character holds no guild.
TEST(join_cost_is_zero_when_no_guild_held) {
    tsup::Env env({0.5, 0.5});
    Character c;
    CHECK_EQ(GuildJoinCost(env.ctx, c), 0);
}

// A character with no worn items has no cursed worn gear.
TEST(has_cursed_worn_false_for_default_character) {
    Character c;
    CHECK(!HasCursedWorn(c));
}

// CON bonus applies only above 16. At or below the threshold there is no bonus.
TEST(con_hits_bonus_zero_at_or_below_threshold) {
    tsup::Env env({0.5, 0.5});
    CHECK_EQ(ConHitsBonus(env.ctx, 16), 0);
    CHECK_EQ(ConHitsBonus(env.ctx, 10), 0);
}

// Above 16 the bonus is drawn from 1 .. CON-16.
TEST(con_hits_bonus_is_random_one_to_con_minus_16) {
    tsup::Env single({0.5, 0.5});
    CHECK_EQ(ConHitsBonus(single.ctx, 17), 1);  // range is exactly 1..1

    tsup::Env lo({0.0, 0.0});
    CHECK_EQ(ConHitsBonus(lo.ctx, 20), 1);
    tsup::Env hi({0.999, 0.999});
    CHECK_EQ(ConHitsBonus(hi.ctx, 20), 4);      // range 1..4

    tsup::Env two({0.5, 0.5});
    const i32 v = ConHitsBonus(two.ctx, 18);    // range 1..2
    CHECK(v >= 1 && v <= 2);
}

// Documented quirk: a draw of exactly 0.0 always yields the minimum for quest band and pick.
TEST(zero_draw_yields_minimum_item_index) {
    tsup::Env env({0.0, 0.0});
    CHECK_EQ(PickItemIndex(env.ctx, 0, 0), 1);
    CHECK_EQ(PickItemIndex(env.ctx, 10, 12), 11);
}
