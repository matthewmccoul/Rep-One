#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct ItemInfoPage {
    inline static cs::List<cs::String> AlignmentTags = cs::List<cs::String>::Of({"(u)", "(g)", "(n)", "(e)"});

    static constexpr int32_t StatNameRow = 6;
    static constexpr int32_t ReqRow = 7;
    static constexpr int32_t ModRow = 8;

    static bool IsItemPage(cs::List<CharStatFile_Piece> pieces);

    inline static cs::List<cs::String> HandWords = cs::List<cs::String>::Of(
        {"Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", "A Lot"});

    static cs::String VbSingle(float v);

    static cs::List<CharStatFile_Piece> Paint(cs::Ref<Item> it, ItemAlignment alignment, IdLevel known, cs::List<cs::Ref<Spell>> spells);
};

}  // namespace mc
