#pragma once

#include "mordor/types.h"

namespace mordor {

// Per-instance companion state. Static monster data stays in GameData.
// Companion.Acts: no companion-owned order state is stated (R2), so no field.

// R2: 4 companion slots per character.
constexpr i32 kCompanionSlotsPerCharacter = 4;
// R11: binding ceiling; 100 means ForLife.
constexpr i32 kBindingMax = 100;
// R35: chance to leave is clamped to 0..90.
constexpr i32 kChanceToLeaveMax = 90;
// UNKNOWN: SRD does not state the Stats array size; index 3 (CON) is used by stone and raise (R28, R30, R42).
constexpr i32 kCompanionStatCount = 4;

// Stored companion alignment, character scheme plus 1 (R5, R10).
// CONFLICT: character scheme is Good=0, Neutral=1, Evil=2 (c8, c9); the stored companion form is kept here.
enum class CompanionAlignment : i32 {
  Good = 1,
  Neutral = 2,
  Evil = 3
};

// Owner's identification level, original numbering (R71). Lowered by 1 on identify.
// c50: other values = Barely.
// CONFLICT: R24 says ID level <= 0 gives grade 4; not resolved here.
enum class OwnerIdLevel : i32 {
  Completely = 0,
  Mostly = 1,
  Partially = 2,
  Barely = 3
};

// Companion stoning state (c64). 2 = Stone.
// UNKNOWN: field name and value names for 0 and 1 are not stated.
enum class CompanionDeathState : i32 {
  Normal = 0,
  Dead = 1,
  Stone = 2
};

struct Companion {
  // Slot identity

  // R2, R3, R52, R67: creature table index (merged Index, MonsterIndex, Ref); -1 = empty slot / released.
  i32 ref = -1;
  // R41: slot holds a companion.
  bool occupied = false;
  // Slot index (0..kCompanionSlotsPerCharacter-1) within the owner's companion slots; released when its member is put down (R106, quirk c45).
  i32 slot = 0;
  // c4, R31: index of the bound Character in Party.Chars (merged Owner, BoundTo, CarriedBy); -1 = none.
  // CONFLICT: CarriedBy and Owner are merged; SRD gives both "party member carrying" and "owning character".
  i32 owner = -1;
  // R28: set when sub0930 releases the companion.
  bool released = false;
  // R55, R56: false after leave, defection, failed raise or stoning; true by default.
  bool present = true;
  // R16, R13: location of the companion when not carried.
  i32 area = 0;
  // R16: dungeon level where the companion lies.
  i32 dungeonLevel = 0;
  // c8: window/table selector, 0..UNKNOWN; 7 keeps window; above 7 maps to mode-7 (c24, c31).
  i32 spawnMode = 7;

  // Identity

  // R3, R4: creature Id; not unique (307 is shared). Distinct from ref.
  i32 monsterId = 0;
  // R52, R44, R45, R67: companion name. Blank on purchase; kept after release.
  // Old saves read blank " " (R67); "" is used here.
  Str name = "";
  // R5, R10, c9, c35, R13, R15, R29, R45: stored companion alignment.
  CompanionAlignment alignment = CompanionAlignment::Good;

  // Binding

  // c4, c21, R29: signed binding level (merged Binding, BindLevel, BindingLevel, BindingLevel).
  // Positive = bound (can turn); 0 or below = content / free will (R51, R53, c50, R67-R69).
  // 100 = ForLife (never drifts, leaves or is bought).
  // Range about -10..100 (R7, R8, R11). Default -10 for joiners.
  // CONFLICT: c24 and c97 say range 0..100 with UNKNOWN upper bound, and R61 says non-zero means bindable.
  // The more specific signed form is kept.
  i32 bindingLevel = -10;
  // c4, c21, R29: never released by Fear, never drifts or leaves (set by Life Domination, effect 150).
  bool forLife = false;
  // c4: bound to its character; released by Fear trap unless ForLife.
  // CONFLICT: bound may be derived from bindingLevel > 0; kept as a flag.
  bool bound = false;
  // c97: joined freely; Bind does not work on it.
  bool joinedFreely = false;

  // Health

  // R3, R22, R34, R36, R37, c5, c6, c30, c14: current hit points (merged Hits, CurHits).
  // 0 or less = dead and a carry candidate. Stoned release leaves this unchanged (c14).
  // CONFLICT: R34 and R22 say reaching 0 is death; c14 says stoned release leaves hits unchanged.
  i32 hits = 0;
  // R3, R34, R45, R51, R16: maximum hit points (merged MaxHits). Heal target for hits (R3).
  // UNKNOWN: max range and cap source not fully stated (R11, R17).
  i32 maxHits = 0;
  // c14, R16: death flag set by mdrcharSetDead only.
  // CONFLICT: R16 says companion death does not set IsDead; kept as separate flag.
  bool isDead = false;
  // c64: stoning state; see CompanionDeathState.
  CompanionDeathState deathState = CompanionDeathState::Normal;
  // R16: a raise is queued on this companion.
  bool pendingRaise = false;
  // R3, R16: poison level kept on the body.
  i32 poisonLevel = 0;
  // R28, R30, R42: creature stats. Index 0 used by blow, index 3 (CON) used by stone and raise.
  // Raise falls back to CON 10 when Stats[3] is missing (R16).
  Arr<i32, kCompanionStatCount> stats{};
  // R16, R32, c6: caster level source; caster level = clamp(Floor(guildLevel / 6), 1, 255).
  i32 guildLevel = 0;
  // R15, R35: companion level (compLvl), used in breath damage and leave chance.
  i32 level = 0;
  // R28: stored defence for companion targets.
  i32 def = 0;
  // R35: percent chance the companion leaves; clamped to 0..kChanceToLeaveMax.
  i32 chanceToLeave = 0;
  // R19, R34, R35: bit 2 = magic immunity (merged MagicImmune). Blocks spells.
  // UNKNOWN: caster-vs-target read for bit 2 (c6, c10).
  u32 specialPropertyFlags = 0;

  // Combat

  // c18, c104: true when the companion lands the blow; selects CompanionFactor 0.25 or 1.0.
  bool strikes = false;
  // R22, c14: slot the blow carries to, or -1; can point at an empty slot after release.
  // CONFLICT: c14 says held body or item reference; kept as slot index.
  i32 carry = -1;

  // Knowledge

  // R6, R17, c8: owner knowledge of this creature; 0 = all shown, higher = less shown; only sharpens.
  OwnerIdLevel idLevel = OwnerIdLevel::Completely;
};

}  // namespace mordor
