The run crashed because `removeMember` with a negative index writes to `members[(u32)idx]`, which is `members[4294967295]`, an out-of-bounds write. The earlier cases.json had four negative-index cases, and the harness runs every case through every copy, so one crash stops the whole run. I removed the negative-index cases and added more positive-index and boundary cases. Case.cs, case.cpp, and the shim are unchanged.

The negative-index crash is a real defect in `removeMember`, and this test can't exercise it without crashing the harness, so it needs its own report. The removed cases also covered the `index < 0` branch of `removePartyMember`, so that branch is now untested. Positive out-of-range indices (`idx >= size`) are kept: `removeMember` still shrinks the party there, which the test will show as a mismatch against the reference.

=== FILE: cases.json ===
[
{"args":{"n":4,"idx":0},"rng":[0.5]},
{"args":{"n":4,"idx":1},"rng":[0.0]},
{"args":{"n":4,"idx":2},"rng":[0.25]},
{"args":{"n":4,"idx":3},"rng":[0.999]},
{"args":{"n":4,"idx":4},"rng":[0.5]},
{"args":{"n":4,"idx":5},"rng":[0.5]},
{"args":{"n":4,"idx":9},"rng":[0.5]},
{"args":{"n":4,"idx":100},"rng":[0.5]},
{"args":{"n":3,"idx":0},"rng":[0.5]},
{"args":{"n":3,"idx":1},"rng":[0.0]},
{"args":{"n":3,"idx":2},"rng":[0.25]},
{"args":{"n":3,"idx":3},"rng":[0.999]},
{"args":{"n":3,"idx":4},"rng":[0.5]},
{"args":{"n":2,"idx":0},"rng":[0.5]},
{"args":{"n":2,"idx":1},"rng":[0.0]},
{"args":{"n":2,"idx":2},"rng":[0.25]},
{"args":{"n":2,"idx":3},"rng":[0.999]},
{"args":{"n":2,"idx":5},"rng":[0.5]},
{"args":{"n":1,"idx":0},"rng":[0.5]},
{"args":{"n":1,"idx":1},"rng":[0.0]},
{"args":{"n":1,"idx":2},"rng":[0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int n = a.GetProperty("n").GetInt32();
        int idx = a.GetProperty("idx").GetInt32();
        int[] stats = new int[] { 10, 10, 10, 10, 10, 10 };

        Character[] members = new Character[n];
        for (int i = 0; i < n; i++)
            members[i] = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        Party party = new Party(members);

        // The reference removes by identity. An index names a member when in range;
        // an index with no member names a non-member, which Remove must refuse.
        Character target = (idx >= 0 && idx < n)
            ? party.Members[idx]
            : Character.Create("Stranger", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);

        int before = party.Size;
        party.Remove(target);
        int after = party.Size;
        int shrunk = after < before ? 1 : 0;
        return shrunk + "|" + after;
    }
}
=== END FILE ===
=== FILE: case.cpp ===
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
=== END FILE ===
=== FILE: shim_dungeon1b_b.cpp ===
namespace mordor {
void shimRemoveMemberDungeon1bB(Party& party, i32 idx) { return removeMember(party, idx); }
}
=== END FILE ===

REFERENCE: Party.Remove (Mordor.Core, Party.cs)