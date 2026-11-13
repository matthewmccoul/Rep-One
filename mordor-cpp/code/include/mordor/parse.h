#pragma once
// One decimal parser for typed numbers. The command handlers each had their own copy with its own
// limits; those limits are now arguments, so every handler keeps exactly the rule it had.
#include <cstdint>
#include "mordor/types.h"

namespace mordor {
// Digits only, with an optional leading '-' when allowNegative. maxChars > 0 rejects longer text
// (the '-' counts). maxMagnitude >= 0 rejects as soon as the value passes it; with -1 only i64
// overflow is rejected.
inline bool parseDecimal(const Str& text, i64& out, bool allowNegative, std::size_t maxChars, i64 maxMagnitude) {
    if (text.empty()) return false;
    if (maxChars > 0 && text.size() > maxChars) return false;
    std::size_t i = 0;
    bool neg = false;
    if (allowNegative && text[0] == '-') { neg = true; i = 1; }
    if (i >= text.size()) return false;
    i64 v = 0;
    for (; i < text.size(); ++i) {
        const char c = text[i];
        if (c < '0' || c > '9') return false;
        const i64 d = c - '0';
        if (maxMagnitude < 0 && v > (INT64_MAX - d) / 10) return false;
        v = v * 10 + d;
        if (maxMagnitude >= 0 && v > maxMagnitude) return false;
    }
    out = neg ? -v : v;
    return true;
}
}  // namespace mordor
