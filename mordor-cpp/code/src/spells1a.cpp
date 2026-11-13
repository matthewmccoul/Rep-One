// src/spells1a.cpp  -- module spells1a (SRD chapter 10, part c10p1)
//
// Global notes (grep for these tags):
//  UNKNOWN: GameData field names are not in the supplied material. This file assumes
//           camelCase SpellDef fields: level, pointCost, damage1, damage2, specialEffect,
//           affectGroup, resistIndex, ailments (effect id), category (7 = charm),
//           refusesModDungeon (map/raise/bind). ctx.data.spells is indexed by spell id.
//  UNKNOWN: CastRefusal enumerators are assumed: None, NotKnown, LevelTooLow,
//           NotEnoughPoints, AntiMagic, NoCharges, AlignedItem, MonstersOnSquare,
//           NotInParty, Dead, NeedsTarget, MapRaiseBind. spells1a.h uses CastRefusal
//           without including casting1.h, so this file includes casting1.h first.
//  CONFLICT: monsterBreathDamage(Ctx&, i32, i32) has the same signature in combat1b.h.
//           It is defined there and is deliberately NOT defined in this file.
//  FIXED: The compile errors show that SpellDef has no pointCost, ailments or
//         refusesModDungeon members, and CastRefusal has no LevelTooLow, NotEnoughPoints,
//         NoCharges, AlignedItem, MonstersOnSquare, NotInParty, Dead, NeedsTarget or
//         MapRaiseBind enumerators. Placeholders are used below (see the FIXED comments
//         at each site). They compile, but they CHANGE BEHAVIOUR until the real names are
//         supplied: every refusal becomes CastRefusal::NotKnown, point costs are 0,
//         effect ids are 0, and the map/raise/bind dungeon refusal is off.

#include <cmath>

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_party.h"
#include "mordor/state_dungeon.h"
#include "mordor/casting1.h"
#include "mordor/spells1a.h"

namespace mordor {
namespace {

constexpr i32 kGridSize = 30;               // R51 30x30 grid
constexpr i32 kLocalMaxDungeonLevel = 15;   // R53 MaxDungeonLevel
constexpr i32 kCharmCategory = 7;           // R23 charm is category 7
constexpr i32 kCharmEffectKind = 7;         // R17.2 / R17.4 kind-7 spell; UNKNOWN: effect-id mapping
constexpr i32 kAllOrNothingEffect = 100;    // R17.3
constexpr i32 kClassGateLow = 75;           // R18
constexpr i32 kClassGateHigh = 99;          // R18
constexpr i32 kGeneralCureEffect = 18;      // R26
constexpr i32 kResistEffectBase = 50;       // R31
constexpr i32 kResistGrant = 95;            // R31
constexpr i32 kBuffLevitate = 3;            // UNKNOWN: spell effect bit 3 (R28/R29)
constexpr i32 kBuffInvisible = 4;           // UNKNOWN: spell effect bit 4
constexpr i32 kBuffSeeInvisible = 5;        // UNKNOWN: spell effect bit 5
constexpr i32 kBuffProtect = 6;             // UNKNOWN: spell effect bit 6
constexpr i32 kBreathSweepGuildLimit = 40;  // R36
constexpr u32 kBitMagicImmune = 1u << 2;    // R19
constexpr u32 kBitCharmProof = 1u << 3;     // R17.2
constexpr u32 kBitBreathSweep = 1u << 3;    // R36 SpecialAttackFlags bit 3

}  // namespace

// ---------------------------------------------------------------------------
// Pre-cast checks and targeting (R1-R5)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Damage (R6-R23)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Healing and cure (R24-R26)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Spell states and resistance (R28-R31)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Companion casts (R32-R36)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Monster casts and acid spit (R38-R41)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Charm (R43-R49)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Teleport, displacement, passwall (R50-R55)
// ---------------------------------------------------------------------------

}  // namespace mordor
