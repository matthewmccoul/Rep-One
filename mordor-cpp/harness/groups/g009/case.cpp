#include "case_api.h"
#include "mordor/companions1b.h"
#include "mordor/companions2a.h"
#include "mordor/status1b.h"

namespace mordor {

static Str run_companionDriftBound(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const int guild = A(a, "guild_level").asInt();
    const int top = A(a, "top_guild_level").asInt();
    const int binding = A(a, "binding_level").asInt();
    return F(companionDriftBound(guild, top, binding));
}

static Str run_driftBoundStep(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const int guild = A(a, "guild_level").asInt();
    const int top = A(a, "top_guild_level").asInt();
    const int binding = A(a, "binding_level").asInt();
    return F(driftBoundStep(guild, top, binding));
}

static Str run_boundDrift(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const int guild = A(a, "guild_level").asInt();
    const int top = A(a, "top_guild_level").asInt();
    const int binding = A(a, "binding_level").asInt();
    return F(boundDrift(guild, top, binding));
}

const CopyEntry kCopies[] = {
    {"companionDriftBound", &run_companionDriftBound},
    {"driftBoundStep", &run_driftBoundStep},
    {"boundDrift", &run_boundDrift},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor
