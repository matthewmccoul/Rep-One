// A tiny self-contained test harness (no dependencies). TEST(name) { CHECK(cond); CHECK_EQ(a, b); }
#pragma once
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace testh {
struct Case { const char* name; void (*fn)(); };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& failures() { static int f = 0; return f; }
inline int& checks() { static int c = 0; return c; }
inline const char*& current() { static const char* c = ""; return c; }
struct Reg { Reg(const char* n, void (*f)()) { registry().push_back({n, f}); } };
template <class A, class B> bool eq(const A& a, const B& b) { return a == b; }
inline bool eq(double a, double b) { return std::fabs(a - b) < 1e-9; }
inline bool eq(const char* a, const char* b) { return std::string(a) == std::string(b); }
}  // namespace testh

#define TEST(name) \
  static void test_##name(); \
  static testh::Reg reg_##name(#name, &test_##name); \
  static void test_##name()

#define CHECK(cond) do { ++testh::checks(); if (!(cond)) { ++testh::failures(); std::printf("FAIL [%s] %s:%d: %s\n", testh::current(), __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { ++testh::checks(); if (!testh::eq((a), (b))) { ++testh::failures(); std::printf("FAIL [%s] %s:%d: %s == %s\n", testh::current(), __FILE__, __LINE__, #a, #b); } } while (0)
