// casting3.cpp: cost, level, gate and charge helpers from SRD part c09p3.
//
// NOT DEFINED HERE (CONFLICT): the API INDEX assigns these exact signatures to
// casting1.h / casting2.h, so they are defined by those modules:
//   - i32 spellLevelFromGuildLevel(i32 guildLevel)   (declared in casting1.h, casting2.h)
//   - void etherealPortalIntoRock(Ctx&, Party&, i32) (declared in casting2.h)
// This file calls spellLevelFromGuildLevel() but does not define it.

#include "mordor/casting3.h"
#include "mordor/math_util.h"
#include "mordor/attributes.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Spell cost products (R7, R10, R11)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Item and level-0 spell levels and charges (R4, R5, R6)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Refusal order and gates (R1, R2, R13 monster low-level)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Combat state and monsters present (R14, R15, R16)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Protection and binding (R17, R18, R29)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Max spell points (R33)
// ---------------------------------------------------------------------------

}  // namespace mordor
