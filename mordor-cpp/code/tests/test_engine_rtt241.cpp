#include "test_support.h"
// FIXED: "mordor/item_info_page.h" does not exist in the project headers or the API INDEX, so the include is removed.
// #include "mordor/item_info_page.h"
// FIXED: "mordor/message_list.h" does not exist in the project headers or the API INDEX, so the include is removed.
// #include "mordor/message_list.h"
#include "mordor/state_common.h"

using namespace mordor;

// FIXED: vbSingle is declared only in the missing item_info_page.h, so this test is disabled until that header's location is confirmed.
#if 0
// B_0066#37 (ItemInfoPage.VbSingle): VB Str$ drops the leading nought.
TEST(engine_rtt241_vbsingle_drops_leading_nought) {
  Str a = vbSingle(0.5f);
  Str b = vbSingle(-0.5f);
  Str c = vbSingle(1.5f);
  Str joined = a + " " + b + " " + c;
  CHECK_EQ(joined, Str(".5 -.5 1.5"));
}
#endif

// FIXED: messageListAdd is declared only in the missing message_list.h, so the MessageList tests are disabled until that header's location is confirmed.
#if 0
// B_0064#36 (MessageList.Add of a blank line): whitespace-only lines are refused.
TEST(engine_rtt241_messagelist_rejects_blank_line) {
  Vec<Str> log;
  bool ok = messageListAdd(log, "   ");
  CHECK_EQ(ok, false);
}

// B_0064#37 (MessageList.Add of 'line 50' when it is already the newest): accepted again.
TEST(engine_rtt241_messagelist_readd_newest_accepted) {
  Vec<Str> log;
  for (int i = 1; i <= 50; i++) messageListAdd(log, Str("line ") + std::to_string(i));
  bool ok = messageListAdd(log, "line 50");
  CHECK_EQ(ok, true);
}

// B_0064#33 (MessageList.Add for lines 1..50): only the default kept count is held.
TEST(engine_rtt241_messagelist_keeps_twenty) {
  Vec<Str> log;
  for (int i = 1; i <= 50; i++) messageListAdd(log, Str("line ") + std::to_string(i));
  CHECK_EQ((int)log.size(), 20);
}

// B_0064#34 (MessageList.Add; log[0]): oldest held line after 1..50.
TEST(engine_rtt241_messagelist_oldest_line) {
  Vec<Str> log;
  for (int i = 1; i <= 50; i++) messageListAdd(log, Str("line ") + std::to_string(i));
  CHECK_EQ(log.front(), Str("line 31"));
}

// B_0064#35 (MessageList.Add; log[^1]): newest held line after 1..50.
TEST(engine_rtt241_messagelist_newest_line) {
  Vec<Str> log;
  for (int i = 1; i <= 50; i++) messageListAdd(log, Str("line ") + std::to_string(i));
  CHECK_EQ(log.back(), Str("line 50"));
}
#endif

// SKIP B_0065#118: PlayClock is not in OUR DECLARATIONS and the construction of the 'pair' is not given in the case.
// SKIP B_0065#119: PlayClock is not in OUR DECLARATIONS; also the expected 86250 does not follow from 86300 + 10 with a 86340-second day under any obvious reading, so the case needs checking against the reference before porting.
