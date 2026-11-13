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
