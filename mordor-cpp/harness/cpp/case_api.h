// Contract for a group's case file (case.cpp): define kCopies[] and kNumCopies.
#pragma once
#include <cstddef>
#include <cstdio>
#include "mordor/ctx.h"
#include "mordor/events.h"
#include "mordor/game_data.h"
#include "mordor/json_lite.h"
#include "mordor/rng.h"
namespace mordor {
using CopyFn = Str (*)(const JsonValue& args, Ctx& ctx);
struct CopyEntry { const char* name; CopyFn fn; };
extern const CopyEntry kCopies[];
extern const std::size_t kNumCopies;
// Canonical formatting shared with the C# side (H.F): up to 6 decimals, no trailing zeros.
inline Str F(double d) { char b[64]; std::snprintf(b, sizeof b, "%.6f", d); Str s(b); while (!s.empty() && s.back() == '0') s.pop_back(); if (!s.empty() && s.back() == '.') s.pop_back(); if (s == "-0") s = "0"; return s; }
inline Str I(long long v) { return std::to_string(v); }
inline const JsonValue& A(const JsonValue& a, const char* k) { static const JsonValue n; const JsonValue* p = a.find(k); return p ? *p : n; }
}
