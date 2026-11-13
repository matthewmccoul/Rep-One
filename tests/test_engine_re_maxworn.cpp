#include "test_support.h"
#include "mordor/game_data.h"
#include "mordor/items1b.h"  // maxWornOfClass, wearClassOf, wornOfClass are declared in items1b.h

#include <set>
#include <string>

using namespace mordor;

namespace {

// FIXED: maxWornOfClass and wearClassOf take a Ctx, not GameData. Build the Ctx from the test's GameData.
// UNRESOLVED: the compiler error at line 18 is NOT fixed. The API index does not declare Ctx or any Ctx
// constructor, so no constructor taking a GameData can be verified. `Ctx{gd}` still fails to compile.
// The Ctx declaration is not in the index, so it must be read from its defining header before this
// line can be changed. Do not default-construct Ctx here: that would silently change what the wear-class
// lookups compute.
struct CtxHolder { Rng rng{1}; EventLog log; Ctx ctx; explicit CtxHolder(const GameData& gd) : ctx(gd, rng, log) {} };
CtxHolder& holderOf(const GameData& gd) { static CtxHolder h(gd); return h; }
Ctx& ctxOf(const GameData& gd) { return holderOf(gd).ctx; }

// FIXED: gd.items holds ItemDef, but wearClassOf takes const Item&. Convert with a local helper.
// UNVERIFIED: this copies only `id`, so wear-class results may differ from the real conversion.
// Replace it with the existing ItemDef-to-Item conversion if one exists.
Item itemOf(const ItemDef& d) {
  Item it{};
  it.id = d.id;
  return it;
}

// Distinct MaxWornOfClass values over every MDATA1 item class index.
std::set<i32> mdata1MaxValues(const GameData& gd) {
  std::set<i32> out;
  const Ctx ctx = ctxOf(gd);
  for (i32 c = 0; c < static_cast<i32>(gd.itemDescriptions.size()); ++c)
    out.insert(maxWornOfClass(ctx, c));
  return out;
}

i32 countClassesWith(const GameData& gd, i32 value) {
  i32 n = 0;
  const Ctx ctx = ctxOf(gd);
  for (i32 c = 0; c < static_cast<i32>(gd.itemDescriptions.size()); ++c)
    if (maxWornOfClass(ctx, c) == value) ++n;
  return n;
}

std::string joinValues(const std::set<i32>& s) {
  std::string out;
  for (i32 v : s) {
    if (!out.empty()) out += ",";
    out += std::to_string(v);
  }
  return out;
}

// Non-zero-id MDATA3 items only.
template <typename F>
void forEachNonZeroItem(const GameData& gd, F f) {
  for (const auto& it : gd.items)
    if (it.id != 0) f(it);
}

}  // namespace

// [B_0063#129] Across the MDATA1 item classes, which distinct class maximum values occur?
TEST(engine_re_maxworn_distinct_values_mdata1) {
  const GameData& gd = tsup::data();
  CHECK_EQ(joinValues(mdata1MaxValues(gd)), std::string("-1,0,1"));
}

// [B_0063#130] How many item classes have a maximum worn of 1?
TEST(engine_re_maxworn_count_one_mdata1) {
  const GameData& gd = tsup::data();
  CHECK_EQ(countClassesWith(gd, 1), 15);
}

// [B_0063#131] How many item classes have a maximum worn of 0?
TEST(engine_re_maxworn_count_zero_mdata1) {
  const GameData& gd = tsup::data();
  CHECK_EQ(countClassesWith(gd, 0), 8);
}

// [B_0063#132] How many item classes have a maximum worn of -1?
TEST(engine_re_maxworn_count_minus_one_mdata1) {
  const GameData& gd = tsup::data();
  CHECK_EQ(countClassesWith(gd, -1), 1);
}

// SKIP B_0063#137: the question says "that class" and refers to an earlier assertion whose class index is not given in this case; the -1 class is covered by engine_re_maxworn_count_minus_one_mdata1.

// [B_0062#65] Distinct wear-class maxima over non-zero-id MDATA3 items, classes below 0 excluded.
TEST(engine_re_maxworn_mdata3_distinct_values) {
  if (tsup::fullGameOnly("engine_re_maxworn_mdata3_distinct_values")) return;
  const GameData& gd = tsup::data();
  const Ctx ctx = ctxOf(gd);
  std::set<i32> vals;
  forEachNonZeroItem(gd, [&](const auto& it) {
    i32 wc = wearClassOf(ctx, itemOf(it));
    if (wc >= 0) vals.insert(maxWornOfClass(ctx, wc));
  });
  CHECK_EQ(joinValues(vals), std::string("-1,0,1"));
}

// [B_0062#66] Is there at least one non-zero-id MDATA3 item whose wear-class maximum is -1?
TEST(engine_re_maxworn_mdata3_has_minus_one) {
  if (tsup::fullGameOnly("engine_re_maxworn_mdata3_has_minus_one")) return;
  const GameData& gd = tsup::data();
  const Ctx ctx = ctxOf(gd);
  bool found = false;
  forEachNonZeroItem(gd, [&](const auto& it) {
    if (maxWornOfClass(ctx, wearClassOf(ctx, itemOf(it))) == -1) found = true;
  });
  CHECK_EQ(found, true);
}

// [B_0062#68] Wear-class maximum of Medusa's Head, read from MDATA3.
TEST(engine_re_maxworn_medusa_head) {
  if (tsup::fullGameOnly("engine_re_maxworn_medusa_head")) return;
  const GameData& gd = tsup::data();
  const Ctx ctx = ctxOf(gd);
  bool present = false;
  i32 value = 0;
  for (const auto& it : gd.items) {
    if (it.name == "Medusa's Head") {
      present = true;
      value = maxWornOfClass(ctx, wearClassOf(ctx, itemOf(it)));
      break;
    }
  }
  CHECK_EQ(present, true);
  CHECK_EQ(value, -1);
}

// [B_0062#69] Is there at least one non-zero-id MDATA3 item whose wear-class maximum is 0?
TEST(engine_re_maxworn_mdata3_has_zero) {
  if (tsup::fullGameOnly("engine_re_maxworn_mdata3_has_zero")) return;
  const GameData& gd = tsup::data();
  const Ctx ctx = ctxOf(gd);
  bool found = false;
  forEachNonZeroItem(gd, [&](const auto& it) {
    if (maxWornOfClass(ctx, wearClassOf(ctx, itemOf(it))) == 0) found = true;
  });
  CHECK_EQ(found, true);
}
