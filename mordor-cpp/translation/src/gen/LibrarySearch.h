#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct LibrarySearch_ListBoxOrder {
    static cs::Ref<LibrarySearch_ListBoxOrder> Instance;

    int32_t Compare(cs::String a, cs::String b);

private:
    static int32_t lstrcmpiW(cs::String a, cs::String b);
};

inline cs::Ref<LibrarySearch_ListBoxOrder> LibrarySearch_ListBoxOrder::Instance = cs::New<LibrarySearch_ListBoxOrder>();

struct LibrarySearch {
    using Outcome = LibrarySearch_Outcome;
    using ListBoxOrder = LibrarySearch_ListBoxOrder;

    inline static const cs::String Title = "Search";
    inline static const cs::String NoMatch = "No match was found.";
    inline static const cs::String Ambiguous = "ambiguous!";

    static Tup_Outcome_Index_fb277 Find(cs::List<cs::String> list, cs::String typed);

    // OrderBy(name, ListBoxOrder.Instance): stable sort with the comparer
    template <class T> static cs::List<T> Sorted(cs::List<T> rows, cs::Func<cs::String(T)> name) {
        auto _r = rows.ToList();
        std::stable_sort(_r.Raw().begin(), _r.Raw().end(), [&](const T& a, const T& b) {
            return ListBoxOrder::Instance->Compare(name(a), name(b)) < 0;
        });
        return _r;
    }

    static int32_t WordSort(cs::String a, cs::String b);
};

}  // namespace mc
