#include <cstdio>
#include <cstring>
#include "harness.h"

int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : nullptr;
  int ran = 0;
  for (const auto& c : testh::registry()) {
    if (filter && !std::strstr(c.name, filter)) continue;
    testh::current() = c.name;
    int before = testh::failures();
    c.fn();
    ++ran;
    if (testh::failures() != before) std::printf("  -> %s FAILED\n", c.name);
  }
  std::printf("%d tests, %d checks, %d failures\n", ran, testh::checks(), testh::failures());
  return testh::failures() == 0 ? 0 : 1;
}
