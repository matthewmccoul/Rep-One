#include <cstdint>
#include "case_api.h"

// Exported shims (defined in shim_ref_respawn.cpp and shim_monsters1b_2.cpp,
// appended to the end of their source files so file-local copies are reachable).
namespace mordor {
double shim_export_respawnDelay(std::int32_t level, std::int32_t headerAreas, std::int32_t tally);
std::int32_t shim_export_respawnDelaySeconds(std::int32_t level, std::int32_t areaCount, std::int32_t tally);

static Str run_respawnDelay(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    std::int32_t level = static_cast<std::int32_t>(A(a, "level").asInt());
    std::int32_t headerAreas = static_cast<std::int32_t>(A(a, "headerAreas").asInt());
    std::int32_t tally = static_cast<std::int32_t>(A(a, "tally").asInt());
    // Pure function: returns seconds as double.
    return F(shim_export_respawnDelay(level, headerAreas, tally));
}

static Str run_respawnDelaySeconds(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    std::int32_t level = static_cast<std::int32_t>(A(a, "level").asInt());
    std::int32_t areaCount = static_cast<std::int32_t>(A(a, "headerAreas").asInt());
    std::int32_t tally = static_cast<std::int32_t>(A(a, "tally").asInt());
    // Pure function: returns seconds as i32.
    return I(shim_export_respawnDelaySeconds(level, areaCount, tally));
}

const CopyEntry kCopies[] = {
    {"respawnDelay", &run_respawnDelay},
    {"respawnDelaySeconds", &run_respawnDelaySeconds},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
