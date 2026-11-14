#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct DungeonState {
    cs::Dictionary<cs::String, cs::Ref<RoomRecord>> Rooms = cs::Dictionary<cs::String, cs::Ref<RoomRecord>>::New();
    cs::Dictionary<cs::String, double> Cleared;
    cs::Dictionary<cs::String, double> Spawned;
    int32_t DeepestReached = 0;
    cs::Dictionary<int32_t, int32_t> Tally = cs::Dictionary<int32_t, int32_t>::New();

    bool NoteDepth(int32_t level);
    static cs::String Key(int32_t level, int32_t areaId);
    static double SecondsSinceMidnight(cs::DateTime when);
    int32_t TallyOf(int32_t level, int32_t headerAreas);
    static cs::TimeSpan RespawnDelay(int32_t level, int32_t headerAreas, int32_t tally);
    double RespawnSeconds(int32_t level, int32_t headerAreas);
    cs::Ref<RoomRecord> LoadRoom(int32_t level, int32_t areaId);
    void StoreRoom(int32_t level, int32_t areaId, cs::Ref<RoomRecord> r);
    bool IsOccupied(int32_t level, int32_t areaId, int32_t headerAreas, cs::DateTime now);
    bool RefillDue(int32_t level, int32_t areaId, int32_t headerAreas, cs::DateTime now);
    void NoteRefilled(int32_t level, int32_t headerAreas);
    void MarkCleared(int32_t level, int32_t areaId, cs::DateTime now, int32_t headerAreas);
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<DungeonState> Load(cs::String savesDir);
    void TakeLegacyStamps();
    void Save(cs::String savesDir);
};

}  // namespace mc
