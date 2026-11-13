#include "test_support.h"
#include "mordor/items1b.h"

using namespace mordor;

namespace {

constexpr int kTheftDraws = 120000;
constexpr int kTheftSlots = 42;  // outcomes 0..41

struct TheftTally {
  int count[kTheftSlots];
  int outside;  // draws outside 0..41 (should not happen)
};

}  // namespace

// SKIP B_0056#1: CombatMonster / CombatMonster::LoadRecord (with Hits, AtNought, Alive)
// are not declared in our headers, so the MDATA5 record load cannot be expressed yet.
