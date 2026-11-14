#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionInfo {
    static constexpr int32_t KnowAll = 0;
    static constexpr int32_t MoreToLearn = 1;
    static constexpr int32_t LotsMoreToLearn = 2;

    static cs::String IdLevelText(int32_t idLevel);
    static cs::List<CharStatFile_Piece> Paint(cs::Ref<Companion> c, int32_t index, cs::Ref<Monster> def, cs::Ref<Character> owner,
                                              cs::Func<cs::Ref<Item>(int16_t)> item, Alignment storeAlignment = Alignment::Good);
    static void Sharpen(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Func<cs::Ref<Item>(int16_t)> item);
    static bool Bit(int64_t mask, int32_t b);
    static int32_t AlignmentOf(cs::Ref<Companion> c, cs::Ref<Monster> def);
    static double LeaveChance(cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Ref<Character> owner, cs::Func<cs::Ref<Item>(int16_t)> item);
};

}  // namespace mc
