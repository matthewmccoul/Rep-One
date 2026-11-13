#include "test_support.h"

#include "mordor/spells2.h"
#include "mordor/state_character.h"

using namespace mordor;

// B_0065#76: resistance spell cast on self at 95 leaves the resistance at 95.
// The "<spell name> thrown on yourself." line is emitted as an event whose code/text
// format is not specified in the headers, so only the resistance value is asserted here.
TEST(engine_B_0065_76_resist_at_95_unchanged) {
    tsup::Env env({0.5, 0.9});
    Resistances r{};
    r.resist[0] = kResistGrantPercent;  // already 95
    castResistSpell(env.ctx, r, SpellResistKind::FIRE);
    CHECK_EQ(r.resist[0], 95);
}

// SKIP B_0065#75: castCureParalysis takes only the caster; no way to target Bob from Ann, and the cast line text is not exposed as a checkable event.
// SKIP B_0065#77: targetPromptFor returns an opaque i32; no mapping to PeopleAndCompanions is exposed.
// SKIP B_0065#78: targetPromptFor returns an opaque i32; no mapping to PeopleAndCompanions is exposed.
// SKIP B_0065#79: targetPromptFor returns an opaque i32; no mapping to None is exposed.
// SKIP B_0065#80: needs Charm Humanoid spell id and class-filter data from GameData, plus the caption event text format (not in headers).
// SKIP B_0065#82: needs the charm setup from #80 (spell id, class filter) and a post-charm encounter state check; setup not derivable from headers.
// SKIP B_0065#83: needs Charm Humanoid spell id and class-filter data from GameData, plus the "Failed!" caption event format.
// SKIP B_0065#84: needs the failed-charm setup from #83 (spell id, class filter, 30000-hit monster) to check hits afterwards.
// SKIP B_0065#86: needs Charm Humanoid spell id and a wrong-class monster setup from GameData, plus the "Failed!" caption event format.
