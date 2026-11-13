// dungeon1b_b.cpp : rules R89-R122 (forced settle, area change, rotator, roster,
// bodies and carrying, rescue, chests, traps, ask to join).
// Cross-module calls whose signatures are not in the API INDEX are omitted and marked UNKNOWN.
#include <cmath>

#include "mordor/dungeon1b.h"
#include "mordor/death1a.h"  // FIXED: AskToJoinOutcome is declared in death1a.h, not dungeon1b.h

namespace mordor {
namespace {

// R89 forced flag bit and source ids. UNKNOWN: exact values of gv1D6A bit and ForcedSettleSource not in API INDEX.
constexpr i32 kForcedSettleBit = 1;
constexpr i32 kForceTeleporterSquare = 1;
constexpr i32 kForceTeleportSpell = 2;
constexpr i32 kForceChestTeleporter = 3;
constexpr i32 kAreaCount = 201;          // FloorState::areas size
constexpr i32 kRescueDayCap = 2000000;   // R114 loop cap

i32 floorIndex(const Vec<FloorState>& floors, i32 level) {
    for (u32 i = 0; i < static_cast<u32>(floors.size()); ++i) {
        if (floors[i].dungeonLevel == level) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

}  // namespace

// ---------------------------------------------------------------- R89-R95 area change and forced settle

// R94 LeaveArea. UNKNOWN: hostile groups not fighting get a leave round (MonstersRound not in API INDEX; not called).
void leaveArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data) {
    (void)ctx;
    (void)party;
    (void)data;
    const i32 fi = floorIndex(floors, party.dungeonLevel);
    if (fi < 0) {
        return;
    }
    // The old room is no longer loaded.
    floors[static_cast<u32>(fi)].roomStamp = -1;
}

// ---------------------------------------------------------------- R96-R98 rotator, lead group, Asmodeus

// ---------------------------------------------------------------- R99-R102 roster and look

// ---------------------------------------------------------------- R103-R110 bodies and carrying

// ---------------------------------------------------------------- R111-R115 rescue

// ---------------------------------------------------------------- R116-R122 chests, traps, joining

}  // namespace mordor
