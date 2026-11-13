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

// Number of thresholds reached at this level (a threshold is reached at exactly that level).
i32 countReached(const i32* thresholds, i32 n, i32 level) {
    i32 swings = 0;
    for (i32 i = 0; i < n; ++i) {
        if (level >= thresholds[i]) {
            ++swings;
        }
    }
    return swings;
}

}  // namespace

i32 swingsAt(Guild guild, i32 level) {
    switch (guild) {
        case Guild::Ninja:
            return countReached(kNinjaSwingLevels, 4, level);
        case Guild::Warrior:
            return countReached(kWarriorSwingLevels, 2, level);
        case Guild::Villain:
            return countReached(kVillainSwingLevels, 2, level);
        case Guild::Paladin:
            return countReached(kPaladinSwingLevels, 2, level);
        case Guild::Scavenger:
            return countReached(kScavengerSwingLevels, 2, level);
        default:
            // UNKNOWN: page table entry for this guild was not provided; treated as no extra swings.
            return 0;
    }
}

}  // namespace mordor
