#pragma once
#include "mordor/types.h"

namespace mordor {

// Reference: src/Mordor.Core/Librarian.cs:678 PlaceOf. Returns true and sets out to "x+1,y+1,level"
// when the lead is not lost and level > 0; otherwise returns false (the C# null).
bool placeOf(bool lostLevel, bool lostPosition, i32 level, i32 x, i32 y, Str& out);

}  // namespace mordor
