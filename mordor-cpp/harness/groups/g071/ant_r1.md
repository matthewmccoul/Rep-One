=== FILE: cases.json ===
[
  {"args": {"ageDays": 0, "raceLifespanYears": 70}, "rng": [0.5]},
  {"args": {"ageDays": 365, "raceLifespanYears": 70}, "rng": [0.5]},
  {"args": {"ageDays": 25550, "raceLifespanYears": 70}, "rng": [0.5]},
  {"args": {"ageDays": 18250, "raceLifespanYears": 100}, "rng": [0.5]},
  {"args": {"ageDays": 35770, "raceLifespanYears": 100}, "rng": [0.5]},
  {"args": {"ageDays": 35771, "raceLifespanYears": 100}, "rng": [0.5]},
  {"args": {"ageDays": 36500, "raceLifespanYears": 100}, "rng": [0.5]},
  {"args": {"ageDays": 36865, "raceLifespanYears": 100}, "rng": [0.5]},
  {"args": {"ageDays": 7300, "raceLifespanYears": 40}, "rng": [0.5]},
  {"args": {"ageDays": 1, "raceLifespanYears": 1}, "rng": [0.5]},
  {"args": {"ageDays": 365, "raceLifespanYears": 1}, "rng": [0.5]},
  {"args": {"ageDays": 730, "raceLifespanYears": 3}, "rng": [0.5]},
  {"args": {"ageDays": 365, "raceLifespanYears": 1073741823}, "rng": [0.5]},
  {"args": {"ageDays": 1, "raceLifespanYears": 1073741823}, "rng": [0.5]},
  {"args": {"ageDays": 2147483647, "raceLifespanYears": 1}, "rng": [0.5]},
  {"args": {"ageDays": 2147483647, "raceLifespanYears": 1073741823}, "rng": [0.5]},
  {"args": {"ageDays": 2147483647, "raceLifespanYears": 70}, "rng": [0.5]},
  {"args": {"ageDays": -1, "raceLifespanYears": 1}, "rng": [0.5]},
  {"args": {"ageDays": -365, "raceLifespanYears": 70}, "rng": [0.5]},
  {"args": {"ageDays": -2147483647, "raceLifespanYears": 70}, "rng": [0.5]},
  {"args": {"ageDays": 0, "raceLifespanYears": 0}, "rng": [0.5]},
  {"args": {"ageDays": 365, "raceLifespanYears": 0}, "rng": [0.5]},
  {"args": {"ageDays": 365, "raceLifespanYears": -1}, "rng": [0.5]},
  {"args": {"ageDays": 0, "raceLifespanYears": -5}, "rng": [0.5]},
  {"args": {"ageDays": 2147483647, "raceLifespanYears": 0}, "rng": [0.5]},
  {"args": {"ageDays": 2147483647, "raceLifespanYears": -2147483647}, "rng": [0.5]},
  {"args": {"ageDays": 12345, "raceLifespanYears": 33}, "rng": [0.5]},
  {"args": {"ageDays": 100, "raceLifespanYears": 365}, "rng": [0.5]},
  {"args": {"ageDays": 1, "raceLifespanYears": 1073741823}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, System.Random rng)
    {
        // The rule is not returned by any shown method. Rules.DiesOfOldAge computes
        // pct internally but returns a bool. Raising.ComplicationChance(ageDays, L2) returns
        // ageDays/365.0/(L2/2.0)*100. With L2 = 2*L (L > 0) this is exactly the rule's
        // pct, with the same operation order and bit-identical doubles. For L <= 0 the
        // reference returns 0 for any argument <= 0, so L is passed through unchanged.
        // 2*L must not overflow, so the test lifespans stay <= 1073741823.
        int ageDays = a.GetProperty("ageDays").GetInt32();
        int lifespan = a.GetProperty("raceLifespanYears").GetInt32();
        int arg = lifespan <= 0 ? lifespan : 2 * lifespan;
        double pct = Raising.ComplicationChance(ageDays, arg);
        return H.F(pct);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

// Same-name copies, renamed by the harness (signatures match the originals).
double oldAgePercent__death1b(i32 ageDays, i32 raceLifespanYears);
double oldAgePercent__death2b(const Character& ch, i32 raceLifespanYears);

static Str run_oldAgePercent_death1b(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // no rng draws consumed by this rule
    const i32 ageDays = static_cast<i32>(A(a, "ageDays").asInt());
    const i32 lifespan = static_cast<i32>(A(a, "raceLifespanYears").asInt());
    return F(oldAgePercent__death1b(ageDays, lifespan));
}

static Str run_oldAgePercent_death2b(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // no rng draws consumed by this rule
    const i32 ageDays = static_cast<i32>(A(a, "ageDays").asInt());
    const i32 lifespan = static_cast<i32>(A(a, "raceLifespanYears").asInt());
    Character ch{};
    ch.ageDays = ageDays;
    return F(oldAgePercent__death2b(ch, lifespan));
}

const CopyEntry kCopies[] = {
    {"oldAgePercent@death1b", &run_oldAgePercent_death1b},
    {"oldAgePercent@death2b", &run_oldAgePercent_death2b},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

REFERENCE: Raising.ComplicationChance (called with 2*raceLifespanYears for lifespan > 0, which yields the rule's pct exactly; Rules.DiesOfOldAge computes the same pct but returns only a bool)