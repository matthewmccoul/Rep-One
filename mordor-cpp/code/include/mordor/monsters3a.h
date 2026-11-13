#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

inline constexpr i32 kRespawnSecondsPerLevel = 150;      // R3 150 s per level
inline constexpr i32 kRespawnEmptyAreaPenaltySec = 5;    // R4 5 s per empty area below header count
inline constexpr i64 kRoomUnsetStamp = 0;                // R8 stamp 0 reads as never set
inline constexpr i32 kEmptiedRoomRef = -1;               // R8 Foes(0).Ref == -1 marks an emptied room
inline constexpr i32 kNewbieCharacterLevel = 1;          // R12 newbie guard: character level 1
inline constexpr i32 kNewbieDepthLimit = 18;             // R12 newbie guard applies in areas below this floor
inline constexpr i32 kNewbieGuildLevelMax = 2;           // R12 newbie guard rerolls leads above this guild level
inline constexpr i32 kNewbieExemptClassIndex = 4;        // R13 table index 4 (not monster Id 4) is exempt
inline constexpr i32 kBypassRollPercent = 1;             // R10 1% bypass roll
inline constexpr i32 kNeutralFriendlyPercent = 40;       // R20 neutral friendliness gate
inline constexpr i32 kJoinRollSides = 3000;              // R24 join chance draw range 1..3000
inline constexpr i32 kJoinChaMultiplier = 10;            // R24 CHA * 10
inline constexpr i32 kJoinLevelMultiplier = 10;          // R24 (level - 1) * 10
inline constexpr i32 kCompanionFloorWindowMax = 2;       // R16 modes 0..2 are floor windows
inline constexpr i32 kCompanionKeepWindowMode = 7;       // R16 mode 7 keeps previous window
inline constexpr i32 kCompanionModeOffset = 7;           // R16 modes above 7 map to mode - 7
inline constexpr i32 kCompanionSubtypeRetryLimit = 150;  // R17 UNKNOWN: 150 (audit) vs 300 abandon (fidelity)
inline constexpr i32 kPenCapacity = 9;                   // pen overflow (store restock) cap on pen 0
inline constexpr i32 kRestockEmptyDrawLimit = 100;       // store restock buy-out gives up after 100 empty draws
inline constexpr f64 kChestCountDamageBonus = 0.75;      // damage modifier added while counting a chest

// R16 companion spawn mode kinds
enum class CompanionModeKind : i32 {
    FloorWindow = 0,   // modes 0..2: floor windows
    TableIndex = 1,    // modes 4..6 and above 7: table index
    KeepWindow = 2     // mode 7: keeps the previous window
};

// R16 result of resolving a companion spawn mode
struct CompanionModeResult {
    CompanionModeKind kind = CompanionModeKind::FloorWindow;
    i32 tableIndex = -1;
};

// ---- rule declarations ----

// R3 respawn base delay in milliseconds for a floor level (150 s per level)
// CONFLICT: text says "150 s per level pair"; engine and fidelity give 150 * level. Engine used.
i64 respawnBaseMs(i32 dungeonLevel);

// R4 respawn delay in milliseconds: base minus 5 s per empty area below header count
// UNKNOWN: full sub0A91 formula (R6) and clamp behaviour are not given; only these terms are declared.
i64 respawnDelayMs(i32 dungeonLevel, i32 headerCount, i32 emptyBelowHeader);

// R8 room refill due: absolute time difference against wait t; stamp 0 means never set and refills
// CONFLICT: refill after more than 2t follows the same rule as stale-group removal is UNKNOWN (R8).
bool roomRefillDue(i64 emptiedStampMs, i64 nowMs, i64 waitMs);

// R8 room is already emptied (Foes(0).Ref == -1); an emptied room is not emptied again
bool roomAlreadyEmptied(const Area& area);

// R8 stamp a room as emptied at nowMs; a clear does not stamp an already-empty room again
void stampRoomEmptied(Ctx& ctx, Area& area, i64 nowMs);

// R9 encounter class pick with the original double-count of the first weight kept
// Leftovers go to the last class; an empty fallback can land on class 0.
i32 pickClassDoubleCount(Ctx& ctx, const Vec<i32>& weights);

// R11 encounter class for a spawn mask; returns -1 for no encounter
// UNKNOWN: whether a zero mask is screened earlier than this routine.
i32 pickClassForMask(Ctx& ctx, i32 spawnMask, i32 dungeonLevel);

// R10 1% bypass roll: the roll is always drawn; the newbie guard only blocks its effect
// CONFLICT: fidelity and audit say the roll is suppressed; engine draws it. Engine used.
bool bypassRollPasses(Ctx& ctx, bool newbieGuardActive);

// R12 newbie guard active: character level 1 in areas below the newbie depth limit
bool newbieGuardActive(i32 characterLevel, i32 dungeonLevel);

// R12 newbie reroll of the lead: guard active, lead above guild level 2, and not table index 4
// Guard scope is the lead only (engine); the test's GL > 2 wording is not used.
bool newbieRerollsLead(bool newbieGuardActive, i32 leadGuildLevel, i32 classIndex);

// R16 resolve companion spawn mode to a floor window, a table index, or keep-window
CompanionModeResult resolveCompanionMode(i32 companionSpawnMode);

// R16 companion encounter groups: re-roll and fill every group. A window with no weight restarts the encounter.
// CONFLICT: Combat.cs invented +-1 floor and same-kind rule is NOT used; the source fills all groups.
void fillCompanionGroups(Ctx& ctx, Encounter& enc, i32 groupCount, i32 floorLevel);

// R17 companion subtype retries exhausted; subtype is dropped after the retry limit
// UNKNOWN: fidelity says abandon after 300 tries; audit says drop after 150. 150 used.
bool companionSubtypeGivesUp(i32 attempts);

// R20 neutral friendliness gate: lead must be neutral, then a 40% roll
// UNKNOWN: whether the gate applies to non-good viewers (engine has no viewer-alignment check).
bool neutralFriendlyGate(Ctx& ctx, const Character& lead);

// R24 join chance roll: draw 1..3000 and test against CHA * 10 - (levelFound - 1) * 10
// CONFLICT: audit (<=, natural CHA, clamp at 0), wiki (<=) and fidelity (<) differ. Fidelity form used.
bool joinChanceRoll(Ctx& ctx, i32 charisma, i32 levelFound);

// R25 join eligibility by guild level: character level equal or greater than monster level
// CONFLICT: wiki says "exceeds"; archive says "equal or greater". Archive wording used. UNKNOWN.
bool joinLevelEligible(i32 characterGuildLevel, i32 monsterGuildLevel);

// R26 a join offer is read only from group 0; multi-group encounters never offer
// UNKNOWN: relation to the wiki's "only group in the room" wording.
bool encounterOffersJoin(const Encounter& enc);

// R26 admit joiners to the front group: joiners take the front's hits; success is silent
void admitJoiners(Ctx& ctx, Encounter& enc, i32 joinerCount);

// R29 level-gap specials: UNKNOWN which specials use LevelGapChance. Not declared.

// Monster Id to table index (first match wins; no match falls back to index 0)
// Shadow Ninja and Morgul share id 307; Morgul appears first and is returned.
i32 monsterIndexForId(const GameData& data, i32 monsterId);

// Asmodeus strip: group count drops by one, never below one
i32 groupCountAfterAsmodeusStrip(i32 groupCount);

// Asmodeus charm kills: a kill by a companion is credited to nobody's Kills
bool killCreditsCharacter(bool killedByCompanion);

// Asmodeus charm: charmed creature raises a kill but no damage; caster still paid experience
void recordCharmKill(Ctx& ctx, Character& caster, const Creature& creature);

// Clear with no able member: original prunes stale groups whenever group count is above one
// CONFLICT: port does not prune when no member is able. Original followed.
void pruneGroupsOnClear(Ctx& ctx, Party& party, Encounter& enc);

// Monster group loop exit: original leaves only for a solo party
// CONFLICT: port leaves on !Party.AnyAble. Original followed.
bool monsterLoopExits(const Party& party);

// Slay-class spell target on a companion: first living group (original)
// CONFLICT: port uses the combat order target. Original followed.
i32 slayTargetGroup(const Encounter& enc);

// Initial KnownAs: original sets one KnownAs per group, not per monster
// CONFLICT: port rolls per monster with a double. Original followed. UNKNOWN: m02AE rounding.
void rollGroupKnownAs(Ctx& ctx, MonsterGroup& group);

// Breath with guild level 0: log(0) error; the group does nothing this turn (chance roll already drawn)
// UNKNOWN: port status differs.
bool breathGroupInert(i32 monsterGuildLevel);

// Breath and acid carry test: companion is carried when hits are below 0
// A companion at exactly 0 is not carried until the next sweep.
bool breathAcidCarryTest(i32 hits);

// Fear release: releases only companions with hits above 0 and not ForLife
bool fearReleases(const Companion& companion);

// Pen overflow: the cap is tested against pen 0 only; the increment may land on another pen
bool penHasRoom(i32 occupancyPenZero);

// Store restock buy-out gives up after the empty-draw limit
bool restockBuyOutGivesUp(i32 emptyDraws);

// Chest counting raises the damage modifier by a fixed amount
f64 chestCountDamageBonus(bool countingChest);

}  // namespace mordor
