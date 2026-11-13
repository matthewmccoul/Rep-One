// Basic types for the Mordor rules core. Everything the core needs from the platform lives in these few files
// (types.h, math_util.h, rng.h, events.h), so an Unreal port can swap them without touching the rules.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mordor {

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;

using Str = std::string;
template <class T> using Vec = std::vector<T>;
template <class T, std::size_t N> using Arr = std::array<T, N>;

// The six attributes, in the data file's order.
enum Stat : int { STR = 0, INT = 1, WIS = 2, CON = 3, CHA = 4, DEX = 5 };
constexpr int kNumStats = 6;

// Visual Basic Currency: a 64-bit integer in units of 1/10000 (four decimal places).
using Currency = i64;
constexpr Currency kCurrencyScale = 10000;

// Alignment of characters: 0 Good, 1 Neutral, 2 Evil (items use 1..3; see the SRD, chapters 1 and 12).
enum Alignment : int { ALIGN_GOOD = 0, ALIGN_NEUTRAL = 1, ALIGN_EVIL = 2 };

}  // namespace mordor
