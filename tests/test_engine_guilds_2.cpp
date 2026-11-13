#include "test_support.h"
#include "mordor/guilds1.h"
#include "mordor/guilds2.h"
#include "mordor/state_character.h"
#include "mordor/types.h"

using mordor::i32;
using mordor::u32;
using mordor::i64;
using mordor::f32;
using mordor::Arr;
using mordor::Character;

// SKIP B_0059#62: needs an MDATA3 fixture with a specific one-item run; no API to inject such records.
// SKIP B_0059#63: needs the same custom MDATA3 run fixture as B_0059#62.
// SKIP B_0059#65: QuestItemReachable in guilds1.h takes only (ctx, tier); no item-type set parameter to express "all item types".
// SKIP B_0059#66: QuestItemReachable takes no item-kind count; zero-kind case cannot be expressed.
// SKIP B_0059#67: monster level is not exposed through the headers (MonsterDef field names unknown), so "at tier level" cannot be checked.
// SKIP B_0061#9: GoldFactor is a GuildDef field whose name does not appear in the headers; only the constant kGoldFactor table exists, which holds different values.

// Draw-count test. Assumes the sentinel "nothing" return is 0 for questMonster and questItem.
TEST(engine_B_0059_64_quest_draws_never_empty) {
    tsup::Env env(12345);
    i32 emptyMonster = 0;
    i32 emptyItem = 0;
    for (i32 tier = 1; tier <= 10; ++tier) {
        for (i32 n = 0; n < 400; ++n) {
            if (mordor::questMonster(env.ctx, tier) == 0) {
                ++emptyMonster;
            }
            if (mordor::questItem(env.ctx, tier) == 0) {
                ++emptyItem;
            }
        }
    }
    CHECK_EQ(emptyMonster + emptyItem, 0);
}

TEST(engine_B_0061_6_warrior_join_cost) {
    // R26 / T5: reference GuildJoinCost(Warrior) = 5,009,672. The formula has no per-guild base term.
    const auto& guildDef = tsup::data().guilds[static_cast<u32>(mordor::Guild::Warrior)];
    Character ch{};
    i64 cost = mordor::guildJoinCost(guildDef, ch) / mordor::kCurrencyScale;   // whole gold, as the reference reports it
    CHECK_EQ(cost, i64{5009672});
}

TEST(engine_B_0061_7_ninja_join_cost) {
    const auto& guildDef = tsup::data().guilds[static_cast<u32>(mordor::Guild::Ninja)];
    Character ch{};
    i64 cost = mordor::guildJoinCost(guildDef, ch) / mordor::kCurrencyScale;   // whole gold, as the reference reports it
    CHECK_EQ(cost, 10886091);
}

TEST(engine_B_0061_8_thief_join_cost) {
    // R26 / T5: reference GuildJoinCost(Thief) = 7,465,060. The formula has no per-guild base term.
    const auto& guildDef = tsup::data().guilds[static_cast<u32>(mordor::Guild::Thief)];
    Character ch{};
    i64 cost = mordor::guildJoinCost(guildDef, ch) / mordor::kCurrencyScale;   // whole gold, as the reference reports it
    CHECK_EQ(cost, i64{7465060});
}
