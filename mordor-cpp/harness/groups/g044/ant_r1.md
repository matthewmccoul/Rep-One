=== FILE: cases.json ===
[
{"args": {"level": 4, "headerAreas": 10, "tally": 10}, "rng": [0.5]},
{"args": {"level": 4, "headerAreas": 10, "tally": 7}, "rng": [0.5]},
{"args": {"level": 4, "headerAreas": 10, "tally": 0}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 0, "tally": 0}, "rng": [0.5]},
{"args": {"level": 1, "headerAreas": 3, "tally": 3}, "rng": [0.5]},
{"args": {"level": 2, "headerAreas": 5, "tally": 5}, "rng": [0.5]},
{"args": {"level": 3, "headerAreas": 0, "tally": 0}, "rng": [0.5]},
{"args": {"level": 3, "headerAreas": 2, "tally": 1}, "rng": [0.5]},
{"args": {"level": 5, "headerAreas": 2, "tally": 9}, "rng": [0.5]},
{"args": {"level": 6, "headerAreas": 4, "tally": 4}, "rng": [0.5]},
{"args": {"level": 7, "headerAreas": 1, "tally": 0}, "rng": [0.5]},
{"args": {"level": 8, "headerAreas": 3, "tally": 3}, "rng": [0.5]},
{"args": {"level": 10, "headerAreas": 0, "tally": 0}, "rng": [0.5]},
{"args": {"level": 100, "headerAreas": 50, "tally": 10}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 216, "tally": 0}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 215, "tally": 0}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 217, "tally": 0}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 1, "tally": 0}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 3, "tally": 5}, "rng": [0.5]},
{"args": {"level": 0, "headerAreas": 0, "tally": 1000}, "rng": [0.5]},
{"args": {"level": -1, "headerAreas": 0, "tally": 0}, "rng": [0.5]},
{"args": {"level": -1, "headerAreas": 1000, "tally": 0}, "rng": [0.5]},
{"args": {"level": -2, "headerAreas": 5, "tally": 5}, "rng": [0.5]},
{"args": {"level": -20, "headerAreas": 0, "tally": 0}, "rng": [0.5]},
{"args": {"level": 2, "headerAreas": 100000, "tally": 0}, "rng": [0.5]},
{"args": {"level": 1000000, "headerAreas": 1, "tally": 1}, "rng": [0.5]},
{"args": {"level": 999999, "headerAreas": 0, "tally": 0}, "rng": [0.5]},
{"args": {"level": 2000000, "headerAreas": 2000000, "tally": 2000000}, "rng": [0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        int level = a.GetProperty("level").GetInt32();
        int headerAreas = a.GetProperty("headerAreas").GetInt32();
        int tally = a.GetProperty("tally").GetInt32();

        // Pure function: no Random draws and no state fields are changed.
        TimeSpan d = DungeonState.RespawnDelay(level, headerAreas, tally);
        return H.F(d.TotalSeconds);
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "ref_respawn.h"
#include "monsters1b.h"

namespace mordor {

static Str run_respawnDelay(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 level = static_cast<i32>(A(a, "level").asInt());
    i32 headerAreas = static_cast<i32>(A(a, "headerAreas").asInt());
    i32 tally = static_cast<i32>(A(a, "tally").asInt());
    // Pure function: returns seconds as double.
    return F(respawnDelay(level, headerAreas, tally));
}

static Str run_respawnDelaySeconds(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 level = static_cast<i32>(A(a, "level").asInt());
    i32 areaCount = static_cast<i32>(A(a, "headerAreas").asInt());
    i32 tally = static_cast<i32>(A(a, "tally").asInt());
    // Pure function: returns seconds as i32.
    return I(respawnDelaySeconds(level, areaCount, tally));
}

const CopyEntry kCopies[] = {
    {"respawnDelay", &run_respawnDelay},
    {"respawnDelaySeconds", &run_respawnDelaySeconds},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===
UNREACHABLE: respawnDelaySeconds(i32 dungeonLevel, i32 roomShortfall) -> i64 (monsters2b.h) - only the declaration is supplied, with no definition body and no statement of what roomShortfall means, so it cannot be fed the rule's (level, headerAreas, tally) inputs without guessing its semantics.

REFERENCE: DungeonState.RespawnDelay