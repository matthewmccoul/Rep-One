#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/attributes.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants and tables (SRD c09p3)

// R7 constant 1.9 in every cost formula (all sources agree)
inline constexpr double kCostFactor = 1.9;
// R33 engine MaxSP = (INT + WIS) * 5
inline constexpr i32 kMaxSpellPointMultiplier = 5;
// U16 stated spell level cap
inline constexpr i32 kSpellLevelCap = 255;
// R13 engine low-level divisor: VbInt(LevelFound - LevelFound / 3.0)
inline constexpr double kLowLevelDivisor = 3.0;
// R15 peaced casting limit: Combat spells allowed with monsters present but not engaged at level <= 7
inline constexpr i32 kPeacedCastLevelLimit = 7;
// R36 Set Sanctuary is spell level 1 in all guilds
inline constexpr i32 kSetSanctuarySpellLevel = 1;
// R18 protection base for Sorcerer and Wizard
// UNKNOWN: manual and archive say 3; Wizard sheet C = 15 (c79) disagrees; 3 used.
inline constexpr i32 kProtectionBaseSorWiz = 3;
// R29 engine: Life Domination effect value 150 sets ForLife
inline constexpr i32 kBindLifeDomination = 150;
// R29 archive: effect value 150 sets binding to 100
// CONFLICT: archive wording differs from engine; kept for reference, engine preferred.
inline constexpr i32 kBindArchiveBinding = 100;
// R20 Firebolt base level, engine MDATA2 value
// CONFLICT: Help and manual say 2; engine preferred.
inline constexpr i32 kFireboltBaseLevel = 1;
// R21 Blue Flame base level, engine MDATA2 value
// CONFLICT: Help and archive say 4; engine preferred.
inline constexpr i32 kBlueFlameBaseLevel = 3;
// R23 Cause Wounds level, engine MDATA2 value
// CONFLICT: Help and archive say 2; engine preferred.
inline constexpr i32 kCauseWoundsBaseLevel = 1;

// R14/R15 cast scope classification of a spell
enum class CastScope : i32 {
    Combat = 0,
    NonCombat = 1,
    Either = 2
};

// ---- cost formulas (R7 to R11)











// ---- spell level derivation (R5, R6, R37)





// ---- casting gates and charges (R1 to R4, R14 to R16, R25, R31, R32)








// R25 item spell stat gate: StatWithMods(i) >= Required[i] for every stat.
// UNKNOWN: whether this applies to general spells (U29); engine item gate used.





// ---- protection and binding (R17, R18, R29)




// ---- max SP (R33)

// R33 maximum spell points: (INT + WIS) * 5, using statWithMods.
// UNKNOWN: archive gives no formula (c107); engine formula used.
i32 maxSpellPoints(const Character& c);

// ---- monster low level (R13)


}  // namespace mordor
