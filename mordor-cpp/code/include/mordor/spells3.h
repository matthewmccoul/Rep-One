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















// R4 Dependencies: magical resistance (T08) and aging (T05) are used by
// movement spells but are declared in their own modules, not here.
// R9 The damage formula is a reconstruction; no separate routine.
// R12 Turn order and caster-versus-target trigger: UNKNOWN, no routine.
// R13 Engine or test confirmation: none supplied, no routine.
// R14 Port behaviour: UNKNOWN, no routine.
// R15 Conflict between the "Real" label and observed vs calculated damage
//     (Blue Flame SL 255: 313 observed, 320 calculated). No routine.

}  // namespace mordor
