#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_party.h"
#include "mordor/casting1.h"  // FIXED: SpellUse, CastRefusal, CastSituation and MonsterSpellBand are declared in casting1.h, which was not included.

namespace mordor {


struct SpellChargeResult {           // c21 item charge spend result
    bool removed = false;            // consumable removed when empty (R66)
    i32 chargesLeft = 0;
};

struct PortalStep {                  // c44 Ethereal Portal planned step
    i32 x = 0;
    i32 y = 0;
    bool intoRock = false;
};

// ---- Rule routines ----






// c1 Cast refusal order: SpellPoints, NonCombat, AntiMagic, CombatOnly, TooLongPeaced
// FIXED: SpellRefusalKind does not exist; using closest existing type CastRefusal.
// FIXED: CastCheckInputs does not exist; using closest existing type CastSituation.










// c62 Spell unreachable if spellLevel > ceiling
bool spellUnreachable(i32 spellLevel, i32 ceiling);






















// c21 Max SP = (natural INT + natural WIS) * 5
i32 maxSpellPoints(i32 naturalInt, i32 naturalWis);









// c21 Apply binding effect to a companion (150 sets binding to 100 and ForLife)
// UNKNOWN: increment size for effects 40..42.

















}  // namespace mordor
