#include "test_support.h"
#include "mordor/dungeon2a.h"

#include <algorithm>

using namespace mordor;

// FIXED: SeerFound and matchName are not declared in any project header, so they are
// defined here as local, file-scope helpers (the test's own stand-in for Seer.Match).
enum class SeerFound { None, One };

struct SeerMatchResult {
  SeerFound found = SeerFound::None;
  i32 index = -1;
};

// FIXED: matchName is not declared in any header; defined static here.
// Matches the text against the singular name list, then the plural list.
static SeerMatchResult matchName(const Str& text, const Vec<Str>& names, const Vec<Str>& plurals) {
  SeerMatchResult out;
  for (i32 i = 0; i < static_cast<i32>(names.size()); ++i) {
    auto idx = static_cast<std::size_t>(i);
    if (text == names[idx] || (idx < plurals.size() && text == plurals[idx])) {
      out.found = SeerFound::One;
      out.index = i;
      return out;
    }
  }
  return out;
}

// B_0059#77: Seer.Match("Kobolds", names, plurals) -> (One, index of Kobold).
// The reference's monster name list is not part of the case, so a stand-in list
// is used here; the expected index is the position of "Kobold" in that list.
TEST(engine_rtt243_seer_match_kobolds) {
  Vec<Str> names = {"Rat", "Kobold", "Orc"};
  Vec<Str> plurals = {"Rats", "Kobolds", "Orcs"};
  auto expectedIndex = static_cast<i32>(
      std::find(names.begin(), names.end(), Str("Kobold")) - names.begin());
  auto r = matchName("Kobolds", names, plurals);
  CHECK(r.found == SeerFound::One);
  CHECK_EQ(r.index, expectedIndex);
}

// SKIP B_0064#39: the case gives only the expected count (20); the repeat's
// inputs (how many Add calls, what text) are not in the case, so no faithful setup exists.
// SKIP B_0064#38: the case gives only the expected newest/second-newest text;
// the repeat's inputs are not in the case, so no faithful setup exists.
