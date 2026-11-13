#include "case_api.h"
#include "mordor/state_party.h"

namespace mordor {

// Declarations of the copies (defined in src/ref_placeof.cpp and src/city2a.cpp).
// Declared here instead of including ref_placeof.h / city2a.h, which the harness cannot find.
bool placeOf(bool lostLevel, bool lostPosition, i32 level, i32 x, i32 y, Str& out);
bool libraryPlaceOf(const Party& party, Str& outPlace);

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
