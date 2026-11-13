I wrote the three files (plus the shim the harness needs). The inputs rest on two assumptions the outline doesn't confirm:

- `H.Dungeon[level]` returns the `Floor` for that level, using the same level value `isRockSquare` passes to `fieldAt`.
- `Floor` and `GameData` are the types the reference and C++ copy take, respectively.

If either is wrong, the build will fail or the sides will diverge at runtime, and I'd need the real definitions to fix it.

The reference returns rock (`1`) for any point outside the floor, while `isRockSquare` returns `0` when no field exists. The test reports that difference as-is. Level 1 is the only level used, because a level missing from the tables would make the C# reference throw, so the `fieldAt == nullptr` branch is not exercised.

=== FILE: cases.json ===
[
  {"args": {"level": 1, "x": 0, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 1, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 1}, "rng": [0.5]},
  {"args": {"level": 1, "x": 1, "y": 1}, "rng": [0.5]},
  {"args": {"level": 1, "x": 2, "y": 2}, "rng": [0.5]},
  {"args": {"level": 1, "x": 3, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 3}, "rng": [0.5]},
  {"args": {"level": 1, "x": 5, "y": 10}, "rng": [0.5]},
  {"args": {"level": 1, "x": 10, "y": 10}, "rng": [0.5]},
  {"args": {"level": 1, "x": 20, "y": 5}, "rng": [0.5]},
  {"args": {"level": 1, "x": 31, "y": 31}, "rng": [0.5]},
  {"args": {"level": 1, "x": 32, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 63, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 63}, "rng": [0.5]},
  {"args": {"level": 1, "x": 63, "y": 63}, "rng": [0.5]},
  {"args": {"level": 1, "x": 64, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 64}, "rng": [0.5]},
  {"args": {"level": 1, "x": 64, "y": 64}, "rng": [0.5]},
  {"args": {"level": 1, "x": 100, "y": 100}, "rng": [0.5]},
  {"args": {"level": 1, "x": 255, "y": 255}, "rng": [0.5]},
  {"args": {"level": 1, "x": -1, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": -1}, "rng": [0.5]},
  {"args": {"level": 1, "x": -1, "y": -1}, "rng": [0.5]},
  {"args": {"level": 1, "x": -2, "y": 5}, "rng": [0.5]},
  {"args": {"level": 1, "x": 5, "y": -100}, "rng": [0.5]},
  {"args": {"level": 1, "x": -1, "y": 64}, "rng": [0.5]},
  {"args": {"level": 1, "x": 64, "y": -1}, "rng": [0.5]},
  {"args": {"level": 1, "x": 1000, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 1000}, "rng": [0.5]},
  {"args": {"level": 1, "x": 1000, "y": 1000}, "rng": [0.5]},
  {"args": {"level": 1, "x": 2147483647, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 2147483647}, "rng": [0.5]},
  {"args": {"level": 1, "x": -2147483648, "y": 0}, "rng": [0.5]}
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
        int level = a.GetProperty("level").GetInt32();
        int x = a.GetProperty("x").GetInt32();
        int y = a.GetProperty("y").GetInt32();

        // ASSUMPTION: H.Dungeon is indexed directly by the level value.
        Floor floor = H.Dungeon[level];
        var nav = new DungeonNav(floor, x, y);
        bool rock = nav.IsRock(x, y);
        return (rock ? 1 : 0).ToString();
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"

namespace mordor {

// Defined in shim_dungeon1b_a.cpp, appended to src/dungeon1b_a.cpp.
bool export_isRockSquare(const GameData& data, i32 level, i32 x, i32 y);

static Str run_isRockSquare(const JsonValue& a, Ctx& ctx) {
    i32 level = static_cast<i32>(A(a, "level").asInt());
    i32 x = static_cast<i32>(A(a, "x").asInt());
    i32 y = static_cast<i32>(A(a, "y").asInt());
    bool rock = export_isRockSquare(ctx.data, level, x, y);
    return I(rock ? 1 : 0);
}

const CopyEntry kCopies[] = {
    {"isRockSquare", &run_isRockSquare},
};
const std::size_t kNumCopies = 1;

}  // namespace mordor
=== END FILE ===

=== FILE: shim_dungeon1b_a.cpp ===
namespace mordor {
bool export_isRockSquare(const GameData& data, i32 level, i32 x, i32 y) {
    return isRockSquare(data, level, x, y);
}
}  // namespace mordor
=== END FILE ===

UNREACHABLE: squareIsRock - the copy reads cellInState/sqAt, which index squares by a width that is not given anywhere (FloorState has no width field and FloorDef's layout is not shown), and SquareState/kBitRock are not defined in the provided material, so the input floor cannot be built.

REFERENCE: DungeonNav.IsRock