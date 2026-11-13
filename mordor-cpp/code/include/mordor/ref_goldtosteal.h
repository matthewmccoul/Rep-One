#pragma once
#include "mordor/types.h"

namespace mordor {

// Ref: CombatEngine.GoldToSteal (src/Mordor.Core/MonsterAttack.cs:210). Returns false where the reference returns null (Gold - rolled overflows Currency).
bool goldToSteal(Currency onHand, i64 rolled, i64& taken);

}  // namespace mordor
