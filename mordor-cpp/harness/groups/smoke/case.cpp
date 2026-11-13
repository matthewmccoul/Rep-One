#include "case_api.h"
namespace mordor {
static Str one(const JsonValue& a, Ctx& c) { long long x = A(a,"a").asInt(), y = A(a,"b").asInt(); return I(x + y) + "|" + I(c.rng.below(10)) + "|" + F(x / 3.0) + "|" + I((long long)c.data.items.size()); }
const CopyEntry kCopies[] = {{"one", &one}};
const std::size_t kNumCopies = 1;
}
