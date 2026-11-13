=== FILE: cases.json ===
[
{"args":{"guildLevel":-2147483648},"rng":[0.5]},
{"args":{"guildLevel":-1000},"rng":[0.5]},
{"args":{"guildLevel":-4},"rng":[0.5]},
{"args":{"guildLevel":-3},"rng":[0.5]},
{"args":{"guildLevel":-2},"rng":[0.5]},
{"args":{"guildLevel":-1},"rng":[0.5]},
{"args":{"guildLevel":0},"rng":[0.5]},
{"args":{"guildLevel":1},"rng":[0.5]},
{"args":{"guildLevel":2},"rng":[0.5]},
{"args":{"guildLevel":3},"rng":[0.5]},
{"args":{"guildLevel":4},"rng":[0.5]},
{"args":{"guildLevel":5},"rng":[0.5]},
{"args":{"guildLevel":6},"rng":[0.5]},
{"args":{"guildLevel":7},"rng":[0.5]},
{"args":{"guildLevel":8},"rng":[0.5]},
{"args":{"guildLevel":100},"rng":[0.5]},
{"args":{"guildLevel":254},"rng":[0.5]},
{"args":{"guildLevel":508},"rng":[0.5]},
{"args":{"guildLevel":509},"rng":[0.5]},
{"args":{"guildLevel":510},"rng":[0.5]},
{"args":{"guildLevel":511},"rng":[0.5]},
{"args":{"guildLevel":512},"rng":[0.5]},
{"args":{"guildLevel":600},"rng":[0.5]},
{"args":{"guildLevel":1000},"rng":[0.5]},
{"args":{"guildLevel":2147483647},"rng":[0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // R21 is a pure function of guildLevel; the Random argument is not used by the reference.
    public static string Run(JsonElement a, Random rng)
    {
        int guildLevel = a.GetProperty("guildLevel").GetInt32();
        int sl = Rules.SpellLevel(guildLevel);
        return sl.ToString(CultureInfo.InvariantCulture);
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/casting1.h"
#include "mordor/casting2.h"

namespace mordor {

// Pure function: ctx.rng is not consumed.
static Str run_guildSpellLevel(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 guildLevel = static_cast<i32>(A(a, "guildLevel").asInt());
    return I(guildSpellLevel(guildLevel));
}

static Str run_spellLevelFromGuildLevel(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 guildLevel = static_cast<i32>(A(a, "guildLevel").asInt());
    return I(spellLevelFromGuildLevel(guildLevel));
}

const CopyEntry kCopies[] = {
    {"guildSpellLevel", &run_guildSpellLevel},
    {"spellLevelFromGuildLevel", &run_spellLevelFromGuildLevel},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

Notes:
- Both copies are declared in headers with external linkage, so no shim file is needed.
- Negative and zero guild levels all clamp to 1 in the reference, so the C# truncating cast and the C++ `vbInt` (floor-style) can only differ on inputs that clamp to the same value. The test cannot tell them apart, and that is expected.
- The reference clamps at a hard-coded 255. The second copy clamps at `kSpellLevelCap`, whose value is not shown. If that cap is not 255, the cases at 512 and above will show a mismatch, and that is a real finding. Likewise, `guildHalfLevelRounded` is not shown, so its rounding is only tested through the outputs.
- `casting3.h` also declares `spellLevelFromGuildLevel`. I did not include it, so the declaration is only seen through `casting2.h`. If `casting3.cpp` defines the same function, the link will fail on a duplicate symbol. That needs checking in the project.

REFERENCE: Rules.SpellLevel