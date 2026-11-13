=== FILE: cases.json ===
[
{"args": {"base": 10, "power": 5, "sl": 5, "resist": 0, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 10, "power": 5, "sl": 5, "resist": 50, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 12, "power": 3, "sl": 7, "resist": 25, "has_resist": 1}, "rng": [0.9]},
{"args": {"base": 20, "power": 8, "sl": 12, "resist": 100, "has_resist": 1}, "rng": [0.3]},
{"args": {"base": 20, "power": 8, "sl": 12, "resist": 150, "has_resist": 1}, "rng": [0.3]},
{"args": {"base": 20, "power": 8, "sl": 12, "resist": -20, "has_resist": 1}, "rng": [0.3]},
{"args": {"base": 0, "power": 1, "sl": 1, "resist": 0, "has_resist": 1}, "rng": [0.0]},
{"args": {"base": 0, "power": 1, "sl": 1, "resist": 0, "has_resist": 1}, "rng": [0.999999]},
{"args": {"base": 0, "power": 1, "sl": 3, "resist": 0, "has_resist": 1}, "rng": [0.7]},
{"args": {"base": 5, "power": 0, "sl": 5, "resist": 0, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 9, "power": 0, "sl": 15, "resist": 0, "has_resist": 1}, "rng": [0.2]},
{"args": {"base": 10, "power": 5, "sl": 0, "resist": 0, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 10, "power": 5, "sl": -7, "resist": 0, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 30, "power": 10, "sl": 255, "resist": 10, "has_resist": 1}, "rng": [0.4]},
{"args": {"base": 30, "power": 10, "sl": 256, "resist": 10, "has_resist": 1}, "rng": [0.4]},
{"args": {"base": 40, "power": 12, "sl": 300, "resist": 0, "has_resist": 1}, "rng": [0.6]},
{"args": {"base": -5, "power": 4, "sl": 6, "resist": 0, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 500, "power": 20, "sl": 40, "resist": 33, "has_resist": 1}, "rng": [0.123456]},
{"args": {"base": 5, "power": 3, "sl": 4, "resist": 50, "has_resist": 1}, "rng": [0.1]},
{"args": {"base": 7, "power": 3, "sl": 4, "resist": 50, "has_resist": 1}, "rng": [0.9]},
{"args": {"base": 9, "power": 2, "sl": 6, "resist": 30, "has_resist": 1}, "rng": [0.6]},
{"args": {"base": 11, "power": 6, "sl": 9, "resist": 70, "has_resist": 1}, "rng": [0.35]},
{"args": {"base": 5, "power": 4, "sl": 5, "resist": 10, "has_resist": 1}, "rng": [0.99]},
{"args": {"base": 2, "power": 1, "sl": 1, "resist": 50, "has_resist": 1}, "rng": [0.25]},
{"args": {"base": 1000, "power": 1, "sl": 1, "resist": 1, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 8, "power": 6, "sl": 4, "resist": 0, "has_resist": 1}, "rng": [0.9999999]},
{"args": {"base": 10, "power": 5, "sl": 5, "resist": 60, "has_resist": 0}, "rng": [0.5]},
{"args": {"base": 10, "power": 5, "sl": 5, "resist": 200, "has_resist": 0}, "rng": [0.5]},
{"args": {"base": 10, "power": -50, "sl": 20, "resist": 0, "has_resist": 1}, "rng": [0.5]},
{"args": {"base": 10, "power": 100, "sl": 100, "resist": 0, "has_resist": 1}, "rng": [0.5, 0.9]},
{"args": {"base": 6, "power": 2, "sl": 2, "resist": 90, "has_resist": 1}, "rng": [0.0]},
{"args": {"base": 3, "power": 2, "sl": 2, "resist": -1, "has_resist": 0}, "rng": [0.5]}
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
        int baseDmg = a.GetProperty("base").GetInt32();
        int power = a.GetProperty("power").GetInt32();
        int sl = a.GetProperty("sl").GetInt32();
        int resist = a.GetProperty("resist").GetInt32();
        bool hasResist = a.GetProperty("has_resist").GetInt32() != 0;
        // has_resist = 0 means no resistance is applied; that is the same as resistance 0 in the reference.
        int resistEff = hasResist ? resist : 0;
        int dmg = Rules.SpellDamage(rng, baseDmg, power, sl, resistEff);
        return dmg.ToString(CultureInfo.InvariantCulture);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/spells1a.h"
#include "mordor/spells3.h"

namespace mordor {

// Export for the file-local monsterSpellDamage (see shim_spells1a.cpp).
i32 mordor_export_monsterSpellDamage(Ctx& ctx, i32 monsterLevel, i32 damage2, i32 specialEffect,
                                     bool hasResist, i32 resistPercent);

static i32 geti(const JsonValue& a, const char* k) {
    return static_cast<i32>(A(a, k).asInt());
}

// Reference applies resistance unconditionally; has_resist = 0 maps to resistance 0 (identity).
static i32 resistEff(const JsonValue& a) {
    return geti(a, "has_resist") != 0 ? geti(a, "resist") : 0;
}

static Str run_playerSpellDamage(const JsonValue& a, Ctx& ctx) {
    const i32 base = geti(a, "base");
    const i32 power = geti(a, "power");
    const i32 sl = geti(a, "sl");
    // Copy's parameter is specialEffect = power - 1 (reference computes power - 1 internally).
    const i32 r = playerSpellDamage(ctx, base, power - 1, sl, resistEff(a));
    return I(r);
}

static Str run_monsterSpellDamage(const JsonValue& a, Ctx& ctx) {
    const i32 base = geti(a, "base");
    const i32 power = geti(a, "power");
    const i32 sl = geti(a, "sl");
    const bool hasResist = geti(a, "has_resist") != 0;
    // Copy takes hasResist and the raw resistance, so pass them unmodified.
    const i32 r = mordor_export_monsterSpellDamage(ctx, sl, base, power - 1, hasResist, geti(a, "resist"));
    return I(r);
}

static Str run_spellDamage(const JsonValue& a, Ctx& ctx) {
    const i32 r = spellDamage(ctx, geti(a, "base"), geti(a, "sl"), geti(a, "power"), resistEff(a));
    return I(r);
}

const CopyEntry kCopies[] = {
    {"playerSpellDamage", &run_playerSpellDamage},
    {"monsterSpellDamage", &run_monsterSpellDamage},
    {"spellDamage", &run_spellDamage},
};
const std::size_t kNumCopies = 3;

} // namespace mordor
=== END FILE ===

=== FILE: shim_spells1a.cpp ===
namespace mordor {
i32 mordor_export_monsterSpellDamage(Ctx& ctx, i32 monsterLevel, i32 damage2, i32 specialEffect,
                                     bool hasResist, i32 resistPercent) {
    return monsterSpellDamage(ctx, monsterLevel, damage2, specialEffect, hasResist, resistPercent);
}
} // namespace mordor
=== END FILE ===

Notes on the mapping:
- Output is the single return value. The rule changes no game state; its only side effect is the rng draws, which the harness scripts.
- `power` in the args is the reference's power. The player and monster copies take `power - 1`, so the wrapper subtracts 1.
- `has_resist = 0` maps to resistance 0 for the reference and the other copies. Only the monster copy receives the flag itself.
- The cases include values that the copies may handle differently from the reference: `sl` = 256 and 300 (the player copy clamps to 255, the reference has no upper bound), negative `power - 1` (truncation versus `vbInt`), and resistance outside 0..100 with the monster copy, which does not clamp.

REFERENCE: Rules.SpellDamage