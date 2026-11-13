#include "flow_support.h"
#include "mordor/access.h"
#include "mordor/features.h"
using namespace flowsup;

// Defined in src/feat_guildlog.cpp; move this declaration into features.h.
namespace mordor {
void guildlogSeedNewGame(Session& s, const Vec<Str>& masterNames);
}

