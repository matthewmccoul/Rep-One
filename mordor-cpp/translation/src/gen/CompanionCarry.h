#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionCarry {
    static cs::List<cs::String> Run(cs::Ref<Character> who, bool inCommand, cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> monsterOf);
};

}  // namespace mc
