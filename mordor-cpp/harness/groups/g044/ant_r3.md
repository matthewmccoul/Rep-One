The link failed because the two copies are not externally visible from `case.cpp`. I added one shim per copy source file, appended to that file so it can reach file-local definitions. `case.cpp` now calls the shims instead of the global names. `Case.cs` and `cases.json` are unchanged apart from two added floor-boundary cases.

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
{"args": {"level": 1, "headerAreas": 246, "tally": 0}, "rng": [0.5]},
{"args": {"level": 1, "headerAreas": 247, "tally": 0}, "rng": [0.5]},
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

// Exported shims (defined in shim_ref_respawn.cpp and shim_monsters1b_2.cpp,
// appended to the end of their source files so file-local copies are reachable).
namespace mordor {
double shim_export_respawnDelay(std::int32_t level, std::int32_t headerAreas, std::int32_t tally);
std::int32_t shim_export_respawnDelaySeconds(std::int32_t level, std::int32_t areaCount, std::int32_t tally);

static Str run_respawnDelay(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    std::int32_t level = static_cast<std::int32_t>(A(a, "level").asInt());
    std::int32_t headerAreas = static_cast<std::int32_t>(A(a, "headerAreas").asInt());
    std::int32_t tally = static_cast<std::int32_t>(A(a, "tally").asInt());
    // Pure function: returns seconds as double.
    return F(shim_export_respawnDelay(level, headerAreas, tally));
}

static Str run_respawnDelaySeconds(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    std::int32_t level = static_cast<std::int32_t>(A(a, "level").asInt());
    std::int32_t areaCount = static_cast<std::int32_t>(A(a, "headerAreas").asInt());
    std::int32_t tally = static_cast<std::int32_t>(A(a, "tally").asInt());
    // Pure function: returns seconds as i32.
    return I(shim_export_respawnDelaySeconds(level, areaCount, tally));
}

const CopyEntry kCopies[] = {
    {"respawnDelay", &run_respawnDelay},
    {"respawnDelaySeconds", &run_respawnDelaySeconds},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===
=== FILE: shim_ref_respawn.cpp ===
namespace mordor {
double shim_export_respawnDelay(i32 level, i32 headerAreas, i32 tally) {
    return respawnDelay(level, headerAreas, tally);
}
}  // namespace mordor
=== END FILE ===
=== FILE: shim_monsters1b_2.cpp ===
namespace mordor {
i32 shim_export_respawnDelaySeconds(i32 level, i32 areaCount, i32 tally) {
    return respawnDelaySeconds(level, areaCount, tally);
}
}  // namespace mordor
=== END FILE ===

REFERENCE: DungeonState.RespawnDelay