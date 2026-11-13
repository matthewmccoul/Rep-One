#pragma once
// loot2.h: SRD part c14p2 (loot, chests, traps, locks, chest gold, shelf sale, quests, drops).
// UNKNOWN: the supplied SRD part contains only the source notes (UNKNOWN/CONFLICT list). No R-numbers
// were given, so declarations are grouped by source chunk id [cNN] instead of rule number.
// Rename the groups to R-numbers once the rule text is available.
// Items already declared in the API INDEX (for example withering() in progression2.h) are called, not redeclared.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants
inline constexpr i32 kChestCountStart = 2;      // [c65][c24] SquareState.chestCount at creation; CONFLICT-22
inline constexpr i32 kChestTrapTries = 200;     // [c19] port cap on trap-bit draws, returns 0 on exhaustion; CONFLICT-5
inline constexpr i32 kLairBoxBonus = 25;        // [c18] lair adds 25 to BoxChance[2] and to both thresholds; CONFLICT-2
inline constexpr i32 kLockGoldNum = 3;          // [c5][c7] magic lock multiplies chest gold by 3/2; CONFLICT-19
inline constexpr i32 kLockGoldDen = 2;
inline constexpr i32 kItemRollCapThreshold = 90; // [c12] item roll above 90 is capped; CONFLICT-4
inline constexpr i32 kBuyOutDrawCap = 100;      // [c22] buy-out draw cap quirk; CONFLICT-13 (pass -1 for unbounded)
inline constexpr i32 kWitheringWeeks = 4;       // [c64] Withering adds 4 weeks; CONFLICT-28 (engine: message only from sub085B)
inline constexpr i32 kTrapIdentifyMin = 2;      // [c107] clamp 2..98; UNKNOWN-27 (clamp applied to conf per engine [c19])
inline constexpr i32 kTrapIdentifyMax = 98;
inline constexpr i32 kKnownScaleMax = 3;        // [c22] Known scale 0..3; CONFLICT-15
inline constexpr i32 kStartingGoldWhole = 1500; // UNKNOWN-22: port value not in sources; manual value 1,500 [c84]

// ---- result enums and structs
enum class ChestOpenOutcome : i32 { Waiting = 0, Opened, Refused };
enum class DisarmOutcome : i32 { Disarmed = 0, Failed, Triggered };

struct ChestTuning {
    // UNKNOWN: BoxChance[0..3], LockedChance and ItemChance values are not in the SRD part; supplied by data.
    // BoxChance[3] is the lock-trap percentage (CONFLICT-16).
    Arr<i32, 4> boxChance{};
    i32 lockedChance = 0;
    i32 itemChance = 0;
};


struct DisarmResult {
    DisarmOutcome outcome = DisarmOutcome::Failed;
    i32 identifyChance = 0;
};

// Magic lock when ChestClass > 1 and the square allows magic (CONFLICT-1); result is MAGIC_LOCKED or UNLOCKED.
ChestLock rollChestLock(Ctx& ctx, const ChestTuning& tuning, ChestClass cls, bool squareAllowsMagic);



// ---- [c12][c24][c34][c65] chest countdown and opening
// One chest tick: count 2 -> 1 -> 0 -> opens on the next tick (CONFLICT-22). Returns true when the chest opens.
bool tickChestCountdown(Ctx& ctx, SquareState& chest);
// Opener attempts the chest: trap, then item attach, then gold. Opener refusal rules unknown (UNKNOWN-16, CONFLICT-8).
// FIXED: ChestOpenResult named no type; replaced by ChestOpenOutcome (defined above in this file).
i32 openChest(Ctx& ctx, Party& party, i32 openerIndex, SquareState& chest);



// ---- [c106] guild quest assignment
// Quest parameters unverified (F37); "chance" field undefined (UNKNOWN-19); monster set (CONFLICT-33).
void assignGuildQuest(Ctx& ctx, const GameData& data, GuildMembership& membership, i32 guildLevel);




}  // namespace mordor
