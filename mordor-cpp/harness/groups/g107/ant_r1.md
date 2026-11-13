I'm building the test with `MonsterProperty.Has` as the reference. It does not implement the out-of-range rule: `1 << bit` masks the shift count to 5 bits in C#, so bit 32 aliases bit 0 and bit -1 aliases bit 31. The out-of-range cases will show that disagreement, and I have not changed the reference to hide it.

=== FILE: cases.json ===
[
  {"args": {"flags": 0, "bit": 0}, "rng": [0.0]},
  {"args": {"flags": 1, "bit": 0}, "rng": [0.0]},
  {"args": {"flags": 2, "bit": 1}, "rng": [0.0]},
  {"args": {"flags": 2, "bit": 0}, "rng": [0.0]},
  {"args": {"flags": 8, "bit": 3}, "rng": [0.0]},
  {"args": {"flags": 16, "bit": 4}, "rng": [0.0]},
  {"args": {"flags": 32, "bit": 5}, "rng": [0.0]},
  {"args": {"flags": 32, "bit": 4}, "rng": [0.0]},
  {"args": {"flags": 5, "bit": 2}, "rng": [0.0]},
  {"args": {"flags": 5, "bit": 1}, "rng": [0.0]},
  {"args": {"flags": 65536, "bit": 16}, "rng": [0.0]},
  {"args": {"flags": 65536, "bit": 15}, "rng": [0.0]},
  {"args": {"flags": 4294967295, "bit": 0}, "rng": [0.0]},
  {"args": {"flags": 4294967295, "bit": 15}, "rng": [0.0]},
  {"args": {"flags": 4294967295, "bit": 31}, "rng": [0.0]},
  {"args": {"flags": 2147483648, "bit": 31}, "rng": [0.0]},
  {"args": {"flags": 2147483647, "bit": 31}, "rng": [0.0]},
  {"args": {"flags": 2147483648, "bit": 30}, "rng": [0.0]},
  {"args": {"flags": 1073741824, "bit": 30}, "rng": [0.0]},
  {"args": {"flags": 0, "bit": 31}, "rng": [0.0]},
  {"args": {"flags": 1, "bit": -1}, "rng": [0.0]},
  {"args": {"flags": 1, "bit": 32}, "rng": [0.0]},
  {"args": {"flags": 2147483649, "bit": 32}, "rng": [0.0]},
  {"args": {"flags": 2147483648, "bit": -1}, "rng": [0.0]},
  {"args": {"flags": 4294967295, "bit": 33}, "rng": [0.0]},
  {"args": {"flags": 4294967295, "bit": -32}, "rng": [0.0]},
  {"args": {"flags": 4, "bit": 34}, "rng": [0.0]},
  {"args": {"flags": 4, "bit": 2147483647}, "rng": [0.0]},
  {"args": {"flags": 4294967295, "bit": -2147483648}, "rng": [0.0]},
  {"args": {"flags": 2, "bit": 64}, "rng": [0.0]},
  {"args": {"flags": 1, "bit": 96}, "rng": [0.0]}
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
        long flags = a.GetProperty("flags").GetInt64();
        int bit = a.GetProperty("bit").GetInt32();
        var m = new Monster { SpecialPropertyFlags = unchecked((int)(uint)flags) };
        bool r = MonsterProperty.Has(m, bit);
        return r ? "1" : "0";
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/combat1a.h"
#include "mordor/companions1b.h"
#include "mordor/state_creature.h"

namespace mordor {

static Str run_monsterHasBit(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const long long flags = A(a, "flags").asInt();
    const i32 bit = static_cast<i32>(A(a, "bit").asInt());
    Creature c;
    c.specialPropertyFlags = static_cast<decltype(c.specialPropertyFlags)>(static_cast<u32>(flags));
    return monsterHasBit(c, bit) ? "1" : "0";
}

static Str run_specialFlagBit(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const u32 flags = static_cast<u32>(A(a, "flags").asInt());
    const i32 bit = static_cast<i32>(A(a, "bit").asInt());
    return specialFlagBit(flags, bit) ? "1" : "0";
}

const CopyEntry kCopies[] = {
    {"monsterHasBit", &run_monsterHasBit},
    {"specialFlagBit", &run_specialFlagBit},
};
const std::size_t kNumCopies = 2;

}
=== END FILE ===

Notes:
- The reference only matches the rule for bits 0..31. Out-of-range cases (bits 32, 33, 34, 64, 96, -1, -32, -2147483648, 2147483647) will show the reference's shift-masking result, which differs from the rule's "returns false". Those mismatches are real findings.
- Monster flags are passed in directly rather than read from a real record. The C++ monster record's field name for `SpecialPropertyFlags` is not shown, so I could not look it up by id without inventing a field.
- The `asInt()` calls assume 64-bit integer access, since flag values reach 4294967295.

REFERENCE: MonsterProperty.Has