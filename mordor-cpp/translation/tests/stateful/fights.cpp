// C++ side of the stateful differential (mirrors cs/Program.cs; random draws sequenced in C# order).
#include "core.h"
#include <fstream>
using namespace mc;
int main(int, char** argv) {
    cs::String dir = argv[1]; int from = std::stoi(argv[2]), to = std::stoi(argv[3]); std::ofstream o(argv[4], std::ios::binary);
    Mdata1::LoadFrom(dir); RaceData::LoadFrom(dir); GuildData::LoadFrom(dir);
    auto items = MdrLoader::LoadItems(cs::Path::Combine(dir, "MDATA3.MDR"));
    auto monsters = MdrLoader::LoadMonsters(cs::Path::Combine(dir, "MDATA5.MDR"));
    auto spells = MdrLoader::LoadSpells(cs::Path::Combine(dir, "MDATA2.MDR"));
    cs::Func<cs::Ref<Item>(int16_t)> Look = [&](int16_t id) { return items.FirstOrDefault([&](auto& i) { return i->Id == id; }); };
    auto B = [](bool b) { return cs::String(b ? "True" : "False"); };
    auto C = [&](const cs::Ref<Character>& c) {
        cs::String g = cs::Str::Join(",", c->Guilds.Select([](auto& m) { return m->Guild + ":" + cs::ToStr(m->Level) + ":" + cs::ToStr(m->Experience); }));
        return c->Name + " hp " + cs::ToStr(c->CurHits) + "/" + cs::ToStr(c->MaxHits) + " sp " + cs::ToStr(c->CurSpellPoints) + " dead " + B(c->IsDead) + " gold " + cs::ToStr(c->GoldOnHand.Raw)
            + " xp " + g + " pois " + B(c->Poisoned) + " para " + cs::ToStr(c->ParalysisLevel); };
    for (int seed = from; seed < to; seed++) {
        auto rng = cs::New<cs::Random>(seed);
        try {
            auto party = cs::New<Party>();
            int n = rng->Next(1, 5);
            for (int k = 0; k < n; k++) {
                auto race = RaceData::All[rng->Next(RaceData::All.Count())];
                auto stats = cs::List<int32_t>::New(); for (int i = 0; i < 6; i++) stats.Add(rng->Next(8, 19));
                auto al = (Alignment)rng->Next(3); auto sx = (Sex)rng->Next(2);
                auto c = Character::Create("c" + std::to_string(seed) + "_" + std::to_string(k), race, al, sx, stats);
                auto g = GuildData::All[rng->Next(GuildData::All.Count())];
                bool strong = seed % 2 == 1;
                c->Guilds.Clear(); auto gm = cs::New<GuildMembership>(); gm->Guild = g->Name; gm->Level = strong ? rng->Next(20, 200) : rng->Next(1, 60); c->Guilds.Add(gm);
                c->MaxHits = strong ? rng->Next(500, 5000) : rng->Next(5, 400); c->CurHits = c->MaxHits; c->MaxSpellPoints = rng->Next(0, 300); c->CurSpellPoints = c->MaxSpellPoints;
                c->RecalcSwings();
                int gear = rng->Next(0, 3);
                for (int j = 0; j < gear; j++) { auto it = items[rng->Next(1, items.Count())]; auto s = cs::New<InventorySlot>(); s->ItemId = it->Id; s->Equipped = rng->Next(2) == 0; s->Charges = (int16_t)it->Charges; c->PutInPack(s); }
                party->Members.Add(c);
                auto ord = party->OrderFor(c);
                int kind = rng->Next(4);
                if (kind == 1) { ord->Kind = OrderKind::Spell; ord->SpellId = spells[rng->Next(1, spells.Count())]->Id; ord->Target = 0; }
                else if (kind == 2) ord->Kind = OrderKind::Defend;
            }
            int level = rng->Next(1, 16);
            auto enc = EncounterGenerator::Generate(monsters, level, rng);
            o << "FIGHT " << seed << " level " << level << " gold " << enc->Gold.ToString() << " class " << enc->TreasureClass << " trap " << enc->ChestTrap << " lock " << enc->ChestLock << " lead " << enc->Lead << "\n";
            for (auto& m : *enc) o << "  M " << (m->Def ? cs::ToStr(m->Def->Id) : "") << " " << (m->Def ? m->Def->Name : "") << " hits " << m->Hits() << "/" << m->MaxHits << " grp " << m->Group << "\n";
            auto fight = cs::New<CombatEngine>(party, *enc, Look, rng);
            fight->UseSpellTable(spells); fight->UseMonsterTable(monsters);
            o << "  start gold " << fight->GoldEarned << " chest " << fight->ChestClass << "\n";
            for (int r = 0; r < 40 && !fight->IsOver(); r++) {
                for (auto& l : fight->RoundPaced(spells)) o << "  R" << r << " " << l.Text << "\n";
                for (auto& c : party->Members) o << "  S" << r << " " << C(c) << "\n";
                o << "  F" << r << " " << cs::Str::Join(" ", fight->Monsters.Select([](auto& m) { return cs::ToStr(m->Hits()); })) << "\n";
            }
            if (fight->Won()) {
                fight->AwardSpoils();
                for (auto& l : fight->TakeSaid()) o << "  W " << l.Text << "\n";
                for (auto& c : party->Members) o << "  S* " << C(c) << "\n";
            }
            int64_t gold = fight->GoldEarned; bool over = fight->IsOver(), won = fight->Won(), wiped = fight->PartyWiped();
            o << "  END over " << B(over) << " won " << B(won) << " wiped " << B(wiped) << " gold " << gold << " rng " << rng->Next() << "\n";
        } catch (const std::exception& e) { o << "  THREW " << typeid(e).name() << " " << e.what() << "\n"; }
    }
}
