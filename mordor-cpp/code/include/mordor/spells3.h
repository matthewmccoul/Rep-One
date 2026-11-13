#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

namespace mordor {

// Forward declaration: FloorState lives in state_dungeon.h, which cannot be
// included here without a double definition of state_character.h.
struct FloorState;

// ---- Constants ----

// R2 Teleport non-combat range: up to 3 levels.
inline constexpr i32 kTeleportMaxLevels = 3;
// R5 Teleport combat range: up to 4 monsters in 4 groups.
inline constexpr i32 kTeleportCombatMaxMonsters = 4;
inline constexpr i32 kTeleportCombatMaxGroups = 4;

// ---- Enums and small structs ----

// R1 Movement and utility spells.
enum class MovementSpell : i32 {
    DetectRock = 0,
    FeatherEssence,
    EtherealPortal,
    Displacement,
    MassLevitate,
    Teleport,
    Sanctuary
};

// R1 Scope of a movement spell.
enum class MovementScope : i32 {
    NonCombat = 0,
    CombatAndNonCombat = 1
};

// R2 Result of one movement or utility cast.
enum class MovementOutcome : i32 {
    Applied = 0,
    NoEffect,      // R2 Sanctuary never set
    Failed,        // R2 Feather Essence levitation failed
    PartyKilled    // R2 Ethereal Portal into rock, Displacement outside/into rock
};

// R2 Caller-supplied choices for a movement cast.
struct MovementRequest {
    i32 casterIndex = 0;   // index into party.members
    i32 targetIndex = 0;   // Feather Essence recipient
    i32 levelDelta = 0;    // Teleport non-combat; clamped to +/- kTeleportMaxLevels
    i32 dx = 0;            // Displacement offset
    i32 dy = 0;            // Displacement offset
    // UNKNOWN: how the caller chooses levelDelta, dx, dy is not stated in the SRD.
};

// R2 Result of one movement or utility cast.
struct MovementCastResult {
    MovementOutcome outcome = MovementOutcome::Applied;
    bool rockBehind = false;   // R2 Detect Rock report
    bool levitated = false;    // R2 Feather Essence / Mass Levitate applied
    i32 levelsMoved = 0;       // R2 Teleport non-combat
    i32 agedDays = 0;          // R2 Sanctuary aging applied to caster
    bool sanctuaryReturned = false;
};

// R5 Result of Teleport in combat.
struct TeleportCombatResult {
    i32 sentAway = 0;     // monsters removed from the encounter
    i32 impaled = 0;      // monsters that were sent into rock
    i32 groupsHit = 0;    // groups that lost at least one monster
};

// R2 Table row: base value and combat scope per movement spell (c106).
struct MovementSpellRow {
    MovementSpell spell;
    i32 base;
    bool combatMarked;
};

// R2 Movement and utility spell table (source c106).
inline constexpr MovementSpellRow kMovementSpellRows[7] = {
    {MovementSpell::DetectRock, 1, false},
    {MovementSpell::FeatherEssence, 3, false},
    {MovementSpell::EtherealPortal, 5, false},
    {MovementSpell::Displacement, 8, false},
    {MovementSpell::MassLevitate, 8, false},
    {MovementSpell::Teleport, 12, true},
    {MovementSpell::Sanctuary, 12, false}
};

// R10 Spell granted by a crest (source c106).
enum class CrestSpell : i32 {
    None = 0,
    DispelUndead,
    Poison,
    Displacement,
    DispelMagicalLock,
    MagicalEntry,
    SoulDomination,
    PillarOfFire,
    DeadlyHarm,
    Restoration
};

// R10 One crest grant row: crest, spell and fixed charges.
struct CrestGrantRow {
    Guild crest;
    CrestSpell spell;
    i32 charges;
};

// R10 Crest grant table (source c106).
// CONFLICT: source lists "Magi" and "Journeyman"; the Guild enum has Mage and no
// Journeyman. Magi is mapped to Guild::Mage. Journeyman has no Guild entry and
// is omitted; it grants no spell in any case.
// CONFLICT: source writes "Dispell Undead"; the c83 spell name is "Dispel Undead".
// Both are the same spell.
inline constexpr CrestGrantRow kCrestGrants[9] = {
    {Guild::Paladin, CrestSpell::DispelUndead, 12},
    {Guild::Villain, CrestSpell::Poison, 6},
    {Guild::Seeker, CrestSpell::Displacement, 6},
    {Guild::Thief, CrestSpell::DispelMagicalLock, 6},
    {Guild::Scavenger, CrestSpell::MagicalEntry, 6},
    {Guild::Mage, CrestSpell::SoulDomination, 6},
    {Guild::Sorcerer, CrestSpell::PillarOfFire, 6},
    {Guild::Wizard, CrestSpell::DeadlyHarm, 6},
    {Guild::Healer, CrestSpell::Restoration, 6}
};

// R10 Result of a crest spell cast.
struct CrestCastResult {
    bool cast = false;
    CrestSpell spell = CrestSpell::None;
    i32 charges = 0;
    i32 spellLevel = 0;   // caster's current spell level used for the cast
};

// ---- Rule routines ----

// R1 Scope of a movement spell (non-combat unless marked).
MovementScope movementScopeOf(MovementSpell spell);

// R2 Base value of a movement spell, read from kMovementSpellRows.
i32 movementSpellBase(MovementSpell spell);

// R3 Stat requirements per movement spell.
// UNKNOWN: the SRD says they are listed per spell but gives none. The
// implementation must return true until data is supplied.
bool meetsMovementStatRequirements(const Character& caster, MovementSpell spell);

// R2 Apply one movement or utility spell to the party and its floor.
// Ethereal Portal and Displacement may kill the party (outcome PartyKilled);
// the death state is written to each member by the implementation.
// Feather Essence levitation may fail; the failure chance is UNKNOWN.
MovementCastResult castMovementSpell(Ctx& ctx,
                                     Party& party,
                                     FloorState& floor,
                                     const MovementRequest& req,
                                     MovementSpell spell,
                                     const Location& sanctuary,
                                     bool sanctuarySet);

// R5 Chance (percent) that a Teleport combat effect sends a monster into rock.
// UNKNOWN: the formula relating creature magical resistance to effectiveness is
// not given. Simplest sensible value: clampv(100 - magicResistance, 0, 100).
i32 teleportImpalePercent(i32 creatureMagicResistance);

// R5 Teleport in combat: sends up to kTeleportCombatMaxMonsters monsters in up to
// kTeleportCombatMaxGroups groups away, impaling them in rock.
// UNKNOWN: group and monster selection order is not stated.
TeleportCombatResult teleportCombatSendAway(Ctx& ctx, Encounter& encounter);

// R6 Damage Modifier1 = Base + Base*LN(SL+1)/LN(5) + Int((SL/10)*(Power-1)).
// Pure arithmetic. The SL/10 term is evaluated as real division before Int.
double damageModifier1(i32 base, i32 sl, i32 power);

// R6 Damage Modifier2 = Int(Rnd*(Modifier1-((SL-1)/SL^2))/2)+1.
// Draws Rnd from ctx.rng.nextDouble(). Rnd range is UNKNOWN.
// UNKNOWN: behaviour at SL 0, where (SL-1)/SL^2 is undefined. The implementation
// treats that term as 0.
i32 spellModifier2(Ctx& ctx, double modifier1, i32 sl);

// R6 Full damage resolution, steps 1 to 4. Returns Int(Damage*(1-Res/100)).
// CONFLICT: the rounding of the resistance-scaled damage is UNKNOWN; the
// implementation truncates with vbInt.
// UNKNOWN: whether SL is capped at kSpellLevelCap before use (R8).
i32 spellDamage(Ctx& ctx, i32 base, i32 sl, i32 power, i32 targetResistance);

// R7 Healing Modifier1 = Base + Base*LN(SL)/LN(10) + Int((SL/10)*(Power-1)).
// Pure arithmetic. Uses the healing log term; there is no resistance step.
double healingModifier1(i32 base, i32 sl, i32 power);

// R7 Full healing resolution, steps 1 to 3. Returns Int(Modifier1-Modifier2).
// Uses the same Modifier2 as R6.
i32 spellHealing(Ctx& ctx, i32 base, i32 sl, i32 power);

// R8 Spell level used in the formulas: clamps to 0..kSpellLevelCap.
i32 effectiveSpellLevel(i32 sl);

// R10 Crest grant for a guild crest. Returns spell None and charges 0 when the
// crest grants no spell (Journeyman, Warrior, Ninja, Nomad).
CrestGrantRow crestGrantFor(Guild crest);

// R10 Cast the spell granted by a crest at the caster's spell level, with the
// charges listed in kCrestGrants.
// UNKNOWN: whether crest spells use a separate SL (R11). The implementation uses
// caster.spellLevel.
CrestCastResult castCrestSpell(Ctx& ctx, const Character& caster, Guild crest);

// R4 Dependencies: magical resistance (T08) and aging (T05) are used by
// movement spells but are declared in their own modules, not here.
// R9 The damage formula is a reconstruction; no separate routine.
// R12 Turn order and caster-versus-target trigger: UNKNOWN, no routine.
// R13 Engine or test confirmation: none supplied, no routine.
// R14 Port behaviour: UNKNOWN, no routine.
// R15 Conflict between the "Real" label and observed vs calculated damage
//     (Blue Flame SL 255: 313 observed, 320 calculated). No routine.

}  // namespace mordor
