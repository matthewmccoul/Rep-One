#include "mordor/ref_wordsort.h"

#include <cstddef>

namespace mordor {
namespace {

// UNKNOWN: the reference uses ICU invariant culture and OrdinalIgnoreCase for non-ASCII text.
// This port folds ASCII letters only; non-ASCII characters compare by their code unit.

bool isMark(char c) {
    // LibrarySearch.cs:90 Words(): Replace("-", "") and Replace("'", "")
    return c == '-' || c == '\'';
}

// Lowercase fold, used by the word-sort stage (symbols sort before letters, as in ICU).
i32 foldLower(char c) {
    i32 v = static_cast<i32>(c);
    if (v >= 'A' && v <= 'Z') v += ('a' - 'A');
    return v;
}

// Uppercase fold, used by the ordinal-ignore-case tie-break (as in OrdinalIgnoreCase).
i32 foldUpper(char c) {
    i32 v = static_cast<i32>(c);
    if (v >= 'a' && v <= 'z') v -= ('a' - 'A');
    return v;
}

// Compares a and b character by character under the chosen fold.
// If skipMarks is set, hyphens and apostrophes are skipped on both sides.
// Returns -1, 0 or 1.
i32 compareFolded(const Str& a, const Str& b, bool skipMarks, bool upper) {
    const std::size_t na = a.size();
    const std::size_t nb = b.size();
    std::size_t i = 0;
    std::size_t j = 0;
    for (;;) {
        if (skipMarks) {
            while (i < na && isMark(a[i])) ++i;
            while (j < nb && isMark(b[j])) ++j;
        }
        if (i >= na || j >= nb) break;
        const i32 ca = upper ? foldUpper(a[i]) : foldLower(a[i]);
        const i32 cb = upper ? foldUpper(b[j]) : foldLower(b[j]);
        if (ca != cb) return ca < cb ? -1 : 1;
        ++i;
        ++j;
    }
    const bool aDone = i >= na;
    const bool bDone = j >= nb;
    if (aDone && bDone) return 0;
    return aDone ? -1 : 1;
}

}  // namespace

i32 wordSort(const Str& a, const Str& b) {
    // LibrarySearch.cs:90 WordSort
    // Stage 1: word compare (marks removed, case ignored).
    i32 c = compareFolded(a, b, true, false);
    if (c != 0) return c;
    // Stage 2: ordinal compare ignoring case on the full strings.
    // Names that differ only in case compare equal (returns 0).
    return compareFolded(a, b, false, true);
}

// UNKNOWN: the Windows path (lstrcmpiW via CompareStringW in the thread locale) is not ported.
// The reference falls back to WordSort when that call is unavailable, so this port matches that fallback.

}  // namespace mordor
