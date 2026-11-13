#include "mordor/ref_swingsat.h"

namespace mordor {

namespace {

// UNKNOWN: thresholds below are taken only from the reference assertions for these five guilds.
// The full page table was not provided; other guilds return 0.
constexpr i32 kNinjaSwingLevels[] = {56, 122, 267, 585};
constexpr i32 kWarriorSwingLevels[] = {91, 324};
constexpr i32 kVillainSwingLevels[] = {105, 434};
constexpr i32 kPaladinSwingLevels[] = {111, 484};
constexpr i32 kScavengerSwingLevels[] = {116, 531};

}  // namespace

}  // namespace mordor
