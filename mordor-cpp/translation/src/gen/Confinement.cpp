#include "core.h"

namespace mc {

cs::String Confinement::PathFor(cs::String savesDir) { return cs::Path::Combine(savesDir, "confinement.json"); }

cs::String Confinement::Key(int32_t tableIndex, ItemAlignment align) {
    return cs::ToStr(tableIndex) + ":" + cs::ToStr(static_cast<int32_t>(align));
}

cs::Nullable<int32_t> Confinement::RefOf(cs::Ref<Monster> def) {
    return def->Ref >= 0 ? cs::Nullable<int32_t>(def->Ref) : cs::Nullable<int32_t>{};
}

cs::List<Tup_Ref_Align_Count_8a8ac> Confinement::Entries() {
    cs::List<Tup_Ref_Align_Count_8a8ac> _result = cs::List<Tup_Ref_Align_Count_8a8ac>::New();
    for (auto& kv : Pens) {
        auto p = Parse(kv.Key);
        if (!p.has_value()) continue;
        _result.Add(Tup_Ref_Align_Count_8a8ac{p.value().Ref, p.value().Align, kv.Value});
    }
    return _result;
}

cs::Nullable<Tup_Ref_Align_38db7> Confinement::Parse(cs::String key) {
    int32_t colon = cs::Str::IndexOf(key, ':');
    int32_t r = 0;
    int32_t a = 0;
    if (colon < 0 || !cs::Parse::TryParse(cs::Str::Substring(key, 0, colon), r) || !cs::Parse::TryParse(cs::Str::Substring(key, colon + 1), a)) return {};
    return Tup_Ref_Align_38db7{r, static_cast<ItemAlignment>(a)};
}

cs::Ref<Confinement> Confinement::Load(cs::String savesDir, cs::List<cs::Ref<Monster>> monsters) {
    cs::String p = PathFor(savesDir);
    if (!cs::File::Exists(p)) {
        auto fresh = Fresh(monsters);
        fresh->Save(savesDir);
        return fresh;
    }
    auto pens = Load(savesDir);
    if (pens->_idKeyed) pens->FromIdKeys(monsters);
    pens->RepairUnaligned(monsters);
    return pens;
}

int32_t Confinement::FromIdKeys(cs::List<cs::Ref<Monster>> monsters) {
    auto old = Pens;
    Pens = cs::Dictionary<cs::String, int32_t>::New();
    int32_t moved = 0;
    for (auto& kv : old) {
        auto key = kv.Key;
        auto n = kv.Value;
        auto parsedOpt = Parse(key);
        if (!parsedOpt.has_value()) continue;
        auto parsed = parsedOpt.value();
        auto idNum = parsed.Ref;
        auto align = parsed.Align;
        int16_t id = static_cast<int16_t>(idNum);
        auto withId = monsters.Where([&](auto& m) { return m->Id == id; }).ToList();
        auto same = withId.Where([&](auto& m) { return AlignmentOf(m) == align; }).ToList();
        // one creature; else the one whose own alignment the pen has; else the first with the Id
        auto to = withId.Count() == 1 ? withId : same.Count() == 1 ? same : withId.Take(1).ToList();
        for (auto& m : to) {
            cs::String k = Key(CompanionRef::IndexOf(monsters, m), align);
            Pens[k] = cs::Wrap<int32_t>((int64_t)Pens.GetValueOrDefault(k) + n);
        }
        if (to.Count() > 0) moved++;
    }
    KeyedBy = ByRef;
    _idKeyed = false;
    return moved;
}

int32_t Confinement::RepairUnaligned(cs::List<cs::Ref<Monster>> monsters) {
    int32_t moved = 0;
    for (auto& key : Pens.Keys().Where([&](auto& k) { return cs::Str::EndsWith(k, ":0"); }).ToList()) {
        int32_t n = Pens[key];
        auto pOpt = Parse(key);
        if (!pOpt.has_value()) continue;
        auto p = pOpt.value();
        if (n <= 0) { Pens.Remove(key); continue; }
        auto def = CompanionRef::At(monsters, p.Ref);
        if (def == nullptr) continue;
        Pens.Remove(key);
        cs::String to = Key(p.Ref, AlignmentOf(def));
        int32_t there = Pens.GetValueOrDefault(to);
        Pens[to] = cs::Math::Max(there, cs::Math::Min(ShelfCap, cs::Wrap<int32_t>((int64_t)there + n)));
        moved = cs::Wrap<int32_t>((int64_t)moved + n);
    }
    return moved;
}

cs::Ref<Confinement> Confinement::Load(cs::String savesDir) {
    try {
        cs::String p = PathFor(savesDir);
        if (cs::File::Exists(p)) {
            auto read = cs::Json::Deserialize<cs::Ref<Confinement>>(cs::File::ReadAllText(p));
            if (read != nullptr) {
                read->_idKeyed = read->KeyedBy != ByRef;
                return read;
            }
        }
    } catch (...) { }
    return cs::New<Confinement>();
}

void Confinement::Save(cs::String savesDir) {
    try {
        if (_idKeyed) return;   // Id keys never go back out labelled as table indices
        cs::Directory::CreateDirectory(savesDir);
        KeyedBy = ByRef;   // every pen this port makes is keyed by table index
        cs::File::WriteAllText(PathFor(savesDir), cs::Json::Serialize(*this));
    } catch (...) { }
}

int32_t Confinement::Count(cs::Ref<Monster> def, ItemAlignment align) {
    auto r = RefOf(def);
    return r.has_value() ? Count(r.value(), align) : 0;
}

int32_t Confinement::Count(int32_t tableIndex, ItemAlignment align) { return Pens.GetValueOrDefault(Key(tableIndex, align)); }

bool Confinement::AnyOf(cs::Ref<Monster> def) {
    return ItemAlignments::All.Any([&](auto& a) { return Count(def, a) > 0; });
}

void Confinement::Take(cs::Ref<Monster> def, ItemAlignment align) {
    // no floor: :141 subtracts one whatever is there, so a pen can go to -1
    auto r = RefOf(def);
    if (!r.has_value()) return;
    cs::String k = Key(r.value(), align);
    Pens[k] = cs::Wrap<int32_t>((int64_t)Pens.GetValueOrDefault(k) - 1);
}

void Confinement::Add(cs::Ref<Monster> def, ItemAlignment align) {
    if (def->LevelFound == 0) return;
    auto r = RefOf(def);
    if (!r.has_value()) return;
    cs::String k = Key(r.value(), align);
    int32_t n = Pens.GetValueOrDefault(k);
    if (n < ShelfCap) Pens[k] = n + 1;
}

double Confinement::Cur(double v) { return cs::Math::Round(v, 4, cs::MidpointRounding::ToEven); }

int64_t Confinement::Lng(double v) { return cs::CastDouble<int64_t>(cs::Math::Round(v, cs::MidpointRounding::ToEven)); }

double Confinement::BaseValue(cs::Ref<Monster> def) {
    double v = 500.0 * def->Att;
    v += 500.0 * def->Def;
    v += 250.0 * def->Hits;
    // Stats(m01DC) with m01DC a Static never assigned, so Stats(0), plus Stats(5). The Max(1, ..)
    // is the port's: the original's Log(0) would raise an error here.
    int32_t str = def->Stats.Length() > 0 ? def->Stats[0] : 0;
    int32_t dex = def->Stats.Length() > 5 ? def->Stats[5] : 0;
    v = Cur(v * cs::Math::Log(cs::Math::Max(1, str + dex)));
    v = Cur(v * cs::Math::Log(def->GuildLevel + 1));

    int32_t gl = def->GuildLevel;
    auto Special = [&](int32_t bit, int32_t rate) { if ((def->SpecialAttackFlags & (1 << bit)) != 0) v += cs::Wrap<int32_t>((int64_t)rate * gl + 1); };
    Special(3, 3250); Special(4, 3250); Special(5, 2050); Special(6, 2050);
    Special(8, 1500); Special(10, 1500); Special(11, 500);

    if (def->SpellFlags > 0) {
        auto Spell = [&](int32_t bit, int32_t rate) { if ((def->SpellFlags & (1 << bit)) != 0) v += cs::Wrap<int32_t>((int64_t)rate * gl + 1); };
        Spell(0, 5250); Spell(1, 3250); Spell(2, 4250); Spell(3, 3250);
        Spell(4, 5250); Spell(5, 5250); Spell(6, 4250);
    }

    for (int32_t r = 0; r < 11 && r < def->Resistances.Length(); r++)
        if (def->Resistances[r] > 0) v += 500.0 * def->Resistances[r];
    if (def->Hits == 1) v = 1;
    return v;
}

int64_t Confinement::PriceToBuy(cs::Ref<Monster> def, ItemAlignment align) {
    double value = BaseValue(def);
    if (value < 10) value = 10;
    int32_t stock = cs::Math::Min(Count(def, align) - 1, ShelfCap);
    double price = (1.5 * value) / (cs::Math::Log(stock + 1) + 1.15);
    // with none penned the original's Log(0) falls into its error trap and the price is 0
    return std::isfinite(price) ? Lng(price) : 0;
}

int64_t Confinement::PriceToSell(cs::Ref<Monster> def, ItemAlignment align, int32_t idLevel) {
    int32_t stock = Count(def, align);
    if (stock == 0 && AnyOf(def)) stock = 1;
    if (stock > ShelfCap) stock = ShelfCap;
    double value = BaseValue(def);
    int64_t v = Lng(((0.9 * value) / (stock + 1.15)) / (idLevel + 1));
    return v < 0 ? 0 : v;
}

int64_t Confinement::PriceToIdentify(cs::Ref<Monster> def, ItemAlignment align, int32_t idLevel) {
    if (idLevel <= 0) return 0;
    int32_t stock = cs::Math::Min(Count(def, align), ShelfCap);
    double value = BaseValue(def);
    int64_t v = Lng((0.13 * value) / ((stock / 2.0 + 1) * 0.9) / idLevel + 1);
    return v < 0 ? 0 : v;
}

int64_t Confinement::PriceToRealign(cs::Ref<Monster> def, ItemAlignment from, ItemAlignment to) {
    return Lng(0.72 * BaseValue(def) * (cs::Math::Abs(static_cast<int32_t>(from) - static_cast<int32_t>(to)) > 1 ? 2 : 1));
}

int32_t Confinement::BoughtBinding(cs::Ref<Monster> def) { return cs::Math::Min(10, 5 + def->LevelFound); }

cs::String Confinement::RealignOffer(int64_t cost) {
    return "Cost to realign is " + cs::Format(cost, "N0") + ".\nDo you wish to pay?";
}

cs::Ref<Confinement> Confinement::Fresh(cs::List<cs::Ref<Monster>> monsters) {
    auto pens = [&] { auto _o = cs::New<Confinement>(); _o->KeyedBy = ByRef; return _o; }();
    auto first = EncounterGenerator::ClassRanges(monsters);
    int32_t kinds = first.Count();
    auto Start = [&](int32_t kind) { return kind < kinds ? first[kind][0] : -1; };
    // MAINMOD.BAS:544 - kinds 0, 7, 8 and 14 and the index after each, kinds 1 and 9 alone
    auto picked = cs::List<int32_t>::Of({Start(0), Start(0) + 1, Start(7), Start(7) + 1, Start(8), Start(8) + 1,
                                         Start(14), Start(14) + 1, Start(1), Start(9)});
    for (int32_t i = 0; i < monsters.Count(); i++)
        if (picked.Contains(i))
            pens->Pens[Key(i, AlignmentOf(monsters[i]))] = 2;   // Align(Monsters(i).Alignment) = 2 (:551)
    return pens;
}

ItemAlignment Confinement::AlignmentOf(cs::Ref<Monster> m) {
    switch (m->AlignmentValue) {
        case 0: return ItemAlignment::Good;
        case 1: return ItemAlignment::Neutral;
        case 2: return ItemAlignment::Evil;
        default: return ItemAlignment::Neutral;
    }
}

void Confinement::Replenish(cs::List<cs::Ref<Monster>> monsters, int32_t days, cs::Ref<cs::Random> rng) {
    Replenish(monsters, days, rng, StoreStock::DeepestStocked);
}

void Confinement::Replenish(cs::List<cs::Ref<Monster>> monsters, int32_t days, cs::Ref<cs::Random> rng, int32_t deepestReached) {
    int32_t deepest = cs::Math::Clamp(deepestReached, 1, (int32_t)StoreStock::DeepestStocked);
    double chance = cs::Math::Clamp(days, 1, 50);

    auto byKind = monsters.Where([&](auto& m) { return m->Id != 0; }).GroupBy([&](auto& m) { return static_cast<int32_t>(m->Type); })
                          .ToDictionary([&](auto& g) { return g.Key; },
                                        [&](auto& g) { return g.Items.OrderBy([&](auto& m) { return m->LevelFound; }).ToList(); });
    int32_t kinds = byKind.Count() == 0 ? 0 : byKind.Keys().Max() + 1;
    if (kinds == 0) return;

    int32_t passes = 0;
    int32_t l0288 = 0;          // the one pen index both paths share, 0 at the start of each call
    while (rng->NextDouble() * 100 <= chance && passes++ < 500) {
        bool adding = rng->NextDouble() * 100 < 60;

        int32_t roll = rng->Next(100) + 1, floor = 0;
        for (int32_t f = 1; f <= deepest; f++) {
            if (roll < 100 - (f / (double)(deepest + 1)) * 100) floor = f; else break;
        }
        floor = cs::Math::Clamp(floor, 1, deepest);

        bool done = false;
        for (int32_t tries = 0; !done && tries < 100; tries++) {
            auto chosen = DrawOne(byKind, kinds, floor, rng);
            if (chosen == nullptr) continue;
            done = adding ? PenOne(chosen, l0288) : ReleaseOne(chosen, rng, l0288);
        }
    }
}

cs::Ref<Monster> Confinement::DrawOne(cs::Dictionary<int32_t, cs::List<cs::Ref<Monster>>> byKind, int32_t kinds, int32_t floor, cs::Ref<cs::Random> rng) {
    // MAINMOD.BAS:1337 - rounded draw like the item store at :1598; see StoreStock.DrawOne
    int32_t kind = Rules::RoundedIndex(rng, kinds);
    cs::List<cs::Ref<Monster>> all{};
    if (!byKind.TryGetValue(kind, all)) return nullptr;
    auto run = all.Where([&](auto& m) { return m->LevelFound <= floor; }).ToList();
    if (run.Count() == 0) return nullptr;

    int32_t total = run.Sum([&](auto& m) { return static_cast<int32_t>(m->Chance); });
    if (total <= 0) return nullptr;
    double pick = rng->NextDouble() * 100, acc = 0;
    cs::Ref<Monster> chosen = nullptr;
    for (auto& m : run) {
        acc += m->Chance / (double)total * 100;
        if (pick <= acc) { chosen = m; break; }
    }
    // MAINMOD.BAS:1371 - a creature from the town level is never penned by anyone else
    return chosen != nullptr && chosen->LevelFound > 0 ? chosen : nullptr;
}

ItemAlignment Confinement::PenIndex(int32_t i) { return static_cast<ItemAlignment>(i + 1); }

bool Confinement::PenOne(cs::Ref<Monster> m, int32_t l0288) {
    if (Count(m, PenIndex(l0288)) >= ShelfCap) return true;
    auto a = AlignmentOf(m);
    auto r = RefOf(m);
    if (r.has_value()) Pens[Key(r.value(), a)] = Count(m, a) + 1;
    return true;
}

bool Confinement::ReleaseOne(cs::Ref<Monster> m, cs::Ref<cs::Random> rng, int32_t& l0288) {
    l0288 = 0;
    if (Count(m, PenIndex(l0288)) <= 0) return false;
    do l0288 = rng->Next(4); while (Count(m, PenIndex(l0288)) == 0);
    Take(m, PenIndex(l0288));
    return true;
}

}  // namespace mc
