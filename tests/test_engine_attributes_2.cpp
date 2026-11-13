#include "test_support.h"
#include "mordor/attributes.h"

// SKIP B_0064#10: the long stat name list is UI text at character creation; no name-list API in the headers (kStatCount is a count of stats, not the name list)
// SKIP B_0068#13: potion effects are not in the attributes headers; race maximum is not exposed as an API
// SKIP B_0068#14: potion effects and the message list are not in the attributes headers
// SKIP B_0068#15: hall of records is not in the attributes headers
// SKIP B_0068#16: reversed/opposing item effects and hall of records are not in the attributes headers
// SKIP A_03#12: no save-file importer function in the headers
// SKIP A_03#15: no save-file importer function in the headers
// SKIP A_03#16: no save-file importer function in the headers

TEST(engine_B_0065_109_stats_ad_all_six_at_15) {
    mordor::Arr<mordor::i32, mordor::kStatCount> stats{};
    stats[static_cast<mordor::u32>(mordor::StatIndex::Str)] = 15;
    stats[static_cast<mordor::u32>(mordor::StatIndex::Dex)] = 15;
    stats[static_cast<mordor::u32>(mordor::StatIndex::Int)] = 15;
    stats[static_cast<mordor::u32>(mordor::StatIndex::Wis)] = 15;
    stats[static_cast<mordor::u32>(mordor::StatIndex::Con)] = 15;
    stats[static_cast<mordor::u32>(mordor::StatIndex::Cha)] = 15;
    CHECK_EQ(mordor::statsAD(stats), 14);
    CHECK_EQ(mordor::statsADArchive(15, 15, 15, 15), 14);
}
