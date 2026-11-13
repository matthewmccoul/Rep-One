The link failed because the three copies are defined in their own source files (`ref_questband.cpp`, `guilds1_b.cpp`, `guilds3.cpp`), and `case.cpp` alone doesn't pull those in. Each copy is a non-static global, so I can't rely on the harness linking those files for me. I added one shim per copy, which the harness appends to its copy's source file so the copy compiles with it. I also removed the global prototypes from `case.cpp`, since the shims now provide the entry points.

=== FILE: cases.json ===
[
{"args":{"tier":0},"rng":[0.5]},
{"args":{"tier":1},"rng":[0.5]},
{"args":{"tier":2},"rng":[0.5]},
{"args":{"tier":3},"rng":[0.5]},
{"args":{"tier":4},"rng":[0.5]},
{"args":{"tier":5},"rng":[0.5]},
{"args":{"tier":6},"rng":[0.5]},
{"args":{"tier":7},"rng":[0.5]},
{"args":{"tier":9},"rng":[0.5]},
{"args":{"tier":10},"rng":[0.5]},
{"args":{"tier":11},"rng":[0.5]},
{"args":{"tier":12},"rng":[0.5]},
{"args":{"tier":15},"rng":[0.5]},
{"args":{"tier":30},"rng":[0.5]},
{"args":{"tier":99},"rng":[0.5]},
{"args":{"tier":100},"rng":[0.5]},
{"args":{"tier":101},"rng":[0.5]},
{"args":{"tier":1000},"rng":[0.5]},
{"args":{"tier":12345},"rng":[0.5]},
{"args":{"tier":999999},"rng":[0.5]},
{"args":{"tier":1000000},"rng":[0.5]},
{"args":{"tier":2147483647},"rng":[0.5]},
{"args":{"tier":-1},"rng":[0.5]},
{"args":{"tier":-2},"rng":[0.5]},
{"args":{"tier":-3},"rng":[0.5]},
{"args":{"tier":-4},"rng":[0.5]},
{"args":{"tier":-5},"rng":[0.5]},
{"args":{"tier":-6},"rng":[0.5]},
{"args":{"tier":-7},"rng":[0.5]},
{"args":{"tier":-10},"rng":[0.5]},
{"args":{"tier":-100},"rng":[0.5]},
{"args":{"tier":-2147483647},"rng":[0.5]},
{"args":{"tier":-2147483648},"rng":[0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        // Rule takes no Random; rng is unused.
        int tier = a.GetProperty("tier").GetInt32();
        int band = Mordor.Core.Rules.QuestItemBand(tier);
        return band.ToString(System.Globalization.CultureInfo.InvariantCulture);
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"

namespace mordor {

// Exported by the shims appended to each copy's source file (see shim_*.cpp).
int mordor_export_questItemBand(int tier);
int mordor_export_itemBand(int tier);
int mordor_export_ItemQuestBand(int tier);

static Str run_questItemBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule takes no RNG
    int tier = static_cast<int>(A(a, "tier").asInt());
    return I(mordor_export_questItemBand(tier));
}

static Str run_itemBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    int tier = static_cast<int>(A(a, "tier").asInt());
    return I(mordor_export_itemBand(tier));
}

static Str run_ItemQuestBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    int tier = static_cast<int>(A(a, "tier").asInt());
    return I(mordor_export_ItemQuestBand(tier));
}

const CopyEntry kCopies[] = {
    {"questItemBand", &run_questItemBand},
    {"itemBand", &run_itemBand},
    {"ItemQuestBand", &run_ItemQuestBand},
};
const std::size_t kNumCopies = sizeof(kCopies) / sizeof(kCopies[0]);

}  // namespace mordor
=== END FILE ===
=== FILE: shim_ref_questband.cpp ===
namespace mordor { int mordor_export_questItemBand(int tier) { return ::questItemBand(tier); } }
=== END FILE ===
=== FILE: shim_guilds1_b.cpp ===
namespace mordor { int mordor_export_itemBand(int tier) { return ::itemBand(tier); } }
=== END FILE ===
=== FILE: shim_guilds3.cpp ===
namespace mordor { int mordor_export_ItemQuestBand(int tier) { return ::ItemQuestBand(tier); } }
=== END FILE ===

REFERENCE: Rules.QuestItemBand