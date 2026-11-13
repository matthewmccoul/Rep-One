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

// SKIP B_0062#20: Race/Guild Chart text is not in the headers; GuildData.All enumeration and GuildDef.AllowsRace string join have no header-visible equivalent.
// SKIP B_0062#21: Same as B_0062#20; the chart reference data and the mismatch-string output are not available in the headers.
// SKIP B_0062#22: The page's takeover-level list is not in the headers, and GuildData.IndexOf / Guildmasters.Seeded are not exposed; only kSeedMasterName/kSeedMasterLevel exist, with no page data to compare against.
// SKIP B_0062#23: Same as B_0062#22; the page comparison data and mismatch output cannot be reproduced from the headers.
