// dataformatsa_a.cpp: character load/roster, option read/write, preload key,
// guild log append/encode/decode (SRD c20p1 R1-R6, R4-R5).
//
// Assumptions, not verified against types.h / ctx.h / events.h / game_data.h:
//   Str behaves like std::string (size, empty, ==, push_back, substr, operator[]).
//   Vec<T> behaves like std::vector<T>.
//   ctx.say(code, text, nums, strs) accepts {} for nums and strs.
//   GameData::races and GameData::guilds support size().
//
// UNKNOWN: the MDATA4 field layout (2900-byte record) and the MDATA9 guild log
//   record layout are not in the SRD half supplied. See the notes per function.

#include "mordor/dataformatsa.h"

namespace mordor {
namespace {

u32 getU32LE(const Vec<u8>& b, decltype(b.size()) pos) {
    u32 v = 0;
    for (i32 k = 0; k < 4; ++k) {
        v |= static_cast<u32>(b[pos + static_cast<decltype(pos)>(k)]) << (8 * k);
    }
    return v;
}

u64 getU64LE(const Vec<u8>& b, decltype(b.size()) pos) {
    u64 v = 0;
    for (i32 k = 0; k < 8; ++k) {
        v |= static_cast<u64>(b[pos + static_cast<decltype(pos)>(k)]) << (8 * k);
    }
    return v;
}

}  // namespace

}  // namespace mordor
