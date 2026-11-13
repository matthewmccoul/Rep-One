#pragma once
#include "mordor/types.h"

namespace mordor {

// Gold amount from a give-command tag: VB CLng of a string (GiveCommand.cs:101, CLng).
// Returns false where the reference returns null (Type mismatch or Overflow).
bool goldFromTag(const Str& tag, i64& out);

}  // namespace mordor
