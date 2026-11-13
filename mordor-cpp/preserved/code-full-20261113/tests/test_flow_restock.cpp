// Store restock on rest, through the public game API. Reference: CityWindow rest (days = the FIRST member's
// RestInCity = round_half_even((100 - hits%)/3.25), 0 when unhurt); only if days > 0 the shelf is replenished with
// chance clamp(days,1,50)% per pass (StoreStock.Replenish): adds only uncursed floor>0 items, at most 9 per item and alignment.
// Written by the coordinator; the implementing ants may not change it.
#include <algorithm>
#include "flow_support.h"

using namespace flowsup;

namespace {
const ItemDef* defOf(const Session& s, i32 id) {
  for (const ItemDef& d : s.data.items) if (d.id == id) return &d;
  return nullptr;
}
Vec<i32> shelfIds(const Session& s) {
  Vec<i32> v;
  for (const Item& it : s.town.store) v.push_back(it.id * 8 + static_cast<i32>(it.alignment));
  std::sort(v.begin(), v.end());
  return v;
}
void hurtFirst(Session& s) { s.game.party.members[0].vitals.curHits = 1; }   // 1 of 15 hits: round((100-6.67)/3.25) = 29 days
}  // namespace

TEST(flow_restock_rest_reports_the_days_the_first_member_aged) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  hurtFirst(s);
  CHECK(townAction(s, "rest", reply));
  CHECK(reply.find("aged 29 day(s)") != Str::npos);
}

TEST(flow_restock_an_unhurt_party_leaves_the_shelf_alone) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  s.town.deepestReached = 10;
  const Vec<i32> before = shelfIds(s);
  for (int i = 0; i < 40; ++i) townAction(s, "rest", reply);
  CHECK(shelfIds(s) == before);                      // days = 0 for everyone: no replenish at all
}

TEST(flow_restock_resting_hurt_changes_the_shelf_over_time) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  s.town.deepestReached = 10;
  const Vec<i32> before = shelfIds(s);
  for (int i = 0; i < 80; ++i) { hurtFirst(s); townAction(s, "rest", reply); }
  CHECK(shelfIds(s) != before);
}

TEST(flow_restock_respects_the_shelf_cap_and_adds_only_eligible_items) {
  Session s; CHECK(makeSession(s, 5));
  Str reply;
  s.town.deepestReached = 14;
  Vec<i32> idsBefore; for (const Item& it : s.town.store) idsBefore.push_back(it.id);
  for (int i = 0; i < 400; ++i) { hurtFirst(s); townAction(s, "rest", reply); }
  for (const Item& it : s.town.store) {
    const ItemDef* d = defOf(s, it.id);
    CHECK(d != nullptr);
    if (d == nullptr) continue;
    bool wasThere = std::find(idsBefore.begin(), idsBefore.end(), it.id) != idsBefore.end();
    if (!wasThere) { CHECK_EQ(d->cursed, 0); CHECK(d->floor > 0); }       // what restock adds is uncursed and not floor-0
    if (d->floor > 0) {
      i32 same = 0;
      for (const Item& o : s.town.store) if (o.id == it.id && o.alignment == it.alignment) ++same;
      CHECK(same <= 9);                                                    // ShelfCap per item and alignment
    }
  }
}

TEST(flow_restock_the_deepest_level_reached_follows_the_party_down) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  CHECK_EQ(s.town.deepestReached, 0);
  townAction(s, "descend", reply);
  CHECK(s.town.deepestReached >= 1);
}
