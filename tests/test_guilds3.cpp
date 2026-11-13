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

