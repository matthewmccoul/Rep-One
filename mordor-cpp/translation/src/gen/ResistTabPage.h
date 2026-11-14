#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct ResistTabPage {
    static cs::List<CharStatFile_Piece> Paint(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item);
};

}  // namespace mc
