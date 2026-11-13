// General store through the public game API. Expected values come from the reference StoreStock (PriceOf, SellPrice):
//   floor-0 items sell at the list price; others at round_half_even(1.5*P/(ln(shelf+1)+1.15) - ln(charges+1)*0.05*P)
//   with shelf = min(count of that item+alignment on the shelf - 1, 9).
// Written by the coordinator; the implementing ants may not change it.
#include <cmath>
#include "flow_support.h"

using namespace flowsup;

namespace {
constexpr Currency kGold = 10000;
const ItemDef* defOf(const Session& s, i32 id) {
  for (const ItemDef& d : s.data.items) if (d.id == id) return &d;
  return nullptr;
}
i32 countOnShelf(const Session& s, const Item& it) {
  i32 n = 0;
  for (const Item& o : s.town.store) if (o.id == it.id && o.alignment == it.alignment) ++n;
  return n;
}
Currency expectedPrice(const Session& s, const Item& it) {
  const ItemDef* d = defOf(s, it.id);
  if (d == nullptr) return -1;
  if (d->floor == 0) return static_cast<Currency>(d->price) * kGold;
  const i32 shelf = std::min(countOnShelf(s, it) - 1, 9);
  const double cost = (1.5 * d->price) / (std::log(shelf + 1.0) + 1.15) - std::log(it.charges + 1.0) * (0.05 * d->price);
  return static_cast<Currency>(roundHalfEven(cost)) * kGold;
}
}  // namespace

TEST(flow_shop_rows_are_numbered_one_to_n_without_gaps) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  CHECK(townAction(s, "shop", reply));
  i32 expect = 1, rows = 0;
  size_t pos = 0;
  while (pos < reply.size()) {
    size_t eol = reply.find('\n', pos); if (eol == Str::npos) eol = reply.size();
    Str line = reply.substr(pos, eol - pos); pos = eol + 1;
    size_t colon = line.find(':');
    if (line.size() > 2 && line[0] == ' ' && colon != Str::npos && colon < 6) {
      const i32 n = std::atoi(line.substr(0, colon).c_str());
      CHECK_EQ(n, expect);
      ++expect; ++rows;
    }
  }
  CHECK(rows > 5);
}

TEST(flow_shop_buying_charges_the_reference_price_and_takes_one_from_the_shelf) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  Purse& p = s.game.party.members[0].wallet;
  p.goldOnHand = 900000000LL * kGold;                    // rich enough for anything on the shelf
  // First distinct item on the shelf is row 1.
  const Item first = s.town.store[0];
  const Currency price = expectedPrice(s, first);
  const i32 before = countOnShelf(s, first);
  const Currency hand0 = p.goldOnHand;
  CHECK(townAction(s, "buy 1 1", reply));
  CHECK_EQ(hand0 - p.goldOnHand, price);
  CHECK_EQ(countOnShelf(s, first), before - 1);
}

TEST(flow_shop_a_stocked_magic_item_costs_the_stock_dependent_price) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  Purse& p = s.game.party.members[0].wallet;
  p.goldOnHand = 900000000LL * kGold;
  // Find a row whose item is not a floor-0 item (its price depends on the shelf).
  size_t at = s.town.store.size();
  for (size_t i = 0; i < s.town.store.size(); ++i) { const ItemDef* d = defOf(s, s.town.store[i].id); if (d && d->floor > 0) { at = i; break; } }
  CHECK(at < s.town.store.size());
  if (at >= s.town.store.size()) return;
  const Item it = s.town.store[at];
  const Currency price = expectedPrice(s, it);
  // Row numbers are 1..N over the distinct items, in shelf order.
  i32 row = 0; Vec<i32> seen;
  for (size_t i = 0; i <= at; ++i) { bool dup = false; for (i32 id : seen) if (id == s.town.store[i].id) dup = true; if (!dup) { seen.push_back(s.town.store[i].id); ++row; } }
  const Currency hand0 = p.goldOnHand;
  CHECK(townAction(s, "buy " + std::to_string(row) + " 1", reply));
  CHECK_EQ(hand0 - p.goldOnHand, price);
}

TEST(flow_shop_selling_back_pays_less_than_buying_and_puts_the_item_on_the_shelf) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  Purse& p = s.game.party.members[0].wallet;
  p.goldOnHand = 900000000LL * kGold;
  // A shelvable item: not floor 0 (floor-0 unaligned goods are never shelved on a sale), fewer than nine on the shelf.
  size_t at = s.town.store.size();
  for (size_t i = 0; i < s.town.store.size(); ++i) { const ItemDef* d = defOf(s, s.town.store[i].id); if (d && d->floor > 0 && d->price < 100000 && countOnShelf(s, s.town.store[i]) < 9) { at = i; break; } }
  CHECK(at < s.town.store.size());
  if (at >= s.town.store.size()) return;
  const Item it = s.town.store[at];
  i32 row = 0; Vec<i32> seen;
  for (size_t i = 0; i <= at; ++i) { bool dup = false; for (i32 id : seen) if (id == s.town.store[i].id) dup = true; if (!dup) { seen.push_back(s.town.store[i].id); ++row; } }
  const Currency hand0 = p.goldOnHand;
  CHECK(townAction(s, "buy " + std::to_string(row) + " 1", reply));
  const Currency paid = hand0 - p.goldOnHand;
  CHECK(paid > 0);
  const size_t shelfAfterBuy = s.town.store.size();
  i32 slot = -1;
  for (i32 k = 1; k < static_cast<i32>(kPackSlots); ++k) { const Item& pk = s.game.party.inventories[0].pack[static_cast<size_t>(k)]; if (pk.id == it.id && !pk.equipped) slot = k + 1; }
  CHECK(slot > 1);
  const Currency beforeSale = p.goldOnHand;
  CHECK(townAction(s, "sell " + std::to_string(slot) + " 1", reply));
  const Currency got = p.goldOnHand - beforeSale;
  CHECK(got > 0);                                       // something came back
  CHECK(got < paid);                                    // but never a profit on the same item
  CHECK_EQ(s.town.store.size(), shelfAfterBuy + 1);     // and it is on the shelf again
}

TEST(flow_shop_sale_message_states_whole_gold_not_raw_units) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  s.game.party.members[0].wallet.goldOnHand = 900000000LL * kGold;
  CHECK(townAction(s, "buy 3 1", reply));              // a shelvable item (Iron Dagger in the standard stock)
  townAction(s, "sell 4 1", reply);
  CHECK(reply.find("1/10000") == Str::npos);
  CHECK(reply.find("gold") != Str::npos);
}
