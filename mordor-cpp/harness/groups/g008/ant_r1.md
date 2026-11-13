=== FILE: cases.json ===
[
{"args":{"hits":10,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":0,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":5,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":9,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":8,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":19,"max_hits":20,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":29,"max_hits":30,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":28,"max_hits":30,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":1,"max_hits":3,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":2,"max_hits":3,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":0,"max_hits":0,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":0,"max_hits":-5,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":-2,"max_hits":0,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":-10,"max_hits":-1,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":15,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":-5,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":-10,"max_hits":10,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":0,"max_hits":1,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":1,"max_hits":1,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":1,"max_hits":1000000,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":2147483646,"max_hits":2147483647,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":1073741823,"max_hits":2147483647,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":3,"max_hits":7,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":6,"max_hits":7,"binding":-1,"monster_id":1},"rng":[0.5]},
{"args":{"hits":2,"max_hits":9,"binding":-1,"monster_id":100},"rng":[0.5]},
{"args":{"hits":4,"max_hits":4,"binding":-1,"monster_id":100},"rng":[0.5]},
{"args":{"hits":-1,"max_hits":-1,"binding":-1,"monster_id":100},"rng":[0.5]},
{"args":{"hits":0,"max_hits":-1,"binding":-1,"monster_id":100},"rng":[0.5]},
{"args":{"hits":5,"max_hits":10,"binding":-12,"monster_id":100},"rng":[0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Linq;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, Random rng)
    {
        int hits = a.GetProperty("hits").GetInt32();
        int maxHits = a.GetProperty("max_hits").GetInt32();
        int binding = a.GetProperty("binding").GetInt32();
        int monsterId = a.GetProperty("monster_id").GetInt32();

        var who = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        var c = new Companion { Hits = hits, MaxHits = maxHits, BindingLevel = binding };
        var def = H.Monsters.First(m => m.Id == monsterId);

        // Only the Binding < 0 branch is under test; rng is unused by this rule.
        return H.F(CompanionMood.Drift(who, c, def));
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/companions1b.h"
#include "mordor/companions2a.h"
#include "mordor/status1b.h"

namespace mordor {

// Copy driftContentStep (src/companions2a.cpp): takes (hits, maxHits).
static Str run_driftContentStep(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 hits = static_cast<i32>(A(a, "hits").asInt());
    const i32 maxHits = static_cast<i32>(A(a, "max_hits").asInt());
    return F(driftContentStep(hits, maxHits));
}

// Copy companionDriftContent (src/companions1b.cpp): takes (hits, maxHits).
static Str run_companionDriftContent(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 hits = static_cast<i32>(A(a, "hits").asInt());
    const i32 maxHits = static_cast<i32>(A(a, "max_hits").asInt());
    return F(companionDriftContent(hits, maxHits));
}

// Copy contentDrift (src/status1b.cpp): takes (missingHits, maxHits).
// The reference computes missing = MaxHits - Hits, so missing is derived the same way here.
static Str run_contentDrift(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 hits = static_cast<i32>(A(a, "hits").asInt());
    const i32 maxHits = static_cast<i32>(A(a, "max_hits").asInt());
    const i32 missingHits = static_cast<i32>(static_cast<long long>(maxHits) - static_cast<long long>(hits));
    return F(contentDrift(missingHits, maxHits));
}

const CopyEntry kCopies[] = {
    {"driftContentStep", &run_driftContentStep},
    {"companionDriftContent", &run_companionDriftContent},
    {"contentDrift", &run_contentDrift},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor
=== END FILE ===

Notes on the test:
- The reference's `Math.Max(0.5, drift)` also applies when MaxHits <= 0, so the reference returns 0.5 there. `companionDriftContent` returns 0.0 for that case, so the test should flag it as a mismatch. I left that as is.
- The C++ copies are declared in headers and are not file-local, so no shim files are needed. I assumed the headers sit at `mordor/companions1b.h`, `mordor/companions2a.h` and `mordor/status1b.h`, and that the copies live in namespace `mordor`.
- `contentDrift` depends on `kDriftHpScale` and `kContentDriftFloor`, which were not shown. The test will expose them if they are not 15 and 0.5.
- The rule takes no Random, so every case uses a single draw of 0.5. The monster is only needed because `Drift` requires a non-null `Monster`. I assumed `H.Monsters` is an enumerable of `Monster` with `Id` values 1 and 100 present.

REFERENCE: CompanionMood.Drift