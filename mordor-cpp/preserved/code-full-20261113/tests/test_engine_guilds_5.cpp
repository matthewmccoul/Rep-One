#include "test_support.h"
#include "mordor/guilds1.h"

namespace {

// Builds a character holding one joined guild membership at the given level.
mordor::Character withGuild(mordor::Guild guild, mordor::i32 level) {
    mordor::Character c;
    auto& m = c.guilds[static_cast<mordor::u32>(guild)];
    m.joined = true;
    m.level = level;
    return c;
}

}  // namespace

// B_0061#136: Scavenger at guild level 522 (page maximum).
TEST(engine_B_0061_136_scavenger_max_ad) {
    mordor::Character c = withGuild(mordor::Guild::Scavenger, 522);
    // FIXED: mordor::AdPair / bestGuildPair do not exist in guilds1.h; use bestGuildAD → BestAd.
    // Displayed values include the 10/10 base (Rules.BaseAD).
    mordor::BestAd r = mordor::bestGuildAD(tsup::data(), c);  // derives A/D from the guild level (R38)
    CHECK_EQ(r.att + 10, 196);  // Char page = BaseAD 10 + guild award
    CHECK_EQ(r.def + 10, 222);
}

// B_0061#137: Mage at guild level 475 (page maximum).
TEST(engine_B_0061_137_mage_max_ad) {
    mordor::Character c = withGuild(mordor::Guild::Mage, 475);
    mordor::BestAd r = mordor::bestGuildAD(tsup::data(), c);  // derives A/D from the guild level (R38)
    CHECK_EQ(r.att + 10, 102);  // Char page = BaseAD 10 + guild award
    CHECK_EQ(r.def + 10, 194);
}

// B_0061#138: Sorcerer at guild level 467 (page maximum).
TEST(engine_B_0061_138_sorcerer_max_ad) {
    mordor::Character c = withGuild(mordor::Guild::Sorcerer, 467);
    mordor::BestAd r = mordor::bestGuildAD(tsup::data(), c);  // derives A/D from the guild level (R38)
    CHECK_EQ(r.att + 10, 98);  // Char page = BaseAD 10 + guild award
    CHECK_EQ(r.def + 10, 195);
}

// B_0061#139: Wizard at guild level 622 (page maximum).
TEST(engine_B_0061_139_wizard_max_ad) {
    mordor::Character c = withGuild(mordor::Guild::Wizard, 622);
    mordor::BestAd r = mordor::bestGuildAD(tsup::data(), c);  // derives A/D from the guild level (R38)
    CHECK_EQ(r.att + 10, 106);  // Char page = BaseAD 10 + guild award
    CHECK_EQ(r.def + 10, 232);
}

// B_0061#140: Healer at guild level 470 (page maximum).
TEST(engine_B_0061_140_healer_max_ad) {
    mordor::Character c = withGuild(mordor::Guild::Healer, 470);
    mordor::BestAd r = mordor::bestGuildAD(tsup::data(), c);  // derives A/D from the guild level (R38)
    CHECK_EQ(r.att + 10, 124);  // Char page = BaseAD 10 + guild award
    CHECK_EQ(r.def + 10, 200);
}

// B_0061#141: Thief at 394 and Nomad at 197; best attack and defence taken independently.
TEST(engine_B_0061_141_thief_nomad_best_independent) {
    mordor::Character c;
    auto& thief = c.guilds[static_cast<mordor::u32>(mordor::Guild::Thief)];
    thief.joined = true;
    thief.level = 394;
    auto& nomad = c.guilds[static_cast<mordor::u32>(mordor::Guild::Nomad)];
    nomad.joined = true;
    nomad.level = 197;
    mordor::BestAd r = mordor::bestGuildAD(tsup::data(), c);  // derives A/D from the guild level (R38)
    CHECK_EQ(r.att + 10, 175);  // Char page = BaseAD 10 + guild award
    CHECK_EQ(r.def + 10, 204);
}

// SKIP B_0062#20: Race/Guild Chart text is not in the headers; GuildData.All enumeration and GuildDef.AllowsRace string join have no header-visible equivalent.
// SKIP B_0062#21: Same as B_0062#20; the chart reference data and the mismatch-string output are not available in the headers.
// SKIP B_0062#22: The page's takeover-level list is not in the headers, and GuildData.IndexOf / Guildmasters.Seeded are not exposed; only kSeedMasterName/kSeedMasterLevel exist, with no page data to compare against.
// SKIP B_0062#23: Same as B_0062#22; the page comparison data and mismatch output cannot be reproduced from the headers.
