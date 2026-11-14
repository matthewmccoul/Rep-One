// Round trip with the C++ translation: Deserialize<T> then Serialize, as Core's own Load/Save do.
#include "core.h"
#include <fstream>
using namespace mc;
template <class T> cs::String RT(const cs::String& text, const cs::JsonSerializerOptions& o) { return cs::Json::Serialize(cs::Json::Deserialize<T>(text, o), o); }
int main(int, char** argv) {
    cs::String dir = argv[1], cases = argv[2], out = argv[3];
    Mdata1::LoadFrom(dir); RaceData::LoadFrom(dir); GuildData::LoadFrom(dir);
    cs::Directory::CreateDirectory(out);
    auto ind = cs::Json::Indented(); cs::JsonSerializerOptions help; help.DefaultIgnoreCondition = cs::Json::JsonIgnoreCondition::WhenWritingNull;
    for (auto& line : cs::File::ReadAllLines(cs::Path::Combine(cases, "cases.txt"))) {
        auto p = cs::Str::Split(line, '\t'); cs::String t = p[0], text = cs::File::ReadAllText(cs::Path::Combine(cases, p[1])), res;
        try {
            if (t == "Party+Roster") res = RT<cs::Ref<Party_Roster>>(text, ind);
            else if (t == "StoreStock") res = RT<cs::Ref<StoreStock>>(text, ind);
            else if (t == "Confinement") res = RT<cs::Ref<Confinement>>(text, ind);
            else if (t == "SharedMap") res = RT<cs::Ref<SharedMap>>(text, ind);
            else if (t == "Guildmasters") res = RT<cs::Ref<Guildmasters>>(text, ind);
            else if (t == "List<System.String>") res = RT<cs::List<cs::String>>(text, ind);
            else if (t == "DungeonState") res = RT<cs::Ref<DungeonState>>(text, ind);
            else if (t == "HallRecords") res = RT<cs::Ref<HallRecords>>(text, ind);
            else if (t == "Character") res = RT<cs::Ref<Character>>(text, ind);
            else if (t == "Librarian+Store") res = RT<cs::Ref<Librarian_Store>>(text, ind);
            else if (t == "List<Extraction.HelpBundle+Topic>") res = RT<cs::List<cs::Ref<HelpBundle_Topic>>>(text, help);
            else res = "UNKNOWN " + t;
        } catch (const cs::Exception& e) { res = cs::String("THREW ") + e.what(); }
        catch (const std::exception& e) { res = cs::String("THREW std ") + e.what(); }
        cs::File::WriteAllText(cs::Path::Combine(out, p[1]), res);
        if (t == "Character") {
            cs::String o2 = cs::Path::Combine(out, p[1] + ".load");
            try { Character::Load(cs::Path::Combine(cases, p[1]))->Save(o2); } catch (const std::exception& e) { cs::File::WriteAllText(o2, cs::String("THREW ") + e.what()); }
        }
    }
}
