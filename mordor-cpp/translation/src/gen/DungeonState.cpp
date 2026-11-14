#include "core.h"

namespace mc {

bool DungeonState::NoteDepth(int32_t level) {
    if (level <= DeepestReached) return false;
    DeepestReached = level;
    return true;
}

cs::String DungeonState::Key(int32_t level, int32_t areaId) {
    return cs::ToStr(level) + ":" + cs::ToStr(areaId);
}

double DungeonState::SecondsSinceMidnight(cs::DateTime when) {
    // DateTime.TimeOfDay.TotalSeconds
    const int64_t TicksPerDay = 864000000000LL;
    cs::TimeSpan tod{};
    tod.ticks = when.ticks % TicksPerDay;
    return tod.TotalSeconds();
}

int32_t DungeonState::TallyOf(int32_t level, int32_t headerAreas) {
    int32_t t{};
    return Tally.TryGetValue(level, t) ? t : headerAreas;
}

cs::TimeSpan DungeonState::RespawnDelay(int32_t level, int32_t headerAreas, int32_t tally) {
    double t = 1200 + ((5 * (level / 2.0)) * 60);
    t = t - ((headerAreas - tally) * 5);
    if (t < 1200 / 10.0) t = 1200 / 10.0;
    // TimeSpan.FromSeconds(double): ticks truncated
    cs::TimeSpan ts{};
    ts.ticks = cs::CastDouble<int64_t>(t * 10000000.0);
    return ts;
}

double DungeonState::RespawnSeconds(int32_t level, int32_t headerAreas) {
    return RespawnDelay(level, headerAreas, TallyOf(level, headerAreas)).TotalSeconds();
}

cs::Ref<RoomRecord> DungeonState::LoadRoom(int32_t level, int32_t areaId) {
    cs::Ref<RoomRecord> r{};
    return Rooms.TryGetValue(Key(level, areaId), r) ? r->Copy() : cs::New<RoomRecord>();
}

void DungeonState::StoreRoom(int32_t level, int32_t areaId, cs::Ref<RoomRecord> r) {
    if (areaId <= 0) return;   // "If p05BA <= False Then ... Exit Function" (3912)
    Rooms[Key(level, areaId)] = r->Copy();
}

bool DungeonState::IsOccupied(int32_t level, int32_t areaId, int32_t headerAreas, cs::DateTime now) {
    return areaId > 0 && (RefillDue(level, areaId, headerAreas, now) || LoadRoom(level, areaId)->AnyStanding());
}

bool DungeonState::RefillDue(int32_t level, int32_t areaId, int32_t headerAreas, cs::DateTime now) {
    if (areaId <= 0) return false;
    auto rec = LoadRoom(level, areaId);
    double t = RespawnSeconds(level, headerAreas), nowSec = SecondsSinceMidnight(now), stamp = rec->Foes[0]->Stamp;
    bool empty = !rec->AnyStanding();
    if (!empty && (cs::Math::Abs(nowSec - stamp) > t * 2 || stamp == 0)) empty = true;
    return empty && (cs::Math::Abs(nowSec - stamp) > t || stamp == 0);
}

void DungeonState::NoteRefilled(int32_t level, int32_t headerAreas) {
    Tally[level] = TallyOf(level, headerAreas) + 1;
}

void DungeonState::MarkCleared(int32_t level, int32_t areaId, cs::DateTime now, int32_t headerAreas) {
    auto rec = LoadRoom(level, areaId);
    // "If Foes(0).Ref = -1 Then Exit Sub" (MONSMOD.BAS:769): a room already emptied is not
    // stamped again, nor the tally lowered twice
    if (rec->Foes[0]->Ref == -1) return;
    for (auto& f : rec->Foes) f->GroupCount = 0;
    rec->Foes[0]->Ref = -1;
    rec->Foes[0]->Stamp = SecondsSinceMidnight(now);
    rec->Foes[0]->JoinOffer = 0;
    StoreRoom(level, areaId, rec);
    Tally[level] = TallyOf(level, headerAreas) - 1;
}

cs::String DungeonState::PathFor(cs::String savesDir) {
    return cs::Path::Combine(savesDir, "dungeon.json");
}

cs::Ref<DungeonState> DungeonState::Load(cs::String savesDir) {
    try {
        cs::String p = PathFor(savesDir);
        if (cs::File::Exists(p)) {
            auto loaded = cs::Json::Deserialize<cs::Ref<DungeonState>>(cs::File::ReadAllText(p));
            auto state = (loaded ? loaded : cs::New<DungeonState>());
            state->TakeLegacyStamps();
            return state;
        }
    } catch (...) {
    }
    return cs::New<DungeonState>();
}

void DungeonState::TakeLegacyStamps() {
    if (Cleared != nullptr)
        for (auto& kv : Cleared) {
            auto key = kv.Key;
            auto at = kv.Value;
            if (at < 0 || at > 86400 || Rooms.ContainsKey(key)) continue;
            auto rec = cs::New<RoomRecord>();
            rec->Foes[0]->Ref = -1;          // emptied, by sub0C53
            rec->Foes[0]->Stamp = at;
            Rooms[key] = rec;
        }
    Cleared = nullptr;
    Spawned = nullptr;
}

void DungeonState::Save(cs::String savesDir) {
    try {
        cs::Directory::CreateDirectory(savesDir);
        cs::String p = PathFor(savesDir), temp = p + ".tmp";
        cs::JsonSerializerOptions opts{};
        opts.WriteIndented = true;
        (void)opts;
        cs::File::WriteAllText(temp, cs::Json::Serialize(*this, cs::Json::Indented()));
        cs::File::Move(temp, p, true);
    } catch (...) {
    }
}

}  // namespace mc
