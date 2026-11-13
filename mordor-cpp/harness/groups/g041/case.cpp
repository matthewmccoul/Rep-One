#include "case_api.h"
#include "mordor/state_party.h"
#include "mordor/death1a.h"

namespace mordor {

// Export wrapper for the file-local copy in dungeon1b_b.cpp (see shim file).
void shimRemoveMemberDungeon1bB(Party& party, i32 idx);

static Party makeParty(const JsonValue& a) {
    Party p{};
    p.size = static_cast<i32>(A(a, "n").asInt());
    return p;
}

static Str run_removeMember(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Party p = makeParty(a);
    i32 idx = static_cast<i32>(A(a, "idx").asInt());
    i32 before = p.size;
    shimRemoveMemberDungeon1bB(p, idx);
    i32 after = p.size;
    return I(after < before ? 1 : 0) + "|" + I(after);
}

static Str run_removePartyMember(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Party p = makeParty(a);
    i32 idx = static_cast<i32>(A(a, "idx").asInt());
    i32 before = p.size;
    removePartyMember(p, idx);
    i32 after = p.size;
    return I(after < before ? 1 : 0) + "|" + I(after);
}

const CopyEntry kCopies[] = {
    {"removeMember", &run_removeMember},
    {"removePartyMember", &run_removePartyMember},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
