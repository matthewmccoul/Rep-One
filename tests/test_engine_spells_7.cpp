#include "test_support.h"

#include "mordor/spells2.h"
#include "mordor/state_character.h"

using namespace mordor;

// SKIP B_0065#75: castCureParalysis takes only the caster; no way to target Bob from Ann, and the cast line text is not exposed as a checkable event.
// SKIP B_0065#77: targetPromptFor returns an opaque i32; no mapping to PeopleAndCompanions is exposed.
// SKIP B_0065#78: targetPromptFor returns an opaque i32; no mapping to PeopleAndCompanions is exposed.
// SKIP B_0065#79: targetPromptFor returns an opaque i32; no mapping to None is exposed.
// SKIP B_0065#80: needs Charm Humanoid spell id and class-filter data from GameData, plus the caption event text format (not in headers).
// SKIP B_0065#82: needs the charm setup from #80 (spell id, class filter) and a post-charm encounter state check; setup not derivable from headers.
// SKIP B_0065#83: needs Charm Humanoid spell id and class-filter data from GameData, plus the "Failed!" caption event format.
// SKIP B_0065#84: needs the failed-charm setup from #83 (spell id, class filter, 30000-hit monster) to check hits afterwards.
// SKIP B_0065#86: needs Charm Humanoid spell id and a wrong-class monster setup from GameData, plus the "Failed!" caption event format.
