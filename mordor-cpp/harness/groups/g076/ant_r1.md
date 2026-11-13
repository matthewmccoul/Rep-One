=== FILE: cases.json ===
[
  {"args": {"spellIndex": 0}, "rng": [0.5]},
  {"args": {"spellIndex": 1}, "rng": [0.5]},
  {"args": {"spellIndex": 2}, "rng": [0.5]},
  {"args": {"spellIndex": 9}, "rng": [0.5]},
  {"args": {"spellIndex": 10}, "rng": [0.5]},
  {"args": {"spellIndex": 39}, "rng": [0.5]},
  {"args": {"spellIndex": 50}, "rng": [0.5]},
  {"args": {"spellIndex": 99}, "rng": [0.5]},
  {"args": {"spellIndex": 103}, "rng": [0.5]},
  {"args": {"spellIndex": 104}, "rng": [0.5]},
  {"args": {"spellIndex": 105}, "rng": [0.5]},
  {"args": {"spellIndex": 106}, "rng": [0.5]},
  {"args": {"spellIndex": 200}, "rng": [0.5]},
  {"args": {"spellIndex": 1000}, "rng": [0.5]},
  {"args": {"spellIndex": 2147483000}, "rng": [0.5]},
  {"args": {"spellIndex": -1}, "rng": [0.5]},
  {"args": {"spellIndex": -2}, "rng": [0.5]},
  {"args": {"spellIndex": -39}, "rng": [0.5]},
  {"args": {"spellIndex": -40}, "rng": [0.5]},
  {"args": {"spellIndex": -41}, "rng": [0.5]},
  {"args": {"spellIndex": -1000}, "rng": [0.5]},
  {"args": {"spellIndex": -2147483648}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int spellIndex = a.GetProperty("spellIndex").GetInt32();
        int buffer = SpellBuffers.ForSpell(spellIndex);
        return buffer.ToString(CultureInfo.InvariantCulture);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"

namespace mordor {

// Renamed copies (same-name bufferForSpell in two files).
i32 bufferForSpell__feat_buffers(i32 spellIndex);
i32 bufferForSpell__items2a(i32 spellIndex);

// Export wrapper for the file-local copy, defined in shim_feat_buffers.cpp.
i32 shim_feat_buffers_export(i32 spellIndex);

static Str run_feat_buffers(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 spellIndex = static_cast<i32>(A(a, "spellIndex").asInt());
    return I(shim_feat_buffers_export(spellIndex));
}

static Str run_items2a(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 spellIndex = static_cast<i32>(A(a, "spellIndex").asInt());
    return I(bufferForSpell__items2a(spellIndex));
}

const CopyEntry kCopies[] = {
    {"bufferForSpell@feat_buffers", &run_feat_buffers},
    {"bufferForSpell@items2a", &run_items2a},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

=== FILE: shim_feat_buffers.cpp ===
namespace mordor {
i32 shim_feat_buffers_export(i32 spellIndex) {
    return bufferForSpell__feat_buffers(spellIndex);
}
}
=== END FILE ===

REFERENCE: SpellBuffers.ForSpell