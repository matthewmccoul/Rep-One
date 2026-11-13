#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

namespace mordor {
bool duseUseCommand(Session& s, const Vec<Str>& w, Str& reply);
}

// A consumable with charges is spent and, on its last charge, leaves the pack.
TEST(feat_duse_consumed_item_leaves_pack) {
  Session s;
  CHECK(makeSession(s, 1));

  const ItemDef* def = nullptr;
  for (const ItemDef& d : s.data.items) {
    if (d.type >= 23 && d.type <= 28 && d.specialType >= 0) {
      def = &d;
      break;
    }
  }
  CHECK(def != nullptr);

  Item* slot = accPackItem(s, 1, 2);
  CHECK(slot != nullptr);
  *slot = Item();
  slot->id = def->id;
  slot->charges = 1;

  Vec<Str> w = {"use", "1", "2"};
  Str reply;
  CHECK(mordor::duseUseCommand(s, w, reply));
  CHECK(accPackItem(s, 1, 2)->id == 0);
}
