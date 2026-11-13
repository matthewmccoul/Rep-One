#include "case_api.h"
#include "mordor/death3.h"

namespace mordor {

// Same-name copy in src/death3.cpp, renamed by the harness.
i32 raiseAgeCost__death3(DeathState cause);

// File-local copy in src/feat_rescue.cpp, reached through the shim export below.
i32 rc_export_feat_rescue(DeathState st);

namespace {

// Returns a DeathState value that is neither Stoned nor Rock (the "other" branch).
DeathState otherCause() {
    for (int v = 0;; ++v) {
        DeathState s = static_cast<DeathState>(v);
        if (s != DeathState::Stoned && s != DeathState::Rock) return s;
    }
}

// Args: 0 = Stoned, 1 = Rock, 2 = ordinary, 3 = other. Same mapping as Case.cs.
DeathState causeFrom(const JsonValue& a) {
    switch (A(a, "cause").asInt()) {
        case 0: return DeathState::Stoned;
        case 1: return DeathState::Rock;
        default: return otherCause();   // 2 (ordinary) and 3 (other) both take the default branch
    }
}

Str run_death3(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    return I(raiseAgeCost__death3(causeFrom(a)));
}

Str run_feat_rescue(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    return I(rc_export_feat_rescue(causeFrom(a)));
}

}  // namespace

const CopyEntry kCopies[] = {
    {"raiseAgeCost@death3", &run_death3},
    {"raiseAgeCost@feat_rescue", &run_feat_rescue},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
