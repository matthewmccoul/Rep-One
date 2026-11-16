// Tiny test harness: TEST(name) registers a case; `<exe> <name>` runs one, `<exe> all` runs all.
#pragma once
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace dd_test {

struct Registry {
    std::map<std::string, std::function<void()>> cases;
    int failures = 0;
    static Registry& get() { static Registry r; return r; }
};

struct Registrar {
    Registrar(const char* name, std::function<void()> f) { Registry::get().cases[name] = f; }
};

inline void fail(const char* file, int line, const std::string& msg) {
    std::cerr << file << ":" << line << ": CHECK failed: " << msg << "\n";
    ++Registry::get().failures;
}

inline int runMain(int argc, char** argv) {
    Registry& r = Registry::get();
    if (argc < 2) { std::cerr << "usage: tests <case>|all|list\n"; return 2; }
    std::string which = argv[1];
    if (which == "list") { for (auto& c : r.cases) std::cout << c.first << "\n"; return 0; }
    int ran = 0;
    for (auto& c : r.cases) {
        if (which != "all" && which != c.first) continue;
        int before = r.failures;
        c.second();
        ++ran;
        std::cerr << (r.failures == before ? "  ok   " : "  FAIL ") << c.first << "\n";
    }
    if (ran == 0) { std::cerr << "no such test case: " << which << "\n"; return 2; }
    return r.failures ? 1 : 0;
}

}  // namespace dd_test

#define TEST(name) \
    static void test_##name(); \
    static dd_test::Registrar reg_##name(#name, test_##name); \
    static void test_##name()

#define CHECK(cond) do { if (!(cond)) dd_test::fail(__FILE__, __LINE__, #cond); } while (0)
#define CHECK_EQ(a, b) do { auto _va = (a); auto _vb = (b); if (!(_va == _vb)) { std::ostringstream _s; _s << #a << " == " << #b << " (got [" << _va << "] vs [" << _vb << "])"; dd_test::fail(__FILE__, __LINE__, _s.str()); } } while (0)
