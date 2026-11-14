#include "core.h"

namespace mc {

Tup_Outcome_Index_fb277 LibrarySearch::Find(cs::List<cs::String> list, cs::String typed) {
    int32_t count = 0, last = -1;
    for (int32_t i = 0; i < list.Count(); i++) {
        cs::String entry = list[i];
        cs::String start = (int32_t)entry.size() > (int32_t)typed.size() ? cs::Str::Substring(entry, 0, (int32_t)typed.size()) : entry;
        if (cs::Str::ToUpperInvariant(start) == cs::Str::ToUpperInvariant(typed)) {
            last = i;
            count++;
        }
    }
    if (count == 0) return Tup_Outcome_Index_fb277{Outcome::NoMatch, -1};
    if (count == 1) return Tup_Outcome_Index_fb277{Outcome::Selected, last};
    return Tup_Outcome_Index_fb277{Outcome::Ambiguous, last};
}

int32_t LibrarySearch_ListBoxOrder::Compare(cs::String a, cs::String b) {
    // OperatingSystem.IsWindows() + lstrcmpiW (P/Invoke) path not translated; falls back as the C# does when the DLL is missing
    return LibrarySearch::WordSort(a, b);
}

int32_t LibrarySearch_ListBoxOrder::lstrcmpiW(cs::String a, cs::String b) {
    (void)a; (void)b;
    throw cs::NotSupportedException("not translated: P/Invoke kernel32 lstrcmpiW");
}

int32_t LibrarySearch::WordSort(cs::String a, cs::String b) {
    auto Words = [&](const cs::String& s) { return cs::Str::Replace(cs::Str::Replace(s, "-", ""), "'", ""); };
    // InvariantCulture CompareInfo IgnoreCase approximated by case-insensitive ordinal compare
    int32_t c = cs::Str::CompareIgnoreCase(Words(a), Words(b));
    if (c != 0) return cs::Math::Sign(c);
    c = cs::Str::CompareIgnoreCase(a, b);
    return cs::Math::Sign(c);
}

}  // namespace mc
