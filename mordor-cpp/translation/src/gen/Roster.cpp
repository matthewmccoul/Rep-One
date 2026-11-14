#include "core.h"

namespace mc {

cs::String Roster::PathFor(cs::String savesDir) { return cs::Path::Combine(savesDir, "roster.json"); }

cs::List<cs::String> Roster::Records(cs::String savesDir) {
    try {
        cs::String p = PathFor(savesDir);
        if (cs::File::Exists(p)) {
            // JSON deserialization is not translated: the stub throws and is swallowed below
            auto l = cs::Json::Deserialize<cs::List<cs::String>>(cs::File::ReadAllText(p));
            return l ? l : cs::List<cs::String>::New();
        }
    } catch (...) {
    }
    return cs::List<cs::String>::New();
}

void Roster::Write(cs::String savesDir, cs::List<cs::String> records) {
    try {
        cs::Directory::CreateDirectory(savesDir);
        cs::JsonSerializerOptions _o{};
        _o.WriteIndented = true;
        (void)_o;
        // JSON serialization is not translated: the stub throws and is swallowed below
        cs::File::WriteAllText(PathFor(savesDir), cs::Json::Serialize(records));
    } catch (...) {
    }
}

bool Roster::Same(cs::String a, cs::String b) {
    return cs::Str::EqualsIgnoreCase(cs::Str::TrimEnd(a), cs::Str::TrimEnd(b));
}

void Roster::Added(cs::String savesDir, cs::String name, cs::List<cs::String> existing) {
    auto records = Records(savesDir);
    for (auto& e : existing)
        if (!Same(e, name) && cs::Str::Trim(e) != "" && !records.Any([&](auto& r) { return Same(r, e); })) records.Add(cs::Str::TrimEnd(e));
    if (records.Any([&](auto& r) { return Same(r, name); })) { Write(savesDir, records); return; }
    int32_t hole = records.FindIndex([&](auto& r) { return cs::Str::Trim(r) == ""; });
    if (hole >= 0) records[hole] = cs::Str::TrimEnd(name);
    else records.Add(cs::Str::TrimEnd(name));
    Write(savesDir, records);
}

cs::List<Tup_File_Ch_15a99> Roster::Saved(cs::String savesDir) {
    auto found = cs::List<Tup_File_Ch_15a99>::New();
    if (!cs::Directory::Exists(savesDir)) return found;
    for (auto& f : cs::Directory::GetFiles(savesDir, "*.json").OrderBy([&](auto& f) { return f; })) {
        if (cs::Str::StartsWith(cs::Path::GetFileName(f), "party-")) continue;
        try {
            auto c = Character::Load(f);
            if (!cs::Str::IsNullOrWhiteSpace(c->Name)) found.Add(Tup_File_Ch_15a99{f, c});
        } catch (...) {
        }
    }
    return Order<Tup_File_Ch_15a99>(savesDir, found, cs::Func<cs::String(Tup_File_Ch_15a99)>([&](Tup_File_Ch_15a99 p) { return p.Ch->Name; }));
}

void Roster::Removed(cs::String savesDir, cs::String name, cs::List<cs::String> existing) {
    auto records = Records(savesDir);
    for (auto& e : existing)
        if (cs::Str::Trim(e) != "" && !records.Any([&](auto& r) { return Same(r, e); })) records.Add(cs::Str::TrimEnd(e));
    for (int32_t i = 0; i < records.Count(); i++)
        if (Same(records[i], name)) records[i] = "";
    Write(savesDir, records);
}

}  // namespace mc
