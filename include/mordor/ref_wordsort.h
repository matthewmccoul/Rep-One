#pragma once
#include "mordor/types.h"

namespace mordor {

// LibrarySearch.cs:90 WordSort: lstrcmpi-style word sort. Hyphens and apostrophes are ignored, case is ignored, and names equal that way are ordered by the full strings (ordinal, ignore case). Returns -1, 0 or 1.
i32 wordSort(const Str& a, const Str& b);

}  // namespace mordor
