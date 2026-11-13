// Two modding-API routines that the header declares against placeholder types (ModTag/ModRef were substituted for
// types the specification never defined). SIMPLIFIED: neutral behaviour so the library links and mods keep their stored values.
#include "mordor/modding.h"

namespace mordor {

// SIMPLIFIED: the manifest has no toggle list in this header; report "not declared on".
bool declaredToggleDefault(const ModRef& /*manifest*/, const Str& /*key*/) { return false; }

// SIMPLIFIED: without the choice list the stored text is returned unchanged (SRD ch.21: declared defaults apply only when nothing is stored).
Str resolveChoice(const ModTag* /*spec*/, const Str& stored) { return stored; }

}  // namespace mordor
