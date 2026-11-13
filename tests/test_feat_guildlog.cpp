#include "flow_support.h"
#include "mordor/access.h"
#include "mordor/features.h"
using namespace flowsup;

// Defined in src/feat_guildlog.cpp; move this declaration into features.h.
namespace mordor {
void guildlogSeedNewGame(Session& s, const Vec<Str>& masterNames);
}

TEST(feat_guildlog_seed_writes_once) {
  Session s;
  CHECK(makeSession(s, 1));
  mordor::Vec<mordor::Str> names(static_cast<size_t>(mordor::kGuildCount));
  names[0] = "Aldric";
  mordor::guildlogSeedNewGame(s, names);
  CHECK(s.featureState["guildlog.count"] == "1");
  mordor::guildlogSeedNewGame(s, names);
  CHECK(s.featureState["guildlog.count"] == "1");
}
