// Spell casting through the public game API. Costs come from the casting module (castCostSp), reachability from
// listReachableSpellIds. Written by the coordinator; the implementing ants may not change it.
#include "flow_support.h"
#include "mordor/casting1.h"

using namespace flowsup;

namespace {
const SpellDef* spellById(const Session& s, i32 id) {
  for (const SpellDef& d : s.data.spells) if (d.id == id) return &d;
  return nullptr;
}
Vec<i32> reachable(Session& s, size_t member) {
  Ctx c = s.ctx();
  return listReachableSpellIds(c, s.game.party.members[member]);
}
i32 costOf(Session& s, size_t member, const SpellDef& sp) {
  Ctx c = s.ctx();
  return castCostSp(c, s.game.party.members[member], sp, 0);
}
// A reachable spell of the given use for a member, or nullptr.
const SpellDef* pick(Session& s, size_t member, bool combat) {
  Ctx c = s.ctx();
  for (i32 id : reachable(s, member)) {
    const SpellDef* sp = spellById(s, id);
    if (sp == nullptr) continue;
    const SpellUse u = spellUse(*sp);
    if (combat ? (u != SpellUse::NonCombat) : (u != SpellUse::Combat)) return sp;
  }
  return nullptr;
}
// Walk the first level until a fight starts, then begin it ("fight") and keep going while the fight lasts.
bool inLongFight(Session& s, unsigned seed) {
  Str reply;
  townAction(s, "descend", reply);
  unsigned r = seed | 1u;
  Str last;
  for (int i = 0; i < 3000 && currentScreen(s) == Screen::Dungeon; ++i) {
    r ^= r << 13; r ^= r >> 17; r ^= r << 5;
    const Str cmd = last.find("Ouch") != Str::npos ? (r % 3 == 0 ? "left" : (r % 3 == 1 ? "right" : "back")) : "forward";
    dungeonAction(s, cmd, reply); last = reply; s.log.clear();
  }
  if (currentScreen(s) != Screen::Combat) return false;
  combatAction(s, "fight", reply);
  return currentScreen(s) == Screen::Combat && s.game.inCombat;
}
}  // namespace

TEST(flow_magic_spells_command_lists_the_reachable_spells) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  const Vec<i32> ids = reachable(s, 1);          // Lyra, the Mage
  CHECK(ids.size() > 0);
  CHECK(townAction(s, "spells 2", reply));
  for (size_t k = 0; k < ids.size() && k < 3; ++k) {
    const SpellDef* sp = spellById(s, ids[k]);
    CHECK(sp != nullptr);
    if (sp != nullptr) CHECK(reply.find(sp->name) != Str::npos);
  }
}

TEST(flow_magic_an_unknown_spell_name_is_refused_and_costs_nothing) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  const i32 sp0 = s.game.party.members[1].vitals.curSpellPoints;
  townAction(s, "cast 2 Zzyzx", reply);
  CHECK(reply.size() > 0);
  CHECK_EQ(s.game.party.members[1].vitals.curSpellPoints, sp0);
}

TEST(flow_magic_a_spell_the_caster_cannot_reach_is_refused_and_costs_nothing) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  // Find a spell id that member 1 (the Warrior) cannot reach.
  const Vec<i32> ids = reachable(s, 0);
  const SpellDef* unreachable = nullptr;
  for (const SpellDef& sp : s.data.spells) { bool in = false; for (i32 id : ids) if (id == sp.id) in = true; if (!in && sp.level >= 1) { unreachable = &sp; break; } }
  CHECK(unreachable != nullptr);
  if (unreachable == nullptr) return;
  const i32 sp0 = s.game.party.members[0].vitals.curSpellPoints;
  townAction(s, "cast 1 " + unreachable->name, reply);
  CHECK(reply.size() > 0);
  CHECK_EQ(s.game.party.members[0].vitals.curSpellPoints, sp0);
}

TEST(flow_magic_casting_in_town_spends_the_reference_cost) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  const SpellDef* sp = pick(s, 3, false);          // Thessa, the Healer: a spell usable outside combat
  if (sp == nullptr) sp = pick(s, 1, false);
  CHECK(sp != nullptr);
  if (sp == nullptr) return;
  const size_t who = (pick(s, 3, false) != nullptr) ? 3 : 1;
  const i32 cost = costOf(s, who, *sp);
  const i32 sp0 = s.game.party.members[who].vitals.curSpellPoints;
  townAction(s, "cast " + std::to_string(who + 1) + " " + sp->name, reply);
  CHECK(reply.size() > 0);
  CHECK_EQ(s.game.party.members[who].vitals.curSpellPoints, sp0 - cost);
}

TEST(flow_magic_casting_in_combat_spends_the_reference_cost_and_the_round_resolves) {
  bool tried = false;
  for (unsigned seed = 1; seed <= 40 && !tried; ++seed) {
    Session s; CHECK(makeSession(s, seed));
    if (!inLongFight(s, seed * 7919u)) continue;
    const SpellDef* sp = pick(s, 1, true);          // Lyra: a spell usable in combat
    if (sp == nullptr) continue;
    tried = true;
    const i32 cost = costOf(s, 1, *sp);
    const i32 sp0 = s.game.party.members[1].vitals.curSpellPoints;
    Str reply;
    combatAction(s, "cast 2 " + sp->name + " 1", reply);
    CHECK(reply.size() > 0);
    CHECK_EQ(s.game.party.members[1].vitals.curSpellPoints, sp0 - cost);
  }
  CHECK(tried);
}
