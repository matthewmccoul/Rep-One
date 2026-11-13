#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"
#include "mordor/city2a.h"  // FIXED: SeerCharacterOutcome is declared in city2a.h; include it so the type name resolves

namespace mordor {

// ---- constants (R123-R139) ----
inline constexpr i32 kFloorWidth = 30;             // R137 blur clamp 1..30; Automap Square = X + 30*(Y-1)
inline constexpr i32 kFloorHeight = 30;            // R137 blur clamp 1..height. UNKNOWN: height not stated; MDATA11 implies 30x30
inline constexpr i32 kFullBuildLevelLimit = 15;    // R128 mc015E (full build)
inline constexpr i32 kLimitedBuildLevelLimit = 3;  // R128 limited (shareware) build; inferred from MORDOR.WRI and MDATA11
inline constexpr i32 kReleaseLevelThreshold = 15;  // R129 levels >= 15 and some LevelFound/Floor <= 15
inline constexpr i32 kFullMdata11Bytes = 339606;   // R129 release size, full build MDATA11.MDR
inline constexpr i32 kLimitedMdata11Bytes = 68166; // R129 release size, shareware MDATA11.MDR
inline constexpr i32 kSeerDeepestReach = 12;       // R132 DeepestHeCanReach
inline constexpr i32 kSeerEarlyFactor = 5;         // R133 fail if Int(Rnd*100)+1 < level*5
inline constexpr i32 kSeerNeedFactor = 3;          // R136 need = level*3
inline constexpr i32 kSeerLevelOnlySlack = 10;     // R136 roll+10 >= need gives "level only"
inline constexpr i32 kSeerLairPercent = 75;        // R135 lair returned 75% of the time when reached
inline constexpr i32 kSeerMonsterBlurSpread = 4;   // R137 monsters blurred by up to four squares
inline constexpr i32 kSeerPluralNameForm = 2;      // R134 monsterNameFor(Me, 2, record); IdLevel passed as 0
inline constexpr i32 kSeerMonsterFirstRecord = 1;  // R134 monsters read from record 1 on
inline constexpr i32 kSeerItemFirstRecord = 0;     // R134 items read from record 0

// ---- enums and small result structs ----
enum class ReleaseKind : i32 { Full = 0, Limited = 1 };
enum class LimitedAction : i32 { TakeChute = 0, WalkOnChute = 1, TeleporterDestination = 2 };
enum class LimitedMessageKind : i32 { None = 0, ChuteTakeDenied, ChuteWalkOnNoEffect, TeleporterDenied };
enum class ActionTrigger : i32 { EnteredDungeonFromTown = 0, LeavingByStairs = 1 };
enum class ActionRunStart : i32 { Refused = 0, NoHooks, RunHooks };
enum class ExitMenuOutcome : i32 { BlockedByMonsters = 0, ConfirmPrompt };
enum class SessionCloseOutcome : i32 { RefusedMonsters = 0, ConfirmExit };
enum class TakeTarget : i32 { None = 0, Stairs, Chute };
enum class SeerTargetKind : i32 { Monster = 0, Item, Character };
enum class SeerCommonality : i32 { VeryCommon = 0, FairlyCommon, Uncommon, Rare, AlmostUnheard };
enum class SeerFrequency : i32 { AlmostNever = 0, VeryRarely, Rarely, Uncommonly, Commonly, VeryCommonly };

// R123 run-state for the single active action run
struct ActionRunState {
    bool active = false;
};

// R123 per-mod hook record; hooks run in load order (loadOrder ascending)
struct ModHookSlot {
    i32 loadOrder = 0;
    bool live = false;
    bool hasHook = false;
    bool disabled = false;
};

// R133 early failure and level-naming roll
struct SeerEarlyResult {
    bool failed = false;
    bool namesLevel = false;
};

// R135 monster locate result; areaIndex -1 when no candidate
struct SeerMonsterPick {
    bool lairResult = false;
    i32 areaIndex = -1;
};

// R134 name match result; count 0 none, 1 one, >=2 several
struct SeerNameMatch {
    i32 count = 0;
    i32 index = -1;
    bool exact = false;
};

// R136 character locate result
struct SeerCharacterReport {
    SeerCharacterOutcome outcome = SeerCharacterOutcome::CityRefused;  // FIXED: CannotLocate not in SeerCharacterOutcome; closest existing is CityRefused
    i32 x = 0;
    i32 y = 0;
    i32 level = 0;
    bool amongDead = false;
};

// R137 blurred coordinates
struct BlurredPoint {
    i32 x = 0;
    i32 y = 0;
};

// ---- accessors ----
// R19 automap addressing: Square = X + 30 * (Y - 1)
inline constexpr i32 squareIndexOf(i32 x, i32 y) {
    return x + kFloorWidth * (y - 1);
}

// R128 level limit for a release (pure)
inline constexpr i32 buildLevelLimit(ReleaseKind release) {
    return release == ReleaseKind::Full ? kFullBuildLevelLimit : kLimitedBuildLevelLimit;
}

// ---- R123 action hooks on dungeon entry ----
// R123 begin an action run; refused (and logged) if one is already active; NoHooks if no live hooked mod
ActionRunStart beginActionRun(Ctx& ctx, ActionRunState& run, ActionTrigger trigger, const Vec<ModHookSlot>& mods);
// R123 next live hooked mod after afterIndex in load order; -1 when none remain
i32 nextHookedMod(const Vec<ModHookSlot>& mods, i32 afterIndex);
// R123 a hook that fails disables its mod (core has no exceptions; failure is reported by the caller)
void disableModAfterHookFailure(Ctx& ctx, ModHookSlot& mod);
// R123 close the action run once every awaited hook has returned
void finishActionRun(ActionRunState& run);

// ---- R124 stairs up from level 1 ----
// R124 leader placed at the party's square; exit to town alive
void climbToTownAlive(Ctx& ctx, Party& party);
// R124 climb continues only while party is still on stairs and in the dungeon
bool stairsClimbStillValid(const Party& party);

// ---- R125 exit to Main Menu ----
// R125 monsters in the party's area (leader's area); UNKNOWN: which Area roster counts (foes, standingGroup, party.foes)
bool monstersInArea(const Party& party, const FloorState& floor);
// R125 Exit button: beep and refuse if monsters present and leader alive, else "Exit to Main Menu?"
ExitMenuOutcome requestExitToMenu(Ctx& ctx, const Party& party, const FloorState& floor);
// R125 Yes: save, unload party, return to title; location unchanged, no town arrival. UNKNOWN: save routine not in index
void confirmExitToMenu(Ctx& ctx, Party& party);

// ---- R126 window close ----
// R126 user-initiated window close is refused with the EXIT-button message
void refuseWindowClose(Ctx& ctx);

// ---- R127 session-end close ----
// R127 refused with monster message if monsters present, leader alive and level > 0; else "Exit Mordor?"
SessionCloseOutcome requestSessionEndClose(Ctx& ctx, const Party& party, const FloorState& floor);

// ---- Original quirk (no rule number): Take button ----
// Take on the dungeon action button uses stairs or chute, not Objects & Spells.
// Square bits checked: FloorFlag FLAG_STAIRS_UP, FLAG_STAIRS_DOWN, FLAG_CHUTE.
TakeTarget takeButtonTarget(const FloorState& floor, const Party& party);

// ---- R128 limited build level clamp ----
// R128 clamp requested level to the build limit (emits LIMITED VERSION message); levels below 0 left alone
i32 clampLevelRequest(Ctx& ctx, ReleaseKind release, i32 requestedLevel);

// ---- R129 release detection ----
// R129 release from first word of MDATA11.MDR and the monster/item level checks.
// Caller computes the two bools from GameData. Monster and item checks do not separate builds (level count is reliable).
ReleaseKind classifyRelease(i32 levelsWord, bool someMonsterLevelAtOrBelowThreshold, bool someItemFloorAtOrBelowThreshold);

// ---- R130 limited-version last-level messages ----
// R130 emit the limited-build chute/teleporter message. TakeChute and WalkOnChute compare level to lastLevel;
// TeleporterDestination denies when level > lastLevel.
// UNKNOWN: value of mc0624 is not stated; caller supplies it as lastLevel for TakeChute.
// CONFLICT: R130 teleporter line tests against mc015E (15); R131 uses the build limit. Caller passes the build limit.
LimitedMessageKind reportLimitedMessage(Ctx& ctx, LimitedAction action, i32 level, i32 lastLevel);

// ---- R131 destination denial ----
// R131 teleporter and chute destinations past the build limit are denied (pure)
bool isDestinationDenied(i32 destinationLevel, ReleaseKind release);

// ---- R132 Seer reach ----
// R132 Seer reaches nothing below dungeon level 12 (pure)
bool seerCanReach(i32 dungeonLevel);

// ---- R133 Seer early failure ----
// R133 roll early failure (fail if Int(Rnd*100)+1 < level*5); on failure a second roll names the level or not
SeerEarlyResult rollSeerEarlyFailure(Ctx& ctx, i32 level);

// ---- R134 Seer name match ----
// R134 case-insensitive prefix test: name.Length >= typed.Length and name starts with typed (pure)
bool isSeerNamePrefix(const Str& typed, const Str& name);
// R134 match typed name against names; exact match wins at once; count 0 none, 1 one, >=2 several (pure)
SeerNameMatch matchSeerNames(const Str& typed, const Vec<Str>& names);
// R134 first record read by LOCATE: monsters from record 1, items from record 0 (pure)
i32 seerFirstRecord(SeerTargetKind kind);
// R134 monster name form used by LOCATE (not in API INDEX; declared here because R134 uses it).
// UNKNOWN: type of "Me" is not stated; taken as monster id. nameForm 2 = plural; caller passes IdLevel 0.
Str monsterNameFor(const GameData& data, i32 monsterId, i32 nameForm, IdLevel idLevel);

// ---- R135 Seer monster locate ----
// R135 lair returned 75% of the time when reached; otherwise random candidate area whose spawn mask admits monsterType
SeerMonsterPick locateMonster(Ctx& ctx, const FloorState& floor, i32 monsterId, i32 monsterType);
// R135 popcount of spawn mask over monster classes (pure)
i32 countSpawnClasses(i32 spawnMask);
// R135 commonality wording from class count (table) (pure)
SeerCommonality seerCommonality(i32 classCount);
// R135 frequency wording from monster chance (table). UNKNOWN: unit of chance (taken as percent)
SeerFrequency seerFrequency(i32 chancePercent);

// ---- R136 Seer character locate ----
// R136 locate roll for a character; a character in the city cannot be located.
// UNKNOWN: how city is detected; caller passes subjectInCity.
SeerCharacterReport locateCharacter(Ctx& ctx, const Character& subject, i32 searchLevel, bool subjectInCity);

// ---- R137 Seer blur ----
// R137 blur each coordinate by up to spread either way; clamp to 1..kFloorWidth and 1..kFloorHeight.
// Monsters use kSeerMonsterBlurSpread. R139: spread for characters and items is UNKNOWN; caller passes it.
BlurredPoint blurPoint(Ctx& ctx, i32 x, i32 y, i32 spread);

// ---- R138 Seer fee ----
// R138 fee taken as charPay takes it; bank makes up a short purse. Returns total charged.
// UNKNOWN: fee amount (not stated); exact bank top-up order not stated.
Currency chargeSeerFee(Ctx& ctx, Purse& wallet, Currency fee);

// ---- Preserved formulas (no rule number) ----
// NoteDepth: if level <= deepestReached return false; else set deepestReached = level and return true
bool noteDepth(i32 level, i32& deepestReached);
// deepest = Max(dungeonDeepest, mapDeepest); if 0 then 1 (pure)
i32 effectiveDeepest(i32 dungeonDeepest, i32 mapDeepest);

}  // namespace mordor
