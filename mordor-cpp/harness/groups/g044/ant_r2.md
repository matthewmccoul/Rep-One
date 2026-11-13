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
#include <cstdint>
#include "case_api.h"

// The two copies' headers (ref_respawn.h, monsters1b.h) are not on the include path,
// so their signatures are declared here exactly as the copies define them: at global
// scope, using the same fixed-width integer types as i32.
double respawnDelay(std::int32_t level, std::int32_t headerAreas, std::int32_t tally);
std::int32_t respawnDelaySeconds(std::int32_t level, std::int32_t areaCount, std::int32_t tally);

namespace mordor {

static Str run_respawnDelay(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    std::int32_t level = static_cast<std::int32_t>(A(a, "level").asInt());
    std::int32_t headerAreas = static_cast<std::int32_t>(A(a, "headerAreas").asInt());
    std::int32_t tally = static_cast<std::int32_t>(A(a, "tally").asInt());
    // Pure function: returns seconds as double.
    return F(::respawnDelay(level, headerAreas, tally));
}

static Str run_respawnDelaySeconds(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    std::int32_t level = static_cast<std::int32_t>(A(a, "level").asInt());
    std::int32_t areaCount = static_cast<std::int32_t>(A(a, "headerAreas").asInt());
    std::int32_t tally = static_cast<std::int32_t>(A(a, "tally").asInt());
    // Pure function: returns seconds as i32.
    return I(::respawnDelaySeconds(level, areaCount, tally));
}

const CopyEntry kCopies[] = {
    {"respawnDelay", &run_respawnDelay},
    {"respawnDelaySeconds", &run_respawnDelaySeconds},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

REFERENCE: DungeonState.RespawnDelay