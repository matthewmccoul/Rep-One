#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Roster {
    static cs::String PathFor(cs::String savesDir);
    static cs::List<cs::String> Records(cs::String savesDir);
    static void Write(cs::String savesDir, cs::List<cs::String> records);
    static bool Same(cs::String a, cs::String b);
    template <class T> static cs::List<T> Order(cs::String savesDir, cs::List<T> found, cs::Func<cs::String(T)> nameOf) {
        auto records = Records(savesDir);
        auto left = found.ToList();
        auto ordered = cs::List<T>::New();
        for (auto& r : records) {
            if (cs::Str::Trim(r) == "") continue;
            int32_t i = left.FindIndex([&](auto& x) { return Same(nameOf(x), r); });
            if (i < 0) continue;
            ordered.Add(left[i]);
            left.RemoveAt(i);
        }
        ordered.AddRange(left);
        return ordered;
    }
    static void Added(cs::String savesDir, cs::String name, cs::List<cs::String> existing);
    static cs::List<Tup_File_Ch_15a99> Saved(cs::String savesDir);
    static void Removed(cs::String savesDir, cs::String name, cs::List<cs::String> existing);
};

}  // namespace mc
