#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/loot1a.h"
#include "mordor/combat2a.h"

namespace mordor {

// Renamed same-name copies. The harness renames each definition to these names.
ChestTrapResult resolveChestTrap__loot1a(Ctx& ctx, Party& party, i32 openerIndex, ChestTrap trap);
void resolveChestTrap__combat2a(Ctx& ctx, Party& party, ChestTrap trap);

static ChestTrap trapOf(i32 n) {
    switch (n) {
        case 1: return ChestTrap::PoisonGas;
        case 2: return ChestTrap::DiseaseSpores;
        case 3: return ChestTrap::Explosion;
        case 4: return ChestTrap::Fire;
        case 5: return ChestTrap::ItemDestruction;
        default: return ChestTrap::Fate;
    }
}

static Str keyOf(const char* prefix, i32 i) { return Str(prefix) + I(i); }

static i32 N(const JsonValue& a, const char* k) { return static_cast<i32>(A(a, k).asInt()); }

// Builds the party from the same keys the C# side reads.
static void buildParty(const JsonValue& a, Party& p) {
    const i32 count = N(a, "count");
    const i32 level = N(a, "level");
    p.size = count;
    p.dungeonLevel = level;
    p.opener = N(a, "opener");
    for (i32 i = 0; i < count && i < kPartyMaxSize; ++i) {
        Character& c = p.members[static_cast<u32>(i)];
        c.vitals.curHits = N(a, keyOf("h", i).c_str());
        c.vitals.maxHits = N(a, keyOf("mh", i).c_str());
        c.vitals.curSpellPoints = N(a, keyOf("s", i).c_str());
        c.vitals.maxSpellPoints = N(a, keyOf("ms", i).c_str());
        c.location.dungeonLevel = level;
    }
}

// Canonical: poisoned|diseased|destroyed|hits per member|spell points per member.
static Str finish(const Party& p, Str poisoned, Str diseased, Str destroyed) {
    Str s = poisoned + "|" + diseased + "|" + destroyed;
    for (i32 i = 0; i < p.size; ++i) s += "|" + I(p.members[static_cast<u32>(i)].vitals.curHits);
    for (i32 i = 0; i < p.size; ++i) s += "|" + I(p.members[static_cast<u32>(i)].vitals.curSpellPoints);
    return s;
}

static Str run_loot1a(const JsonValue& a, Ctx& ctx) {
    Party p;
    buildParty(a, p);
    ChestTrapResult r = resolveChestTrap__loot1a(ctx, p, N(a, "opener"), trapOf(N(a, "trap")));
    return finish(p, I(r.poisoned ? 1 : 0), I(r.diseased ? 1 : 0), I(r.itemsDestroyed ? 1 : 0));
}

// combat2a returns void and its ailment and item results are not observable through the shown
// fields, so those three flags print NA. This mismatch is reported, not hidden.
static Str run_combat2a(const JsonValue& a, Ctx& ctx) {
    Party p;
    buildParty(a, p);
    resolveChestTrap__combat2a(ctx, p, trapOf(N(a, "trap")));
    return finish(p, Str("NA"), Str("NA"), Str("NA"));
}

const CopyEntry kCopies[] = {
    {"resolveChestTrap@loot1a", &run_loot1a},
    {"resolveChestTrap@combat2a", &run_combat2a},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
