#pragma once
// dungeon2b: SRD part c15p2. Chutes and pits, teleporters, square effects,
// lost state, fog and perception, rescue and rocked characters, trap disarm
// formulas. Declarations only.
#include "mordor/types.h"
#include "mordor/dungeon1b.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants ----
inline constexpr i32 kChuteFloatPercent = 7;           // R2 chute float threshold as written (level 8)
inline constexpr i32 kChuteFloatLevel = 8;             // R2 level the float threshold is stated for
inline constexpr i32 kFullBuildLevelCount = 15;        // R21 full build has at least 15 dungeon levels
inline constexpr i32 kLimitedBuildTeleportCap = 3;     // R17 limited spell clamp (value inferred, c33/c35)
inline constexpr i32 kShareMonsterItemReach = 16;      // c35 shareware monsters and items reach level 16
inline constexpr i32 kRescueBaseLevel = 16;            // R31 rescue chance is max(16 - L, floor)
inline constexpr double kRescueMinChance = 0.1;        // R31 floor applied before easy-find
inline constexpr double kRescueCeilingChance = 100.0;  // R31 ceiling on level 1 areas 18 and lower

// FIXED: SquareEffectStep is declared nowhere in the API INDEX or state headers,
// so the square-effect steps are plain i32 values in the preferred order (c44).
// Order: 0 WaterOrQuicksand, 1 AntiMagicAndStudPerception, 2 Extinguisher,
// 3 Facing, 4 Rotator, 5 Pit, 6 StairsCaption, 7 Teleporter, 8 Chute
inline constexpr Arr<i32, 9> kSquareEffectOrder = {
    0,  // WaterOrQuicksand
    1,  // AntiMagicAndStudPerception
    2,  // Extinguisher
    3,  // Facing
    4,  // Rotator
    5,  // Pit
    6,  // StairsCaption
    7,  // Teleporter
    8   // Chute
};


// R8 result of a pit fall
struct PitFallResult {
    i32 fallen = 0;
    i32 killed = 0;
};

// R14 setters of the forced flag (preferred: three setters, probe asserts three, c44/c56)
enum class ForcedFlagSetter : i32 {
    TeleporterSquare = 0,
    TeleportSpell,
    LoneChestTrap
};

// R31 result of a body search
struct RescueResult {
    i64 daysSearched = 0;
    bool found = false;
};

// ---- chutes, pits and falls ----

// R2 per-member chute float check (R1 CONFLICT: levitation is not a float here; engine preferred, c30/c45)
// CONFLICT: threshold direction unresolved (Rnd x 100 <= 7 reads as about 7%, dossier says about 93%).
bool chuteFloatRoll(Ctx& ctx, const Character& member, i32 dungeonLevel);

// R3 chute float threshold in percent for a level; UNKNOWN beyond level 8 (returns kChuteFloatPercent)
i32 chuteFloatPercent(i32 dungeonLevel);

// R4 landing level of a chute; depth 0 means no floor change (c30)
// UNKNOWN: whether another mechanism can set level 0
i32 chuteLandingLevel(i32 fromLevel, i32 depth);

// R5 number of levels a chute drops, from the chute record (rec.Depth)
// CONFLICT: manual says one level; engine uses variable depth (preferred: engine, c45/c88)
i32 chuteDepth(const FloorChute& chute);

// R6 chute damage to a member; expression beyond the test is UNKNOWN (c34, c66)
i32 chuteDamage(Ctx& ctx, const Character& member, i32 chuteLevel);

// R7 pit damage from pit size: round_half_even(log(size + 2)) (preferred engine, c44)
// CONFLICT: test uses Log(4) with Round_half_even (c66). Log base and Integer vs Floor UNKNOWN.
i32 pitDamage(i32 pitSize);

// R8 pit fall: every member rolls separately (current port, c44)
PitFallResult applyPitFall(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 pitSize);

// R8 chute aftermath and party-list prune; runs after pit falls and walked chutes, not after a taken chute (c30)
void pruneFallenMembers(Ctx& ctx, Party& party);

// R8 landing area of a chute: the area of the landed-on square on the lower floor (c66)
i32 chuteLandingArea(const FloorState& lowerFloor, i32 squareIndex);

// Chute TAKE: drops everyone, no box and no sound; refused on the limited build dead-end level (c34, c45)
ChuteTakeOutcome takeChute(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 chuteDepth, bool deadEndLevel);

// Chute under the caster skips SettleArea because the fall settles the area it ends in (c47)
bool settleAreaAfterFall(bool chuteUnderCaster);

// ---- teleporters ----

// R11 teleporter is always noticed and its bit is written to the automap (c28, c63)
// CONFLICT: port records no teleporter on entry (c13). Engine preferred.
void noticeTeleporter(Ctx& ctx, SquareState& sq);

// Writes a perceived feature bit into the automap (sub087C; MAZEMOD 634-637) (c28)
void writeAutomapBit(Ctx& ctx, SquareState& sq, i32 bit);

// R12 accept test for a random teleport destination: rejects rock and teleporter squares (c22)
// CONFLICT: FIDELITY text says redraw until not rock only (c28). Engine followed.
bool teleportDrawAccepts(const FloorState& floor, i32 squareIndex);

// R12 random teleport destination draw, redrawn until accepted; uses rng (c22)
i32 drawTeleportDestination(Ctx& ctx, const FloorState& floor);

// R13 and R19 re-run of the square routine after a teleport or chute destination (c31, c44)
// CONFLICT: engine has no loop guard and chained teleporters occur; preferred settles and describes only (c31).
void rerunSquare(Ctx& ctx, Party& party, Vec<Companion>& companions, FloorState& floor, i32 squareIndex);

// R14 forced flag setter lookup (three setters in the preferred source, c44/c56)
// UNKNOWN: where the forced flag is stored.
bool forcedFlagSetBy(ForcedFlagSetter who);

// R15 chest teleporter moves only the opener, only in a party; a lone opener's room is settled at once (c46)
// Formula also moves a lone opener via SetPosition; taken as correct.
void applyChestTeleporter(Ctx& ctx, Party& party, FloorState& floor, i32 openerIndex);

// R16 level used for the first teleporter draw
// UNKNOWN: old level or leader's new level. Returns the leader's new level (inferred, c22/c31).
i32 teleporterDrawLevel(const Party& party);

// R17 limited-build spell clamp applied before the fall and rock tests (c35)
i32 limitedSpellLevel(bool limitedBuild, i32 level);

// R17 teleporter square past the last level does nothing but logs (c44, c63)
bool teleporterSquareDenied(bool limitedBuild, i32 destLevel, i32 lastLevel);

// R18 teleport spell rock test: rock if level exceeds MaxDungeonLevel or the dungeon count (c22)
// UNKNOWN: numeric value of the mc0276 threshold named in the comment.
bool teleportLevelIsRock(i32 level, i32 maxDungeonLevel, i32 dungeonLevelCount);

// ---- limited and shareware builds ----

// R21 limited build test: uses the dungeon level count only (preferred, c35)
// CONFLICT: source test also lists monsters and items stopping short of 15 (c33). Those are unreliable.
bool isLimitedBuild(i32 dungeonLevelCount);

// R22 limited and shareware naming: UNKNOWN whether one build. No routine declared.

// ---- square effects ----

// R23 square-effect resolution in kSquareEffectOrder (preferred order, c44)
// CONFLICT: c31 gives a different order and per-member extinguisher step.
void resolveSquareEffects(Ctx& ctx, Party& party, Vec<Companion>& companions, FloorState& floor, i32 squareIndex);

// R24 square refuses casts: anti-magic square (preferred, c28)
// CONFLICT: extinguisher refusal per modding doc (c36) not confirmed. UNKNOWN whether it covers the tile or an area.
bool squareRefusesCasts(const SquareState& sq);

// R25 Ethereal Portal: step one square forward, then test the stepped-onto square; rock there kills (c9)
// CONFLICT: wiki tests the square behind (c89). Engine preferred.
bool etherealPortalKills(const FloorState& floor, i32 x, i32 y, Facing facing);

// R26 extinguisher behaviour: UNKNOWN in the wiki ("dont know", c96). No routine declared.

// ---- combat clock and encounters ----

// R27 same-area teleport: turns the combat clock off until the next monShowMonsters (preferred, c30)
// CONFLICT: c44 wording is ambiguous.
void suspendCombatClockSameArea(Ctx& ctx, Encounter& encounter);

// R28 lair test: engine meaning is the monster's home area (c20). Wiki meaning (colored room) differs (c87).
bool isLairOf(const Creature& monster, i32 areaId);

// R29 encounter generation beyond the lead group is UNKNOWN (c47). No routine declared.

// ---- rescue, death and retrieval ----

// R30 rescue arrival shows the town dead screen (preferred engine, c39)
// CONFLICT: Q2 code loads the dungeon form when DungeonLevel is not 0 or not dead (c59).
bool rescueArrivesInTown(const Character& body);

// R31 rescue chance per day: max(16 - L, 0.1); easy-find adds 16 - L after the floor (c17)
// Quirk: above level 16 the chance is driven negative, so the body is never found (c17).
// CONFLICT: wiki gives different tiers (c97).
double rescueChance(i32 level, bool easyFind);

// R31 day-by-day body search; ends at kRescueDayCap days (c17)
RescueResult searchForBody(Ctx& ctx, i32 level, bool easyFind);

// R32 rocked character undone by a spell: only Resurrect in the engine (c11)
// CONFLICT: wiki says retrieval is like Rock Retrieve (c91). Engine preferred.
bool rockUndoneBy(RaiseKind kind);

// R33 dead-body facing from the leader's facing: (lead.Facing + 3) & 3 (c48)
// UNKNOWN: whether the result is 0-based or 1-based.
i32 deadBodyFacing(i32 leadFacing);

// ---- traps and disarm ----

// R34 disarm log term: ln(max(2, M)) (preferred, c24). c97 gives ln(mLvl) with no max.
double trapDisarmLogTerm(double m);

// R34 disarm stat reading: WIS/4 + DEX/2 (c24, c97)
// UNKNOWN: integer or fractional division.
double trapDisarmStat(i32 wis, i32 dex);

// R35 trap identification roll may show a guessed trap that differs from the disarm value (c24). Not a rule; no routine.
// R36 disarm party-size adjustment is named but unspecified (c24). No routine.

// ---- lost state ----

// Rotator loss sets position and facing only; the floor flag is untouched (c13)
void loseBearingByRotator(Ctx& ctx, Party& party);

// FoundYourself resolves all lost flags. The position roll runs when floor or facing is lost (c13)
void foundYourself(Ctx& ctx, Party& party);

// Walking onto mapped ground ends being lost outright (c13)
void stepOntoMappedGround(Ctx& ctx, Party& party);

// The step on which you realise you are lost is recorded on the automap (c9)
void realiseLost(Ctx& ctx, Party& party, FloorState& floor, i32 squareIndex);

// A newcomer takes the leader's lost flags even when later refused as dead (c30)
void inheritLostFlags(Ctx& ctx, Party& party, Character& newcomer);

// ---- fog and perception ----

// Fog on the stepped-onto square gates all neighbour writes (c13)
void markNeighbours(Ctx& ctx, FloorState& floor, i32 x, i32 y, bool steppedOntoFogged);

// Fog removes walls and doors but not the square (c27, c28)
void applyFog(Ctx& ctx, SquareState& sq);

// The mapped bit is set whether or not the square is fogged (c27, c28)
void setMappedBit(Ctx& ctx, SquareState& sq);

// A wall bump in fog is recorded; fog edges are not mapped in the grey view (c42, c44)
void recordBumpInFog(Ctx& ctx, SquareState& sq);

// Secret doors found before a backfill are forgotten (c13)
void forgetSecretDoorsBeforeBackfill(Ctx& ctx, FloorState& floor);

// R perception ahead gear rating: WIS + StatMods[WIS]/4 (MAINMOD2 1962) (c17, c31)
// UNKNOWN: integer division of the /4 term is assumed.
i32 perceptionAheadRating(const Character& leader);

// Perception ahead announces each feature on every turn facing it, with no automap check (c17, c28)
void perceiveAhead(Ctx& ctx, const Party& party, const FloorState& floor);

// Square perception: certainty taken from (int)perception (c17)
// UNKNOWN: exact exclusion count rule; the "eight bits always pass" comment does not match.
bool squarePerceptionPasses(Ctx& ctx, i32 perception, i32 exclusions);

// Quicksand is suppressed only while standing in quicksand, and the square is still mapped (c17, c28)
bool quicksandSuppressesPerception(const Party& party, const SquareState& standingOn);

// ---- chests, areas and companions ----

// Chest trap in a party: chest stays untrapped and full; loot, gold and trap exit skipped; "You realize" only when alone (c46)
void resolveChestTrapInParty(Ctx& ctx, Party& party, SquareState& sq);

// SyncAreas: writes area -1 at level 0; a body in rock is written as nought and dropped (c45)
void syncAreas(Ctx& ctx, Party& party, FloorState& floor);

// A companion put down is released from its slot and not left lying in it (c45)
void putDownCompanion(Ctx& ctx, Companion& companion);

// Teleport-related caption and screen behaviour, look readout, take-button mapping and asmodeus picture are out of scope (screens/input).

}  // namespace mordor
