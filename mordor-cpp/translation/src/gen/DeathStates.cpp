#include "core.h"

namespace mc {

cs::String DeathStates::Name(int32_t causeOfDeath) {
    int32_t i = cs::Math::Max(1, causeOfDeath) - 1;
    return i >= 0 && i < Names.Length() ? Names[i] : Names[0];
}

}  // namespace mc
