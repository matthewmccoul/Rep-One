#pragma once
#include "mordor/types.h"

namespace mordor {

// Ports Rules.QuestItemBand (src/Mordor.Core/Rules.cs:1299): item quest band from quest tier, (int)(tier / 1.5) + 1.
i32 questItemBand(i32 tier);

}  // namespace mordor
