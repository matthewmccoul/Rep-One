#pragma once
// One table of guild names (the order of the Guild enum). Callers that want lower case lower it.
#include "mordor/types.h"

namespace mordor {
inline const char* guildNameOrNull(i32 guildIndex) {
    static const char* const kNames[] = {"Nomad", "Warrior", "Paladin", "Ninja", "Villain", "Seeker",
                                         "Thief", "Scavenger", "Mage", "Sorcerer", "Wizard", "Healer"};
    return (guildIndex >= 0 && guildIndex < static_cast<i32>(sizeof(kNames) / sizeof(kNames[0]))) ? kNames[guildIndex] : nullptr;
}
}  // namespace mordor
