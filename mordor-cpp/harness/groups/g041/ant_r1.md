=== FILE: cases.json ===
[
{"args":{"n":4,"idx":0},"rng":[0.5]},
{"args":{"n":4,"idx":1},"rng":[0.5]},
{"args":{"n":4,"idx":2},"rng":[0.5]},
{"args":{"n":4,"idx":3},"rng":[0.5]},
{"args":{"n":4,"idx":4},"rng":[0.5]},
{"args":{"n":4,"idx":-1},"rng":[0.5]},
{"args":{"n":3,"idx":0},"rng":[0.5]},
{"args":{"n":3,"idx":1},"rng":[0.5]},
{"args":{"n":3,"idx":2},"rng":[0.5]},
{"args":{"n":3,"idx":3},"rng":[0.5]},
{"args":{"n":3,"idx":-1},"rng":[0.5]},
{"args":{"n":2,"idx":0},"rng":[0.5]},
{"args":{"n":2,"idx":1},"rng":[0.5]},
{"args":{"n":2,"idx":2},"rng":[0.5]},
{"args":{"n":2,"idx":5},"rng":[0.5]},
{"args":{"n":2,"idx":-1},"rng":[0.5]},
{"args":{"n":1,"idx":0},"rng":[0.5]},
{"args":{"n":1,"idx":1},"rng":[0.5]},
{"args":{"n":1,"idx":-1},"rng":[0.5]},
{"args":{"n":4,"idx":9},"rng":[0.5]}
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

Notes on what this test can and cannot show:

- **Identity is not observable on the C++ side.** The C++ `Character` fields shown are not enough to tell members apart, so the output is only the shrink flag and the final size. A copy that shifts members wrongly but keeps the right size will still match. Adding a per-member identity field (for example a name or current hits) to both sides would close this gap, but that field must be confirmed from the C++ `Character` definition first.
- **Selected index is not compared.** The reference's `SelectedIndex` clamp has no C++ counterpart in the shown types, so it is left out of both strings.
- **Index-to-member mapping.** The reference takes a `Character`, not an index. The C# side maps an in-range index to `Members[idx]`, and an out-of-range index to a non-member, for which `Remove` returns false with no change. This mapping is a harness translation, not part of the reference.
- **`removeMember` has undefined behaviour for negative indices.** Cases with `idx` = -1 write through `static_cast<u32>(-1)`. The run may crash on those cases. The out-of-range non-negative cases (`idx` ≥ size) are defined but wrong: the copy decrements size and clears a live member. That divergence is what the test is meant to expose.
- **Header paths are assumed.** `mordor/state_party.h` is taken from the file name in the type listing. `mordor/death1a.h` is inferred from the `death1a.h` declaration. Both need checking against the real tree.

REFERENCE: Party.Remove