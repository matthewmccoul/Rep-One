#include "test_support.h"

#include <string>
#include <vector>

#include "mordor/guilds1.h"
#include "mordor/guilds3.h"

using namespace mordor;

// SKIP B_0064#1: rendering of guild-log wordings (Wrote.* text) has no equivalent function in the new headers; addGuildLog takes already-rendered text.

TEST(engine_B_0064_2_guild_data_count) {
    CHECK_EQ(tsup::data().guilds.size(), 12);
}

// SKIP B_0064#4: GuildDef's ExtraSwings field is not present in the headers shown, so the per-guild count cannot be expressed.

TEST(engine_B_0064_5_named_guildmasters_on_new_game) {
    tsup::Env env(1234);
    static GameState game{};
    newGame(env.ctx, game);
    i32 named = 0;
    for (u32 i = 0; i < kGuildCount; ++i) {
        if (game.guildMasters[i].name != "") {
            ++named;
        }
    }
    CHECK_EQ(named, 12);
}

TEST(engine_B_0064_27_ninja_title_level_125) {
    CHECK_EQ(guildTitle(Guild::Ninja, 125), Str("Shogun"));
}

TEST(engine_B_0064_28_ninja_title_level_135) {
    CHECK_EQ(guildTitle(Guild::Ninja, 135), Str("Shogun"));
}

TEST(engine_B_0064_29_ninja_title_level_119) {
    CHECK_EQ(guildTitle(Guild::Ninja, 119), Str("Assassin"));
}

TEST(engine_B_0064_30_ninja_ninja_titles_over_1_to_400) {
    i32 count = 0;
    for (i32 level = 1; level <= 400; ++level) {
        Str title = guildTitle(Guild::Ninja, level);
        if (title == "Ninja" || title == "Master Ninja") {
            ++count;
        }
    }
    CHECK_EQ(count, 0);
}

TEST(engine_B_0064_31_non_ninja_title_ladders_no_repeats) {
    i32 repeats = 0;
    for (i32 g = 0; g < kGuildCount; ++g) {
        Guild guild = static_cast<Guild>(g);
        if (guild == Guild::Ninja) {
            continue;
        }
        std::vector<Str> runs;
        for (i32 level = 1; level <= 400; ++level) {
            Str title = guildTitle(guild, level);
            if (!runs.empty() && runs.back() == title) {
                continue;
            }
            for (u32 r = 0; r + 1 < runs.size(); ++r) {
                if (runs[r] == title) {
                    ++repeats;
                }
            }
            if (!runs.empty() && runs.back() != title) {
                // title changed; a match with an earlier run (checked above) is a repeat
            }
            runs.push_back(title);
        }
    }
    CHECK_EQ(repeats, 0);
}

TEST(engine_B_0065_108_nomad_all_stats_15_guild_ad) {
    // Guild level is not specified in the case; default membership is used.
    Character c{};
    c.activeGuild = Guild::Nomad;
    for (u32 s = 0; s < kStatCount; ++s) {
        c.abilities.stats[s] = 15;
    }
    GuildAttDefResult ad = GuildAttDefDisplay(c);  // Char page shows BaseAD (10/10) + best guild award
    CHECK_EQ(ad.att, 10);
    CHECK_EQ(ad.def, 10);
}
