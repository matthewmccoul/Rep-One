I wrote the three files below. The C++ copies are declared in headers, so no shim files were needed.

=== FILE: cases.json ===
[
{"args":{"lostLevel":false,"lostPosition":false,"level":1,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":15,"x":10,"y":5},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":0,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":-1,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":-5,"x":3,"y":3},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":2,"x":-1,"y":-1},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":3,"x":-2,"y":-3},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":1,"x":-100,"y":100},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":15,"x":1000,"y":1000},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":2147483647,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":1,"x":2147483646,"y":2147483646},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":false,"level":1,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":false,"level":15,"x":5,"y":5},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":true,"level":1,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":true,"level":7,"x":2,"y":2},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":true,"level":1,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":true,"level":0,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":false,"level":0,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":true,"level":-1,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":false,"level":-1,"x":-1,"y":-1},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":1,"x":1,"y":1},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":14,"x":-1,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":1,"x":0,"y":-1},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":2,"x":-1,"y":5},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":true,"level":-100,"x":-100,"y":-100},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":-2147483648,"x":0,"y":0},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":true,"level":15,"x":-1,"y":-1},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":5,"x":-1000,"y":-1000},"rng":[0.5]},
{"args":{"lostLevel":false,"lostPosition":false,"level":8,"x":7,"y":-7},"rng":[0.5]},
{"args":{"lostLevel":true,"lostPosition":false,"level":3,"x":1,"y":1},"rng":[0.5]}
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
        bool lostLevel = a.GetProperty("lostLevel").GetBoolean();
        bool lostPosition = a.GetProperty("lostPosition").GetBoolean();
        int level = a.GetProperty("level").GetInt32();
        int x = a.GetProperty("x").GetInt32();
        int y = a.GetProperty("y").GetInt32();

        string? place = Librarian.PlaceOf(lostLevel, lostPosition, level, x, y);
        // Pure function: no state fields change. Return value is ok flag plus place string (or "null").
        return $"{(place != null ? 1 : 0)}|{place ?? "null"}";
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_party.h"
#include "ref_placeof.h"
#include "city2a.h"

namespace mordor {

static Str run_placeOf(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    bool lostLevel = A(a, "lostLevel").asBool();
    bool lostPosition = A(a, "lostPosition").asBool();
    i32 level = static_cast<i32>(A(a, "level").asInt());
    i32 x = static_cast<i32>(A(a, "x").asInt());
    i32 y = static_cast<i32>(A(a, "y").asInt());

    Str out;
    bool ok = placeOf(lostLevel, lostPosition, level, x, y, out);
    // Pure function: no state fields change. Return value is ok flag plus place string (or "null").
    return I(ok ? 1 : 0) + "|" + (ok ? out : Str("null"));
}

static Str run_libraryPlaceOf(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    bool lostLevel = A(a, "lostLevel").asBool();
    bool lostPosition = A(a, "lostPosition").asBool();
    i32 level = static_cast<i32>(A(a, "level").asInt());
    i32 x = static_cast<i32>(A(a, "x").asInt());
    i32 y = static_cast<i32>(A(a, "y").asInt());

    Party party;
    party.lostLevel = lostLevel;
    party.lostPosition = lostPosition;
    party.dungeonLevel = level;  // level maps to Party::dungeonLevel
    party.x = x;
    party.y = y;

    Str out;
    bool ok = libraryPlaceOf(party, out);
    return I(ok ? 1 : 0) + "|" + (ok ? out : Str("null"));
}

const CopyEntry kCopies[] = {
    {"placeOf", &run_placeOf},
    {"libraryPlaceOf", &run_libraryPlaceOf},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

Notes:
- The reference never takes a `Random`, so every case uses `rng: [0.5]`, which is never consumed.
- I left out inputs where `x + 1` or `y + 1` overflows `int`. C# wraps silently, but signed overflow is undefined behaviour in C++, so the two sides would not be comparable there. The largest values tested are `2147483646` (which gives `2147483647` with no overflow) and `-2147483648` for `level`.
- The header paths `ref_placeof.h`, `city2a.h`, and `mordor/state_party.h` follow the copy declarations in the spec. The C++ accessors `asBool()` and `asInt()` are assumed from the spec's `asInt()` example; check them against `json_lite.h`.

REFERENCE: Librarian.PlaceOf