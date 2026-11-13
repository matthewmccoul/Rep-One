#include "test_support.h"
#include "mordor/items1b.h"

// SKIP B_0056#15: needs enumeration of MDATA3 item definitions and ItemDef fields, which are not in the headers
// SKIP B_0056#16: no guild recharge function (RechargeGuildItems) in the headers
// SKIP B_0056#17: no guild recharge function (RechargeGuildItems) in the headers
// SKIP B_0056#18: no guild recharge function (RechargeGuildItems) in the headers
// SKIP B_0056#19: no guild recharge function (RechargeGuildItems) in the headers
// SKIP B_0056#20: no guild recharge function (RechargeGuildItems) in the headers
// SKIP B_0056#21: no guild recharge function (RechargeGuildItems) in the headers
// SKIP B_0056#22: needs enumeration of MDATA1 item types by Index; no such list function in the headers
// SKIP B_0056#25: needs a one-handed dagger item definition id from game data, which is not in the headers

TEST(engine_B_0056_23_max_worn_class0) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::maxWornOfClass(env.ctx, 0), 1);
}
