#include "flow_support.h"
#include "mordor/access.h"
#include "mordor/casting1.h"
using namespace flowsup;

namespace mordor {
bool dcastRun(Session& s, const Vec<Str>& w, Str& reply);
}

// Smoke test on real data: a combat-only spell is refused by the combat gate, and the
// refusal leaves Aldric's spell points and the party's state untouched.
// NOTE: no non-combat effect is applied yet (see NEED lines), so this checks the refusal path only.
TEST(feat_dcast_combat_refusal_spends_no_sp) {
  Session s;
  CHECK(makeSession(s, 1));
  const SpellDef* combat = nullptr;
  for (const SpellDef& d : s.data.spells) {
    if (spellUse(d) == SpellUse::Combat) { combat = &d; break; }
  }
  CHECK(combat != nullptr);
  Character* aldric = accMember(s, 1);
  CHECK(aldric != nullptr);
  const i32 spBefore = aldric->vitals.curSpellPoints;
  const i32 partyXBefore = s.game.party.x;
  Vec<Str> w = {"cast", "1", combat->name};
  Str reply;
  CHECK(dcastRun(s, w, reply));
  CHECK(aldric->vitals.curSpellPoints == spBefore);
  CHECK(s.game.party.x == partyXBefore);
  CHECK(reply.find("combat spell") != Str::npos);
}
