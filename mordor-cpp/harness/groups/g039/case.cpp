#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

// Exports from the shim files. The file-local copies are reached only through these,
// so no pruneTheDead declaration is visible here (that caused the ambiguous call).
void shim_partyb_prune(Ctx& ctx, Party& party);
void shim_death1a_prune(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead);

static i32 slot(const JsonValue& a, const char* prefix, i32 i) {
    Str k = Str(prefix) + I(i);
    return static_cast<i32>(A(a, k.c_str()).asInt());
}

static void fillParty(Party& p, const JsonValue& a) {
    const i32 n = static_cast<i32>(A(a, "n").asInt());
    for (i32 i = 0; i < n; ++i) {
        Character& m = p.members[i];
        m.name = Str(1, static_cast<char>('A' + i));
        m.vitals.curHits = slot(a, "h", i);
        m.location.dungeonLevel = slot(a, "l", i);
        m.location.x = slot(a, "x", i);
        m.location.y = slot(a, "y", i);
    }
    p.size = n;
    p.selectedIndex = static_cast<i32>(A(a, "sel").asInt());
}

// Same canonical form as the C# Dump: size|selected, then per member
// name|curHits|isDead|stoned|dungeonLevel|x|y.
static Str dump(const Party& p) {
    Str s = I(p.size) + "|" + I(p.selectedIndex);
    for (i32 i = 0; i < p.size; ++i) {
        const Character& m = p.members[i];
        s += "|" + m.name
           + "|" + I(m.vitals.curHits)
           + "|" + I(m.isDead() ? 1 : 0)
           + "|" + I(m.deathState == DeathState::Stoned ? 1 : 0)
           + "|" + I(m.location.dungeonLevel)
           + "|" + I(m.location.x)
           + "|" + I(m.location.y);
    }
    return s;
}

static Str run_partyb(const JsonValue& a, Ctx& ctx) {
    Party p;
    fillParty(p, a);
    shim_partyb_prune(ctx, p);
    return dump(p);
}

static Str run_death1a(const JsonValue& a, Ctx& ctx) {
    Party p;
    fillParty(p, a);
    Arr<bool, kPartyMaxSize> countedDead{};  // no casualties counted before the call
    shim_death1a_prune(ctx, p, countedDead);
    return dump(p);
}

const CopyEntry kCopies[] = {
    {"pruneTheDead@partyb", &run_partyb},
    {"pruneTheDead@death1a", &run_death1a},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
