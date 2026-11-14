#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CharacterOptions {
    bool AutoChangeLevel = true;
    bool Hide = false;
    bool PreloadCharacter = false;
    bool NoPerceptionMessages = false;
};

}  // namespace mc
