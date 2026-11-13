Changes from the previous attempt:
- Added the in-bounds rock squares from the coordinator's list, plus non-rock squares on levels 2 and 3.
- Kept the out-of-bounds cases. Core returns true there and the C++ copy returns false, so that mismatch is real and is not adjusted.
- Fixed the floor lookup. The old code used `H.Dungeon[level]`, which picks the level-2 floor for level 1 if index 0 is level 1. The new code uses `level - 1`. The type outline has no Floor level field, so I could not verify this mapping. The rock cases will expose it: if the mapping is wrong, the reference returns 0 for the listed rock squares.
- Changed the navigator constructor to anchor at (0,0), so the constructor never receives out-of-bounds coordinates.
- Added the shim that the previous `case.cpp` referenced but never provided.
- `squareIsRock` is UNREACHABLE. It reads live `SquareState::bits` and `kBitRock`, and the outline does not show how those bits are populated from `FloorDef`. Building that state would mean inventing the mapping.

=== FILE: cases.json ===
[
  {"args": {"level": 1, "x": 10, "y": 5}, "rng": [0.5]},
  {"args": {"level": 1, "x": 11, "y": 5}, "rng": [0.5]},
  {"args": {"level": 1, "x": 12, "y": 5}, "rng": [0.5]},
  {"args": {"level": 1, "x": 13, "y": 5}, "rng": [0.5]},
  {"args": {"level": 2, "x": 7, "y": 3}, "rng": [0.5]},
  {"args": {"level": 2, "x": 8, "y": 3}, "rng": [0.5]},
  {"args": {"level": 2, "x": 9, "y": 3}, "rng": [0.5]},
  {"args": {"level": 3, "x": 7, "y": 0}, "rng": [0.5]},
  {"args": {"level": 3, "x": 4, "y": 1}, "rng": [0.5]},
  {"args": {"level": 3, "x": 5, "y": 1}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 1, "y": 1}, "rng": [0.5]},
  {"args": {"level": 1, "x": 5, "y": 10}, "rng": [0.5]},
  {"args": {"level": 1, "x": 20, "y": 5}, "rng": [0.5]},
  {"args": {"level": 2, "x": 0, "y": 0}, "rng": [0.5]},
  {"args": {"level": 2, "x": 1, "y": 1}, "rng": [0.5]},
  {"args": {"level": 2, "x": 2, "y": 2}, "rng": [0.5]},
  {"args": {"level": 2, "x": 15, "y": 10}, "rng": [0.5]},
  {"args": {"level": 3, "x": 0, "y": 0}, "rng": [0.5]},
  {"args": {"level": 3, "x": 1, "y": 1}, "rng": [0.5]},
  {"args": {"level": 3, "x": 2, "y": 2}, "rng": [0.5]},
  {"args": {"level": 3, "x": 15, "y": 10}, "rng": [0.5]},
  {"args": {"level": 1, "x": -1, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": -1}, "rng": [0.5]},
  {"args": {"level": 1, "x": -1, "y": -1}, "rng": [0.5]},
  {"args": {"level": 1, "x": 64, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 64}, "rng": [0.5]},
  {"args": {"level": 1, "x": 64, "y": 64}, "rng": [0.5]},
  {"args": {"level": 1, "x": 1000, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 0, "y": 1000}, "rng": [0.5]},
  {"args": {"level": 1, "x": -2147483648, "y": 0}, "rng": [0.5]},
  {"args": {"level": 1, "x": 2147483647, "y": 0}, "rng": [0.5]},
  {"args": {"level": 2, "x": -1, "y": 0}, "rng": [0.5]},
  {"args": {"level": 3, "x": 64, "y": 0}, "rng": [0.5]}
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

        // ASSUMPTION: H.Dungeon[0] is level 1, so level N maps to index N-1.
        // Not verified: the Floor type outline has no level field. The in-bounds
        // rock cases check this indirectly; a wrong mapping shows as reference 0.
        Floor floor = H.Dungeon[level - 1];

        // IsRock(x, y) ignores the navigator position, so anchor at the origin
        // and keep out-of-bounds query coordinates away from the constructor.
        var nav = new DungeonNav(floor, 0, 0);
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
UNREACHABLE: squareIsRock - it reads live SquareState::bits and kBitRock, but SquareState, kBitRock, and the rule for filling bits from FloorDef are not in the provided types, so a FloorState cannot be built without inventing them.
REFERENCE: DungeonNav.IsRock