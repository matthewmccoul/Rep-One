#include "test_support.h"
#include "mordor/items1b.h"
#include "mordor/partyb.h"

using namespace mordor;

namespace {

// A character with a name, alignment and dungeon level, living at the given location.
Character makeChar(const char* name, Align al, i32 dungeonLevel) {
    Character c;
    c.name = name;
    c.alignment = al;
    c.location.dungeonLevel = dungeonLevel;
    c.vitals.curHits = 10;
    c.vitals.maxHits = 15;
    return c;
}

// A party whose size is n. Every member slot starts alive so pruning does not remove anyone.
Party makeParty(i32 n) {
    Party p;
    p.size = n;
    for (u32 i = 0; i < static_cast<u32>(kPartyMaxSize); ++i) {
        p.members[i].vitals.curHits = 10;
    }
    return p;
}

i32 count(const Vec<i32>& v) {
    return static_cast<i32>(v.size());
}

}  // namespace

TEST(pool_gold_to_selected_sums_onto_selected) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.selectedIndex = 1;
    party.members[0].wallet.goldOnHand = 10;
    party.members[1].wallet.goldOnHand = 20;
    party.members[2].wallet.goldOnHand = 30;
    poolGoldToSelected(ctx, party);
    CHECK_EQ(party.members[1].wallet.goldOnHand, 60);
    CHECK_EQ(party.members[0].wallet.goldOnHand, 0);
    CHECK_EQ(party.members[2].wallet.goldOnHand, 0);
}

