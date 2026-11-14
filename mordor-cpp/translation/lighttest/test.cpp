// Light test of the C++ translation against C# Mordor.Core reference results (ref.txt).
#include "core.h"
#include <cstdio>
#include <fstream>
#include <map>
using namespace mc;
static std::string V(const cs::String& s) { return "\"" + s + "\""; }
static std::string V(float f) { return "f" + std::to_string(cs::BitConverter::SingleToInt32Bits(f)); }
static std::string V(double d) { return "d" + std::to_string(cs::BitConverter::DoubleToInt64Bits(d)); }
static std::string V(bool b) { return b ? "1" : "0"; }
template <class I, std::enable_if_t<std::is_integral<I>::value, int> = 0> static std::string V(I i) { return std::to_string((long long)i); }
template <class E, std::enable_if_t<std::is_enum<E>::value, int> = 0> static std::string V(E e) { return std::to_string((long long)e); }
template <class T> static std::string V(const cs::List<T>& l) { if (l.IsNull()) return "null"; std::string s = "["; bool f = true; for (auto& x : l) { if (!f) s += ","; s += V(x); f = false; } return s + "]"; }
#include "dumps.inc"
static double D(const std::string& s) { return cs::BitConverter::Int64BitsToDouble(std::stoll(s)); }
static float F(const std::string& s) { return cs::BitConverter::Int32BitsToSingle(std::stoi(s)); }
int main(int, char** argv) {
    std::string dir = argv[1]; std::ifstream in(argv[2]);
    Mdata1::LoadFrom(dir); RaceData::LoadFrom(dir); GuildData::LoadFrom(dir);
    auto items = MdrLoader::LoadItems(cs::Path::Combine(dir, "MDATA3.MDR"));
    auto monsters = MdrLoader::LoadMonsters(cs::Path::Combine(dir, "MDATA5.MDR"));
    auto spells = MdrLoader::LoadSpells(cs::Path::Combine(dir, "MDATA2.MDR"));
    std::map<std::string, std::pair<int, int>> tally; std::map<std::string, int> shown;
    size_t iI = 0, iM = 0, iS = 0;
    auto check = [&](const std::string& k, const std::string& want, const std::string& got) {
        auto& t = tally[k]; t.second++; if (want == got) { t.first++; return; }
        if (shown[k]++ < 3) std::printf("MISMATCH %s\n  C#:  %.300s\n  C++: %.300s\n", k.c_str(), want.c_str(), got.c_str()); };
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line); std::string k; ss >> k;
        try {
        if (k == "P1A") { int lv; std::string g, r; long long want; ss >> lv >> g >> r >> want; check(k, std::to_string(want), std::to_string(Rules::Fn12A1(lv, F(g), F(r)))); }
        else if (k == "P1B") { int lv, gold; long long want; ss >> lv >> gold >> want; check(k, std::to_string(want), std::to_string(Rules::Fn12B2(lv, gold))); }
        else if (k == "P1P") { int id, price, floor, charges, n; long long want; ss >> id >> price >> floor >> charges >> n >> want;
            auto it = cs::New<Item>(); it->Id = (int16_t)id; it->Price = price; it->Floor = (int16_t)floor; it->Charges = charges;
            auto s = cs::New<StoreStock>(); for (int i = 0; i < n; i++) s->Add(it->Id, ItemAlignment::Neutral);
            check(k, std::to_string(want), std::to_string(s->PriceOf(it, ItemAlignment::Neutral))); }
        else if (k == "P2I") check(k, line, iI < (size_t)items.Count() ? DumpItem(items[(int)iI]) : "(missing)"), iI++;
        else if (k == "P2M") check(k, line, iM < (size_t)monsters.Count() ? DumpMonster(monsters[(int)iM]) : "(missing)"), iM++;
        else if (k == "P2S") check(k, line, iS < (size_t)spells.Count() ? DumpSpell(spells[(int)iS]) : "(missing)"), iS++;
        else if (k == "P2G") { std::string name; long long want; ss >> name >> want; long long got = -1;
            for (auto& g : GuildData::All) if (g->Name == name) got = Rules::GuildJoinCost(g); check(k, std::to_string(want), std::to_string(got)); }
        else if (k == "P2V") {
            std::string a, b; ss >> a >> b; std::string rest; std::getline(ss, rest);
            auto ca = VbCurrency::FromDouble(D(a)); auto cb = VbCurrency::FromDouble(D(b));
            std::string got = " " + std::to_string(ca.Raw) + " " + std::to_string(cb.Raw) + " " + std::to_string(VbCurrency::AddSaturating(ca, cb).Raw) + " " + std::to_string(VbCurrency::SubtractSaturating(ca, cb).Raw)
                + " " + std::to_string(ca.RoundToWhole()) + " " + std::to_string(ca.Whole()) + " " + (ca.IsWhole() ? "1" : "0") + " [" + ca.Str() + "] [" + ca.ToString() + "] [" + ca.ToDecimal().ToString() + "]";
            check(k, rest, got); }
        } catch (const std::exception& e) { check(k, line, std::string("EXCEPTION: ") + e.what()); }
    }
    std::printf("\nloaded: %d items, %d monsters, %d spells\n", items.Count(), monsters.Count(), spells.Count());
    for (auto& [k, t] : tally) std::printf("%-4s %6d / %6d match\n", k.c_str(), t.first, t.second);
}
