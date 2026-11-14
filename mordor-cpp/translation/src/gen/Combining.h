#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Combining_Result {
    cs::String Line{};
    bool Merged = false;
    bool Overflowed = false;

    Combining_Result() = default;
    Combining_Result(cs::String Line, bool Merged, bool Overflowed = false)
        : Line(Line), Merged(Merged), Overflowed(Overflowed) {}

    bool operator==(const Combining_Result& o) const {
        return Line == o.Line && Merged == o.Merged && Overflowed == o.Overflowed;
    }
    bool operator!=(const Combining_Result& o) const { return !(*this == o); }
};

struct Combining {
    using Result = Combining_Result;

    // C# returns null for "no refusal"; a C++ string cannot be null, so that case is "" here.
    static cs::String Refusal(cs::List<cs::Ref<InventorySlot>> chosen, cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::List<int32_t> slotNumbers);
    static bool CombinableKind(cs::Ref<Item> it);

    inline static const cs::String OverflowError = "Overflow";

    static cs::List<Combining_Result> Combine(cs::Ref<Character> c, cs::List<cs::Ref<InventorySlot>> chosen,
                                              cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::List<int32_t> slotNumbers);
};

}  // namespace mc
