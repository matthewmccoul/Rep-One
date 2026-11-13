=== FILE: cases.json ===
[
{"args": {"cause": 0}, "rng": [0.0]},
{"args": {"cause": 0}, "rng": [0.3]},
{"args": {"cause": 0}, "rng": [0.5]},
{"args": {"cause": 0}, "rng": [0.8]},
{"args": {"cause": 0}, "rng": [0.99999]},
{"args": {"cause": 1}, "rng": [0.0]},
{"args": {"cause": 1}, "rng": [0.3]},
{"args": {"cause": 1}, "rng": [0.5]},
{"args": {"cause": 1}, "rng": [0.8]},
{"args": {"cause": 1}, "rng": [0.99999]},
{"args": {"cause": 2}, "rng": [0.0]},
{"args": {"cause": 2}, "rng": [0.3]},
{"args": {"cause": 2}, "rng": [0.5]},
{"args": {"cause": 2}, "rng": [0.8]},
{"args": {"cause": 2}, "rng": [0.99999]},
{"args": {"cause": 3}, "rng": [0.0]},
{"args": {"cause": 3}, "rng": [0.3]},
{"args": {"cause": 3}, "rng": [0.5]},
{"args": {"cause": 3}, "rng": [0.8]},
{"args": {"cause": 3}, "rng": [0.99999]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // Causes: 0 = Stoned, 1 = Rock, 2 = ordinary (Rules.DeathOrdinary), 3 = any other int
    // that is neither Stoned nor Rock. The rule consumes no random draws.
    public static string Run(JsonElement a, Random rng)
    {
        int code = a.GetProperty("cause").GetInt32();
        int cause = code switch
        {
            0 => Rules.DeathStoned,
            1 => Rules.DeathRock,
            2 => Rules.DeathOrdinary,
            _ => OtherCause(),
        };
        int age = Raising.AgeDaysFor(cause);
        return age.ToString(CultureInfo.InvariantCulture);
    }

    private static int OtherCause()
    {
        int v = 0;
        while (v == Rules.DeathStoned || v == Rules.DeathRock) v++;
        return v;
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/death3.h"

namespace mordor {

// Same-name copy in src/death3.cpp, renamed by the harness.
i32 raiseAgeCost__death3(DeathState cause);

// File-local copy in src/feat_rescue.cpp, reached through the shim export below.
i32 rc_export_feat_rescue(DeathState st);

namespace {

// Returns a DeathState value that is neither Stoned nor Rock (the "other" branch).
DeathState otherCause() {
    for (int v = 0;; ++v) {
        DeathState s = static_cast<DeathState>(v);
        if (s != DeathState::Stoned && s != DeathState::Rock) return s;
    }
}

// Args: 0 = Stoned, 1 = Rock, 2 = ordinary, 3 = other. Same mapping as Case.cs.
DeathState causeFrom(const JsonValue& a) {
    switch (A(a, "cause").asInt()) {
        case 0: return DeathState::Stoned;
        case 1: return DeathState::Rock;
        default: return otherCause();   // 2 (ordinary) and 3 (other) both take the default branch
    }
}

Str run_death3(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    return I(raiseAgeCost__death3(causeFrom(a)));
}

Str run_feat_rescue(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    return I(rc_export_feat_rescue(causeFrom(a)));
}

}  // namespace

const CopyEntry kCopies[] = {
    {"raiseAgeCost@death3", &run_death3},
    {"raiseAgeCost@feat_rescue", &run_feat_rescue},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

=== FILE: shim_feat_rescue.cpp ===
namespace mordor { i32 rc_export_feat_rescue(DeathState st) { return raiseAgeCost__feat_rescue(st); } }
=== END FILE ===

REFERENCE: Raising.AgeDaysFor