// C++ side of the second differential (mirrors cs2/Program.cs; draws sequenced in C# order).
#include "core.h"
#include <fstream>
using namespace mc;
template <class F> static cs::String T(F f) {
    try { using R = decltype(f()); if constexpr (std::is_same<R, bool>::value) return f() ? "True" : "False"; else return cs::ToStr(f()); }
    catch (const std::exception& e) { cs::String n = typeid(e).name(); return "THREW " + n; } }
static const char* AL[] = {"Good", "Neutral", "Evil"};
static const char* FA[] = {"North", "East", "South", "West"};
int main(int, char** argv) {
    cs::String dir = argv[1]; int from = std::stoi(argv[2]), to = std::stoi(argv[3]); std::ofstream o(argv[4], std::ios::binary);
    Mdata1::LoadFrom(dir); RaceData::LoadFrom(dir); GuildData::LoadFrom(dir);
    auto items = MdrLoader::LoadItems(cs::Path::Combine(dir, "MDATA3.MDR"));
    auto monsters = MdrLoader::LoadMonsters(cs::Path::Combine(dir, "MDATA5.MDR"));
    auto floors = MdrLoader::LoadDungeon(cs::Path::Combine(dir, "MDATA11.MDR"));
    for (int seed = from; seed < to; seed++) {
        auto rng = cs::New<cs::Random>(seed);
        o << "SEED " << seed << "\n";
        auto race = RaceData::All[rng->Next(RaceData::All.Count())];
        auto stats = cs::List<int32_t>::New(); for (int i = 0; i < 6; i++) stats.Add(rng->Next(3, 26));
        auto al0 = (Alignment)rng->Next(3); auto sx = (Sex)rng->Next(2);
        auto c = Character::Create("w" + std::to_string(seed), race, al0, sx, stats);
        c->Guilds.Clear();
        int ng = rng->Next(1, 4);
        for (int i = 0; i < ng; i++) { auto gm = cs::New<GuildMembership>(); gm->Guild = GuildData::All[rng->Next(GuildData::All.Count())]->Name; gm->Level = rng->Next(1, 5); c->Guilds.Add(gm); }
        for (int step = 0; step < 25; step++) {
            if (rng->Next(4) == 0 && c->Guilds.Count() > 1) { auto g0 = c->Guilds[rng->Next(c->Guilds.Count())]; c->Guilds.Remove(g0); c->Guilds.Insert(0, g0); }
            c->Guild()->Experience += rng->Next(0, 3000000);
            int made = 0;
            while (made < 60) { bool can; try { can = c->CanLevelUp(); } catch (const std::exception& e) { o << "  can THREW " << typeid(e).name() << "\n"; break; } if (!can) break;
                cs::String g = c->Guild()->Guild; cs::String r = T([&] { return c->LevelUp(rng); }); o << "  L " << g << " -> " << r << " hp " << c->MaxHits << "\n"; made++; }
            o << "  C " << cs::Str::Join(",", c->Guilds.Select([](auto& g) { return g->Guild + ":" + cs::ToStr(g->Level) + ":" + cs::ToStr(g->Experience); }))
              << " next " << T([&] { return c->ExpForNextLevel(); }) << " total " << T([&] { return c->TotalLevel(); }) << " title " << T([&] { return c->Title(); })
              << " gt " << T([&] { return c->GuildTitle(); }) << " swings " << T([&] { c->RecalcSwings(); return c->Swings; }) << "\n";
        }
        auto s = cs::New<StoreStock>();
        for (int k = 0; k < 40; k++) {
            auto it = items[rng->Next(1, items.Count())]; auto al = (ItemAlignment)rng->Next(3); int op = rng->Next(6);
            auto slot = cs::New<InventorySlot>(); slot->ItemId = it->Id; slot->Charges = (int16_t)rng->Next(0, std::max(1, (int)it->Charges + 1)); slot->set_Known((IdLevel)rng->Next(4)); slot->Alignment = al;
            cs::String r;
            if (op == 0) r = T([&] { s->Add(it->Id, al); return s->Count(it->Id, al); });
            else if (op == 1) r = T([&] { return s->Take(it->Id, al); });
            else if (op == 2) r = T([&] { return s->PriceOf(it, al); });
            else if (op == 3) r = T([&] { return s->SellPrice(it, slot); });
            else if (op == 4) r = T([&] { return s->IdentifyFee(it, slot); });
            else r = T([&] { auto to2 = (ItemAlignment)rng->Next(3); return StoreStock::RealignCost(it, al, to2); });
            o << "  B " << it->Id << " " << ToStr(al) << " " << op << " " << r << "\n";
        }
        auto conf = cs::New<Confinement>();
        int days = rng->Next(0, 30); conf->Replenish(monsters, days, rng);
        o << "  P " << cs::Str::Join(",", conf->Pens.Pairs().Select([](auto& kv) { return kv.Key + "=" + cs::ToStr(kv.Value); })) << "\n";
        int fi = rng->Next(floors.Count()); auto floor = floors[fi];
        int sx0 = rng->Next(floor->Width); int sy0 = rng->Next(floor->Height); auto f0 = (Facing)rng->Next(4);
        auto nav = cs::New<DungeonNav>(floor, sx0, sy0, f0);
        for (int k = 0; k < 150; k++) {
            int a = rng->Next(6); cs::String r;
            if (a == 0) r = T([&] { return nav->MoveForward(); }); else if (a == 1) r = T([&] { return nav->MoveBackward(); });
            else if (a == 2) r = T([&] { nav->TurnLeft(); return 1; }); else if (a == 3) r = T([&] { nav->TurnRight(); return 1; });
            else if (a == 4) r = T([&] { nav->TurnAround(); return 1; }); else r = T([&] { return nav->StepThrough(); });
            o << "  N " << a << " " << r << " " << nav->X << "," << nav->Y << " " << ToStr(nav->Facing) << "\n";
            if (k % 25 == 0) {
                o << "  D " << cs::Str::Replace(T([&] { return nav->Describe(); }), "\n", "|") << "\n";
                try { auto e = EncounterGenerator::Generate(monsters, fi + 1, rng, floor, nav->X, nav->Y);
                      o << "  E " << e->Gold.ToString() << " " << e->TreasureClass << " " << e->Lead << " " << cs::Str::Join(";", e->Select([](auto& m) { return (m->Def ? cs::ToStr(m->Def->Id) : cs::String()) + ":" + cs::ToStr(m->Hits()) + ":" + cs::ToStr(m->Group); })) << "\n"; }
                catch (const std::exception& e) { o << "  E THREW " << typeid(e).name() << "\n"; }
            }
        }
        o << "  END rng " << rng->Next() << " explored " << nav->Explored.Count() << "\n";
    }
}
