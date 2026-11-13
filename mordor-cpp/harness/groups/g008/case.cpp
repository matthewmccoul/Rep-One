#include "case_api.h"
#include "mordor/companions1b.h"
#include "mordor/companions2a.h"
#include "mordor/status1b.h"

namespace mordor {

// Copy driftContentStep (src/companions2a.cpp): takes (hits, maxHits).
static Str run_driftContentStep(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 hits = static_cast<i32>(A(a, "hits").asInt());
    const i32 maxHits = static_cast<i32>(A(a, "max_hits").asInt());
    return F(driftContentStep(hits, maxHits));
}

// Copy companionDriftContent (src/companions1b.cpp): takes (hits, maxHits).
static Str run_companionDriftContent(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 hits = static_cast<i32>(A(a, "hits").asInt());
    const i32 maxHits = static_cast<i32>(A(a, "max_hits").asInt());
    return F(companionDriftContent(hits, maxHits));
}

// Copy contentDrift (src/status1b.cpp): takes (missingHits, maxHits).
// The reference computes missing = MaxHits - Hits, so missing is derived the same way here.
static Str run_contentDrift(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const i32 hits = static_cast<i32>(A(a, "hits").asInt());
    const i32 maxHits = static_cast<i32>(A(a, "max_hits").asInt());
    const i32 missingHits = static_cast<i32>(static_cast<long long>(maxHits) - static_cast<long long>(hits));
    return F(contentDrift(missingHits, maxHits));
}

const CopyEntry kCopies[] = {
    {"driftContentStep", &run_driftContentStep},
    {"companionDriftContent", &run_companionDriftContent},
    {"contentDrift", &run_contentDrift},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor
