#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GuildLog_Entry {
    int32_t Guild = 0;
    int64_t Day = 0;
    cs::String Text{};
};

struct GuildLog_Wrote {
    static cs::String Q(cs::String name);
    static cs::String JoinedGuild(cs::String who);
    static cs::String Titled(cs::String who, cs::String title, int32_t level);
    static cs::String BecameGuildmaster(cs::String who);
    static cs::String AppointedGuildmaster(cs::String who);
    static cs::String QuestForItem(cs::String who, cs::String item);
    static cs::String QuestForMonster(cs::String who, cs::String monster);
    static cs::String GuildmasterAward(cs::String who, cs::String item);
    static cs::String Demoted(cs::String who, int32_t levels, Sex sex);
    static cs::String SetRecord(cs::String who, cs::String record, cs::String value);
    static cs::String FirstToLevel(cs::String who, int32_t level);
    static cs::String PartyBeatAsmodeus(cs::String who);
    static cs::String AloneBeatAsmodeus(cs::String who);
    static cs::String Resurrected(cs::String who, cs::String by);
    static cs::String ResurrectedInMorgue(cs::String who);
    static cs::String DiedOnLevel(cs::String who, int32_t level);
    static cs::String DiedSomewhere(cs::String who);
};

struct GuildLog {
    using Entry = GuildLog_Entry;
    using Wrote = GuildLog_Wrote;

    static constexpr int32_t KeepOnClean = 50;

    inline static const cs::String FileName = "MDATA9.MDR";
    inline static const cs::String BackupName = "MDATA9B.MDR";
    inline static const cs::String TextName = "GUILDLOG.TXT";

    static cs::String PathFor(cs::String savesDir);
    static int64_t Today();
    static int64_t DaySerial(cs::DateTime d);
    static cs::DateTime FromDaySerial(int64_t day);
    static cs::List<cs::Ref<GuildLog_Entry>> All(cs::String savesDir);
    static cs::List<cs::Ref<GuildLog_Entry>> For(cs::String savesDir, int32_t guild);
    static cs::String Prefix(cs::Ref<GuildLog_Entry> e, int64_t today);
    static cs::String Line(cs::Ref<GuildLog_Entry> e, int64_t today);
    static bool NeedsCleaning(cs::String savesDir, int32_t guildCount);
    static void Append(cs::String savesDir, int32_t guild, cs::String text);
    static void Rewrite(cs::String savesDir, cs::List<cs::Ref<GuildLog_Entry>> entries);
    static void WriteAll(cs::String savesDir, cs::List<cs::Ref<GuildLog_Entry>> entries);
    static int32_t Clean(cs::String savesDir, cs::String doneBy, cs::Func<cs::String(int32_t)> guildName, bool writeText);
};

}  // namespace mc
