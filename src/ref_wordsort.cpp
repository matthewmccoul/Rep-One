#include "mordor/ref_wordsort.h"

#include <cstddef>

namespace mordor {
namespace {

// UNKNOWN: the reference uses ICU invariant culture and OrdinalIgnoreCase for non-ASCII text.
// This port folds ASCII letters only; non-ASCII characters compare by their code unit.

}  // namespace

// UNKNOWN: the Windows path (lstrcmpiW via CompareStringW in the thread locale) is not ported.
// The reference falls back to WordSort when that call is unavailable, so this port matches that fallback.

}  // namespace mordor
