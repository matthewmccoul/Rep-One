I'm writing the test against `Character.RestInCity`. The rule under test is the age cost, so the canonical output is the return value plus `Age` for both sides. Healing, ailment clearing and companion effects in the reference are excluded because no C++ copy models them. The reference has no `partyMany` parameter, so the `partyMany` cases compare the 1b copy against the reference as if the flag were absent.

=== FILE: cases.json ===
[
  {"args": {"curHits": 20, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 0, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 10, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 19, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 1, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 735, "maxHits": 800, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 787, "maxHits": 800, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 683, "maxHits": 800, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 387, "maxHits": 400, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 25, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 1, "maxHits": 1, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 0, "maxHits": 0, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 5, "maxHits": -10, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": -10, "maxHits": 20, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 1, "maxHits": 1000000, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 0, "maxHits": 1, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 1000000000, "maxHits": 2000000000, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 7, "maxHits": 9, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 0, "maxHits": 20, "age": 100, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 0, "maxHits": 20, "age": 5840, "partyMany": true}, "rng": [0.5]},
  {"args": {"curHits": 10, "maxHits": 20, "age": 5840, "partyMany": true}, "rng": [0.5]},
  {"args": {"curHits": 20, "maxHits": 20, "age": 5840, "partyMany": true}, "rng": [0.5]},
  {"args": {"curHits": 2, "maxHits": 3, "age": 5840, "partyMany": false}, "rng": [0.5]},
  {"args": {"curHits": 761, "maxHits": 800, "age": 5840, "partyMany": false}, "rng": [0.5]}
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
        int curHits = a.GetProperty("curHits").GetInt32();
        int maxHits = a.GetProperty("maxHits").GetInt32();
        int age = a.GetProperty("age").GetInt32();
        // partyMany is not a parameter of the reference; it is read by the copies only.

        var stats = new int[] { 10, 10, 10, 10, 10, 10 };
        var ch = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        ch.MaxHits = maxHits;
        ch.CurHits = curHits;
        ch.Age = age;

        int days = ch.RestInCity();

        // Only the age cost is compared: return value and Age.
        return $"{days}|{ch.Age}";
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/progression2.h"

namespace mordor {

// Renamed same-name copy from src/progression2.cpp.
i32 restInCity__progression2(Ctx& ctx, Character& ch);

// Export wrapper for the file-local copy in src/progression1b.cpp (see shim_progression1b.cpp).
i32 shimRestInCity1b(Ctx& ctx, Character& ch, bool partyMany);

static Str combine(i32 days, const Character& ch) {
    Str s = I(days);
    s += "|";
    s += I(ch.ageDays);
    return s;
}

static Str run_restInCity_progression2(const JsonValue& a, Ctx& ctx) {
    Character ch;
    ch.vitals.curHits = static_cast<i32>(A(a, "curHits").asInt());
    ch.vitals.maxHits = static_cast<i32>(A(a, "maxHits").asInt());
    ch.ageDays = static_cast<i32>(A(a, "age").asInt());
    const i32 days = restInCity__progression2(ctx, ch);
    return combine(days, ch);
}

static Str run_restInCity_progression1b(const JsonValue& a, Ctx& ctx) {
    Character ch;
    ch.vitals.curHits = static_cast<i32>(A(a, "curHits").asInt());
    ch.vitals.maxHits = static_cast<i32>(A(a, "maxHits").asInt());
    ch.ageDays = static_cast<i32>(A(a, "age").asInt());
    const bool partyMany = A(a, "partyMany").asBool();
    const i32 days = shimRestInCity1b(ctx, ch, partyMany);
    return combine(days, ch);
}

const CopyEntry kCopies[] = {
    {"restInCity@progression2", &run_restInCity_progression2},
    {"restInCity@progression1b", &run_restInCity_progression1b},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

=== FILE: shim_progression1b.cpp ===
namespace mordor { i32 shimRestInCity1b(Ctx& ctx, Character& ch, bool partyMany) { return restInCity__progression1b(ctx, ch, partyMany); } }
=== END FILE ===

REFERENCE: Character.RestInCity