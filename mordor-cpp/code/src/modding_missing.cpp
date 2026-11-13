// Implements the two modding functions that were declared in modding.h but never defined.
// Source: SRD c21p1 Formula F1 (manifest problem text), Formula F2 (display title), R20, R21.
#include "mordor/modding.h"

namespace mordor {


// R21, F2: Title = (IsBlank(Name) ? Id : Name) + (Version == "" ? "" : " " + Version)
// FIXED: ModRef has no `name` member (compiler error), so the Name branch cannot be read.
// The closest existing equivalent is the `id` field, so the title is Id + optional Version.
// ASSUMED: ModRef exposes string fields `id` and `version`, and Str supports empty() and operator+.
Str modDisplayTitle(const ModRef& manifest) {
    const Str& base = manifest.id;
    if (manifest.version.empty()) {
        return base;
    }
    return base + Str(" ") + manifest.version;
}

}  // namespace mordor
