#include "core.h"

namespace mc {

namespace {
// .NET DateTime constants: ticks per day, ticks of 1899-12-30 (the OLE automation epoch), max ticks
constexpr int64_t TicksPerDay = 864000000000LL;
constexpr int64_t OleEpochTicks = 599264352000000000LL;
constexpr int64_t MaxTicks = 3155378975999999999LL;

cs::DateTime DateOf(cs::DateTime d) {
    cs::DateTime r;
    r.ticks = d.ticks - d.ticks % TicksPerDay;
    return r;
}

cs::DateTime TodayDate() { return DateOf(cs::DateTime::Now()); }

void AppendAllText(const cs::String& path, const cs::String& text) {
    std::ofstream f(path, std::ios::binary | std::ios::app);
    if (!f) throw cs::IOException("cannot write " + path);
    f << text;
}

void PutInt32(cs::Bytes& b, int32_t v) {
    uint8_t buf[4];
    cs::BinaryPrimitives::WriteInt32LittleEndian(buf, v);
    for (int i = 0; i < 4; i++) b.Add(buf[i]);
}

void PutInt64(cs::Bytes& b, int64_t v) {
    uint8_t buf[8];
    cs::BinaryPrimitives::WriteInt64LittleEndian(buf, v);
    for (int i = 0; i < 8; i++) b.Add(buf[i]);
}
}  // namespace

cs::String GuildLog::PathFor(cs::String savesDir) { return cs::Path::Combine(savesDir, FileName); }

int64_t GuildLog::Today() { return DaySerial(TodayDate()); }

int64_t GuildLog::DaySerial(cs::DateTime d) {
    int64_t diff = DateOf(d).ticks - OleEpochTicks;
    return cs::CastDouble<int64_t>((double)diff / (double)TicksPerDay);
}

cs::DateTime GuildLog::FromDaySerial(int64_t day) {
    double ms = (double)day * 86400000.0;
    if (ms <= -315537897600000.0 || ms >= 315537897600000.0) throw cs::ArgumentOutOfRangeException("value");
    int64_t t = OleEpochTicks + cs::CastDouble<int64_t>(ms) * 10000LL;
    if (t < 0 || t > MaxTicks) throw cs::ArgumentOutOfRangeException("value");
    cs::DateTime r;
    r.ticks = t;
    return r;
}

cs::List<cs::Ref<GuildLog_Entry>> GuildLog::All(cs::String savesDir) {
    auto entries = cs::List<cs::Ref<GuildLog_Entry>>::New();
    cs::String path = PathFor(savesDir);
    if (!cs::File::Exists(path)) return entries;
    try {
        auto data = cs::File::ReadAllBytes(path);
        int64_t length = data.Count();
        int64_t pos = 0;
        auto need = [&](int64_t n) { if (pos + n > length) throw cs::EndOfStreamException(); };
        if (length < 4) return entries;
        need(4); pos += 4;  // the header count; the records are the truth
        while (pos < length) {
            need(4);
            int32_t guild = cs::BinaryPrimitives::ReadInt32LittleEndian(data.Raw().data() + pos);
            pos += 4;
            need(8);
            int64_t day = cs::BinaryPrimitives::ReadInt64LittleEndian(data.Raw().data() + pos);
            pos += 8;
            need(4);
            int32_t len = cs::BinaryPrimitives::ReadInt32LittleEndian(data.Raw().data() + pos);
            pos += 4;
            if (len < 0 || pos + len > length) break;
            cs::String text = cs::Encoding::Latin1GetString(data.Raw().data() + pos, len);
            pos += len;
            entries.Add([&] { auto _o = cs::New<GuildLog_Entry>(); _o->Guild = guild; _o->Day = day; _o->Text = text; return _o; }());
        }
    } catch (...) {
    }  // a damaged log loses its tail, not the game
    return entries;
}

cs::List<cs::Ref<GuildLog_Entry>> GuildLog::For(cs::String savesDir, int32_t guild) {
    return All(savesDir).Where([&](auto& e) { return e->Guild == guild; }).Reversed().ToList();
}

cs::String GuildLog::Prefix(cs::Ref<GuildLog_Entry> e, int64_t today) {
    int64_t gap = cs::Math::Abs((int64_t)((uint64_t)e->Day - (uint64_t)today));
    if (e->Day == today) return "TODAY: ";
    return gap > 1 ? cs::ToStr(gap) + " DAYS AGO: " : cs::ToStr(gap) + " DAY AGO: ";
}

cs::String GuildLog::Line(cs::Ref<GuildLog_Entry> e, int64_t today) { return Prefix(e, today) + e->Text; }

bool GuildLog::NeedsCleaning(cs::String savesDir, int32_t guildCount) {
    return All(savesDir).Count() > cs::Wrap<int32_t>((int64_t)KeepOnClean * cs::Math::Max(1, guildCount));
}

void GuildLog::Append(cs::String savesDir, int32_t guild, cs::String text) {
    try {
        cs::Directory::CreateDirectory(savesDir);
        auto entries = All(savesDir);
        int64_t today = Today();
        entries.Add([&] { auto _o = cs::New<GuildLog_Entry>(); _o->Guild = guild; _o->Day = today; _o->Text = text; return _o; }());
        WriteAll(savesDir, entries);
    } catch (...) {
    }  // a log that cannot be written is not fatal
}

void GuildLog::Rewrite(cs::String savesDir, cs::List<cs::Ref<GuildLog_Entry>> entries) {
    try {
        cs::Directory::CreateDirectory(savesDir);
        WriteAll(savesDir, entries);
    } catch (...) {
    }
}

void GuildLog::WriteAll(cs::String savesDir, cs::List<cs::Ref<GuildLog_Entry>> entries) {
    cs::Bytes w = cs::Bytes::New();
    PutInt32(w, entries.Count());
    for (auto& e : entries) {
        auto bytes = cs::Encoding::UTF8GetBytes(e->Text);
        PutInt32(w, e->Guild);
        PutInt64(w, e->Day);
        PutInt32(w, bytes.Length());
        w.AddRange(bytes);
    }
    cs::File::WriteAllBytes(PathFor(savesDir), w);
}

int32_t GuildLog::Clean(cs::String savesDir, cs::String doneBy, cs::Func<cs::String(int32_t)> guildName, bool writeText) {
    auto entries = All(savesDir);
    if (entries.Count() <= KeepOnClean) return 0;

    int32_t cut = entries.Count() - KeepOnClean;
    auto discarded = entries.Take(cut).ToList();
    auto kept = entries.Skip(cut).ToList();

    try {
        cs::File::Copy(PathFor(savesDir), cs::Path::Combine(savesDir, BackupName), true);
    } catch (...) {
    }

    if (writeText) {
        try {
            auto sb = cs::New<cs::StringBuilder>();
            sb->AppendLine(" ");
            sb->AppendLine("BEGIN GUILD LOG CLEAN (" + TodayDate().ToString("d") + ")");
            sb->AppendLine("Done By: \"" + doneBy + "\"");
            sb->AppendLine(" ");
            for (auto& e : discarded) {
                sb->AppendLine("GUILD: " + guildName(e->Guild));
                sb->AppendLine(" DATE: " + FromDaySerial(e->Day).ToString("d"));
                sb->AppendLine("  LOG: " + e->Text);
            }
            AppendAllText(cs::Path::Combine(savesDir, TextName), sb->ToString());
        } catch (...) {
        }
    }

    WriteAll(savesDir, kept);
    return discarded.Count();
}

cs::String GuildLog_Wrote::Q(cs::String name) { return "\"" + cs::Str::TrimEnd(name) + "\""; }

cs::String GuildLog_Wrote::JoinedGuild(cs::String who) { return Q(who) + " joined our guild."; }

cs::String GuildLog_Wrote::Titled(cs::String who, cs::String title, int32_t level) {
    return Q(who) + " was appointed the title of \"" + title + "\" for reaching Level " + cs::ToStr(level) + ".";
}

cs::String GuildLog_Wrote::BecameGuildmaster(cs::String who) { return Q(who) + " became the new GuildMaster."; }

cs::String GuildLog_Wrote::AppointedGuildmaster(cs::String who) { return Q(who) + " was appointed GuildMaster."; }

cs::String GuildLog_Wrote::QuestForItem(cs::String who, cs::String item) {
    return Q(who) + " was assigned a quest to return with " + item + ".";
}

cs::String GuildLog_Wrote::QuestForMonster(cs::String who, cs::String monster) {
    return Q(who) + " was assigned a quest to kill one " + monster + ".";
}

cs::String GuildLog_Wrote::GuildmasterAward(cs::String who, cs::String item) {
    return Q(who) + " was awarded the '" + item + "' for achieving the rank of new Guildmaster.";
}

cs::String GuildLog_Wrote::Demoted(cs::String who, int32_t levels, Sex sex) {
    return Q(who) + " was demoted by " + cs::ToStr(levels) + " levels for not completing " + (sex == Sex::Male ? "his" : "her") + " quest.";
}

cs::String GuildLog_Wrote::SetRecord(cs::String who, cs::String record, cs::String value) {
    return Q(who) + " set the record for " + record + " with a value of " + value + ".";
}

cs::String GuildLog_Wrote::FirstToLevel(cs::String who, int32_t level) {
    return Q(who) + " was the first to go to level " + cs::ToStr(level) + ".";
}

cs::String GuildLog_Wrote::PartyBeatAsmodeus(cs::String who) {
    return Q(who) + " was part of the party that defeated Asmodeus.";
}

cs::String GuildLog_Wrote::AloneBeatAsmodeus(cs::String who) {
    return Q(who) + " single handedly defeated Asmodeus.";
}

cs::String GuildLog_Wrote::Resurrected(cs::String who, cs::String by) {
    return Q(who) + " was resurrected by " + Q(by) + ".";
}

cs::String GuildLog_Wrote::ResurrectedInMorgue(cs::String who) { return Q(who) + " was resurrected in the morgue."; }

cs::String GuildLog_Wrote::DiedOnLevel(cs::String who, int32_t level) {
    return Q(who) + " died on level " + cs::ToStr(level) + ".";
}

cs::String GuildLog_Wrote::DiedSomewhere(cs::String who) { return Q(who) + " died somewhere in the dungeon."; }

}  // namespace mc
