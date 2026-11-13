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
#include "ref_questband.h"
#include "guilds1.h"
#include "guilds3.h"

namespace mordor {

static Str run_questItemBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule takes no RNG
    i32 tier = static_cast<i32>(A(a, "tier").asInt());
    return I(questItemBand(tier));
}

static Str run_itemBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 tier = static_cast<i32>(A(a, "tier").asInt());
    return I(itemBand(tier));
}

static Str run_ItemQuestBand(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 tier = static_cast<i32>(A(a, "tier").asInt());
    return I(ItemQuestBand(tier));
}

const CopyEntry kCopies[] = {
    {"questItemBand", &run_questItemBand},
    {"itemBand", &run_itemBand},
    {"ItemQuestBand", &run_ItemQuestBand},
};
const std::size_t kNumCopies = sizeof(kCopies) / sizeof(kCopies[0]);

}  // namespace mordor
=== END FILE ===

REFERENCE: Mordor.Core.Rules.QuestItemBand