// firstFreeCompanionSlot has two declarations (companions1a.h on CompanionSlots&, companions2a.h on const Arr<Companion,4>&); only the first had a definition.
#include "mordor/companions2a.h"

namespace mordor {

// SRD ch.12 R2: a new companion fills the first hole; an empty slot has ref == -1. Returns -1 when all four slots are used.
i32 firstFreeCompanionSlot(const Arr<Companion, kCompanionSlots>& slots) {
  for (i32 i = 0; i < kCompanionSlots; ++i)
    if (slots[static_cast<size_t>(i)].ref < 0) return i;
  return -1;
}

}  // namespace mordor
