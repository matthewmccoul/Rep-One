// C++ side of the probe (same output as cs/Program.cs).
#include "core.h"
#include <fstream>
using namespace mc;
static cs::String H(const cs::Bytes& b) { return cs::Convert::ToHexString(cs::SHA256::HashData(b)).substr(0, 16); }
static cs::String DecText(const cs::Decimal& d) { return d.ToString(); }
int main(int, char** argv) {
    cs::String inDir = argv[1], game = argv[2]; std::ofstream o(argv[3], std::ios::binary);
    int k = 0;
    for (auto& s : cs::File::ReadAllLines(cs::Path::Combine(inDir, "numbers.txt"))) {
        int32_t cl = 0; bool a = VbConvert::TryCLng(s, cl);
        VbCurrency cur{}; bool b = VbCurrency::TryParse(s, cur);
        auto g = GiveCommand::CLng(s);
        double dv = 0; bool d = cs::Parse::TryParse(s, cs::Parse::Float | cs::Parse::AllowThousands, cs::Parse::InvariantCulture(), dv);
        cs::Decimal mv; bool m = cs::Parse::TryParse(s, cs::Parse::Number, cs::Parse::InvariantCulture(), mv);
        o << "N" << k++ << " " << (a ? std::to_string(cl) : "F") << " " << (b ? std::to_string(cur.Raw) : "F") << " " << (g ? std::to_string(*g) : "null") << " "
          << (d ? std::to_string(cs::BitConverter::DoubleToInt64Bits(dv)) : "F") << " " << (m ? DecText(mv) : "F") << "\n";
    }
    auto files = cs::Directory::GetFiles(inDir).OrderBy([](auto& x) { return x; });
    for (auto& f : files) { auto n = cs::Path::GetFileName(f); if (n.rfind("zip_", 0) != 0) continue;
        cs::String r;
        try { auto c = InstallerUnpack::Parse(cs::File::ReadAllBytes(f));
              r = cs::Str::Join(",", c->Files.Select([](auto& e) { return e->RelativePath + ":" + std::to_string(e->Data.Count()) + ":" + H(e->Data); })) + " | " + cs::Str::Join(",", c->Skipped); }
        catch (const std::exception& e) { r = cs::String("THREW ") + typeid(e).name(); }
        o << "Z " << n << " " << r << "\n"; }
    for (auto& f : files) { auto n = cs::Path::GetFileName(f); if (n.rfind("png_", 0) != 0) continue;
        cs::String r;
        try { auto img = Png::Decode(cs::File::ReadAllBytes(f)); cs::Bytes px = cs::Bytes::New();
              for (auto p : img->Pixels) for (int i = 0; i < 4; i++) px.Add((uint8_t)(p >> (8 * i)));
              r = std::to_string(img->Width) + "x" + std::to_string(img->Height) + " " + H(px) + " enc " + H(Png::Encode(img)); }
        catch (const std::exception& e) { r = cs::String("THREW ") + typeid(e).name(); }
        o << "P " << n << " " << r << "\n"; }
    auto rec = GameFiles::Recognise(cs::Path::Combine(game, "MORDOR.EXE"));
    o << "K " << GameArt::Key(game) << " " << (rec.empty() ? "null" : rec) << "\n";
}
