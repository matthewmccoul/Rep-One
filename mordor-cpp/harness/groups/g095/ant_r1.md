=== FILE: cases.json ===
[
{"args": {"type": -32768}, "rng": [0.5]},
{"args": {"type": -1}, "rng": [0.5]},
{"args": {"type": 0}, "rng": [0.5]},
{"args": {"type": 1}, "rng": [0.5]},
{"args": {"type": 2}, "rng": [0.5]},
{"args": {"type": 15}, "rng": [0.5]},
{"args": {"type": 21}, "rng": [0.5]},
{"args": {"type": 22}, "rng": [0.5]},
{"args": {"type": 23}, "rng": [0.5]},
{"args": {"type": 28}, "rng": [0.5]},
{"args": {"type": 29}, "rng": [0.5]},
{"args": {"type": 30}, "rng": [0.5]},
{"args": {"type": 31}, "rng": [0.5]},
{"args": {"type": 32}, "rng": [0.5]},
{"args": {"type": 33}, "rng": [0.5]},
{"args": {"type": 34}, "rng": [0.5]},
{"args": {"type": 35}, "rng": [0.5]},
{"args": {"type": 36}, "rng": [0.5]},
{"args": {"type": 64}, "rng": [0.5]},
{"args": {"type": 255}, "rng": [0.5]},
{"args": {"type": 1000}, "rng": [0.5]},
{"args": {"type": 32767}, "rng": [0.5]}
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
        // Pure function of the type code; rng is not consumed by this rule.
        int t = a.GetProperty("type").GetInt32();
        var it = new Item { Type = (short)t };
        bool r = ItemUse.MustBeEquippedToUse(it);
        return (r ? 1 : 0).ToString();
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"

namespace mordor {

static Str run_isMustBeEquippedType(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule does not draw random numbers
    i32 t = static_cast<i32>(A(a, "type").asInt());
    return I(isMustBeEquippedType(t) ? 1 : 0);
}

static Str run_mustBeEquipped(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule does not draw random numbers
    i32 t = static_cast<i32>(A(a, "type").asInt());
    return I(mustBeEquipped(t) ? 1 : 0);
}

const CopyEntry kCopies[] = {
    {"isMustBeEquippedType", &run_isMustBeEquippedType},
    {"mustBeEquipped", &run_mustBeEquipped},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

REFERENCE: ItemUse.MustBeEquippedToUse