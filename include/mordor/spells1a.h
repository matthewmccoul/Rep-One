#pragma once
// spells1a.h: SRD c10p1 Part A, rules R1-R55.
// Covers pre-cast checks, targeting, the shared damage shape, the group loop,
// healing and cure, party-wide encoding, spell states and resistance, caster
// levels, companion and monster special hits, charm, and teleport,
// displacement and passwall.
// CONFLICT: state_character.h, state_common.h, state_companion.h, state_item.h
// and state_dungeon.h have no include guards. This header includes state_party.h
// and state_dungeon.h, which both pull in state_character.h. Those files must be
// made idempotent (#pragma once) or this header will not compile.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_party.h"
#include "mordor/state_dungeon.h"
#include "mordor/casting1.h"  // FIXED: CastRefusal is declared in casting1.h; it was used without this include.

namespace mordor {

// ---- constants ----
inline constexpr i32 kPartyEffectBase = 200;        // R27 effect ids >= this apply party-wide
inline constexpr i32 kDamageGroupSlots = 4;         // R12 fixed group slots

// R4 dispatch route for an effect id
// UNKNOWN: Table A (effect id to route) is not included in this chunk.
enum class SpellRoute : i32 {
    Unhandled = 0,
    Damage,
    Heal,
    Cure,
    Buff,
    Resist,
    Charm,
    Teleport,
    Displace,
    Passwall,
    Fate
};

// R15-R17 verdict on one victim
enum class VictimVerdict : i32 {
    Hit = 0,
    MagicImmune,
    BlockedByCharmBit,
    AllOrNothingMiss,
    CharmSpared,
    ClassGateBlocked
};

// R53 outcome of a destination test
enum class ArrivalOutcome : i32 {
    FallsFromTheSky = 0,
    IntoRock,
    Arrives
};

// ---- small structs ----
// R32/R34/R38 caster description for the shared spell paths
struct SpellCasterInfo {
    i32 level = 1;             // SL: spell level used in the formulas (R32, R38 for companions and monsters)
    bool isCompanion = false;  // companion caster: R33 heals fail, R34 immunity test reads the caster
    bool magicImmune = false;  // R34 caster companion's bit 2 (companion casts only)
};

// R14 group-loop counters
struct DamageLoopCounters {
    i32 groupCount = 0;
    i32 step = 0;
};

// R12-R22 result of a damage spell over foe groups
struct DamageSpellOutcome {
    i32 groupsReached = 0;          // R20 one caption per group reached
    i32 victimsHit = 0;
    i32 targetMark = -1;            // R22 aimed group restored; -1 when nothing is left standing
    bool nothingDone = false;       // R20
    bool casterOrderToFight = false;  // R17.1 / R20
    bool itemOrderLost = false;     // R20
};

// R50 typed coordinates, relative to facing
struct TypedOffset {
    i32 right = 0;
    i32 forward = 0;
    i32 down = 0;
};

// R43-R48 result of a charm cast
struct CharmOutcome {
    bool refusedNoRoom = false;     // R43
    i32 charmedCount = 0;           // R46
    bool joinSkipped = false;       // R47 companion caster: creature vanishes, no join
    bool casterOrderToFight = false;  // R46 nothing charmed
};

// ---- declarations ----






























// R29 clear used by antimagic or blackout: SpellEffects = 0, spell resistance cleared, recompute
// UNKNOWN: spell-granted resistance (SpellResist) has no field in Resistances; resist[] is assumed.



// R31 resistance grant: kind = effect - 50; raise to 95 if lower, then recompute; repeat casts do not stack





























}  // namespace mordor
