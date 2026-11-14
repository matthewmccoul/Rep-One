#include "core.h"

namespace mc {

// _ranges is keyed by reference (C#'s ConditionalWeakTable): order the keys by list identity
static bool operator<(const cs::List<cs::Ref<Monster>>& a, const cs::List<cs::Ref<Monster>>& b) {
    return std::less<const void*>()(static_cast<const void*>(&a.Raw()), static_cast<const void*>(&b.Raw()));
}

// ------------------------------------------------------------------ CombatMonster

int32_t CombatMonster::Hits() { return _hits; }

void CombatMonster::set_Hits(int32_t value) { _hits = value; AtNought = false; }

void CombatMonster::LoadRecord(cs::Ref<cs::Random> rng) {
    int32_t hits = Def->Hits;
    int32_t sign = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * 2));
    int32_t swing = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * (Def->Hits * .5) / 2)) + 1;
    TakeRecord(sign == 0 ? hits - swing : hits + swing, 0);
    MaxHits = Hits();
}

void CombatMonster::TakeRecord(int32_t hits, int32_t maxHits) {
    set_Hits(hits);
    MaxHits = maxHits;
    Unrolled = false;
    AtNought = hits <= 0;
}

ItemAlignment CombatMonster::CompanionAlignment() {
    return Alignment >= 0 && Alignment <= 2
        ? static_cast<ItemAlignment>(Alignment + 1)
        : Confinement::AlignmentOf(Def);
}

cs::Ref<Monster> CombatMonster::Seen() { return Believed ? Believed : Def; }

bool CombatMonster::Alive() { return !Gone && (Hits() > 0 || AtNought); }

bool CombatMonster::SameGroup(cs::Ref<CombatMonster> a, cs::Ref<CombatMonster> b) {
    return a->Group >= 0 || b->Group >= 0 ? a->Group == b->Group : a->Def == b->Def;
}

cs::List<cs::List<cs::Ref<CombatMonster>>> CombatMonster::Runs(cs::List<cs::Ref<CombatMonster>> monsters) {
    auto groups = cs::List<cs::List<cs::Ref<CombatMonster>>>::New();
    for (auto& m : monsters)
        if (groups.Count() > 0 && SameGroup(groups[groups.Count() - 1][0], m)) groups[groups.Count() - 1].Add(m);
        else groups.Add(cs::List<cs::Ref<CombatMonster>>::Of({m}));
    return groups;
}

cs::List<cs::List<cs::Ref<CombatMonster>>> CombatMonster::BySlot(cs::List<cs::Ref<CombatMonster>> monsters) {
    auto slots = cs::List<cs::List<cs::Ref<CombatMonster>>>::New();
    for (auto& run : Runs(monsters)) {
        int32_t at = run[0]->Group >= slots.Count() ? run[0]->Group : slots.Count();
        while (slots.Count() < at) slots.Add(cs::List<cs::Ref<CombatMonster>>::New());
        slots.Add(run);
    }
    return slots;
}

// ------------------------------------------------------------------ MonsterProperty

bool MonsterProperty::Has(cs::Ref<Monster> m, int32_t bit) { return (m->SpecialPropertyFlags & (1 << bit)) != 0; }

// ------------------------------------------------------------------ FoeSlot

cs::Ref<FoeSlot> FoeSlot::Copy() { return cs::New<FoeSlot>(*this); }

// ------------------------------------------------------------------ RoomRecord

cs::Ref<RoomRecord> RoomRecord::Copy() {
    return [&] {
        auto _o = cs::New<RoomRecord>();
        _o->Foes = Foes.Select([&](auto& f) { return f->Copy(); }).ToArray();
        _o->TreasureClass = TreasureClass; _o->Lead = Lead; _o->Gold = Gold; _o->ChestTrap = ChestTrap; _o->ChestLock = ChestLock;
        return _o;
    }();
}

bool RoomRecord::AnyStanding() { return Foes.Any([&](auto& f) { return f->GroupCount > 0; }); }

// ------------------------------------------------------------------ Encounter

cs::Ref<RoomRecord> Encounter::ToRecord() {
    return [&] {
        auto _o = cs::New<RoomRecord>();
        _o->Foes = Foes.Select([&](auto& f) { return f->Copy(); }).ToArray();
        _o->TreasureClass = TreasureClass; _o->Lead = Lead; _o->Gold = Gold; _o->ChestTrap = ChestTrap; _o->ChestLock = ChestLock;
        return _o;
    }()->Copy();
}

void Encounter::Take(cs::Ref<RoomRecord> r, cs::List<cs::Ref<Monster>> all) {
    for (int32_t g = 0; g < Foes.Length(); g++) {
        cs::Ref<FoeSlot> s = g < r->Foes.Length() ? r->Foes[g] : cs::New<FoeSlot>();
        auto d = Foes[g];
        d->Ref = s->Ref; d->KnownAs = s->KnownAs; d->GroupCount = s->GroupCount; d->Att = s->Att; d->Def = s->Def;
        d->Hits = s->Hits; d->MaxHits = s->MaxHits; d->Alignment = s->Alignment; d->IdLevel = s->IdLevel;
        d->Engaged = s->Engaged; d->JoinOffer = s->JoinOffer; d->Stamp = s->Stamp;
    }
    TreasureClass = r->TreasureClass; Lead = r->Lead; Gold = r->Gold; ChestTrap = r->ChestTrap; ChestLock = r->ChestLock;
    LeadMonster = r->Lead >= 0 && r->Lead < all.Count() ? all[r->Lead] : nullptr;
}

// ------------------------------------------------------------------ EncounterGenerator

cs::Ref<Encounter> EncounterGenerator::InRoom(cs::List<cs::Ref<Monster>> all, cs::Ref<Floor> floor, int32_t level, int32_t x, int32_t y, cs::Ref<DungeonState> state, cs::DateTime now,
                                              cs::Ref<cs::Random> rng, cs::Ref<Character> viewer, cs::Ref<Character> idReader, cs::Func<cs::Ref<Item>(int16_t)> item) {
    bool _written = false;
    return InRoom(all, floor, level, x, y, state, now, rng, _written, viewer, idReader, item);
}

cs::Ref<Encounter> EncounterGenerator::InRoom(cs::List<cs::Ref<Monster>> all, cs::Ref<Floor> floor, int32_t level, int32_t x, int32_t y, cs::Ref<DungeonState> state, cs::DateTime now,
                                              cs::Ref<cs::Random> rng, bool& written, cs::Ref<Character> viewer, cs::Ref<Character> idReader,
                                              cs::Func<cs::Ref<Item>(int16_t)> item) {
    written = false;
    int32_t areaId = floor->AreaIdAt(x, y);
    if (areaId <= 0) return nullptr;
    int32_t header = floor->HeaderAreaCount;
    auto rec = state->LoadRoom(level, areaId);                                         // fn0E38 (687)
    double nowSec = DungeonState::SecondsSinceMidnight(now);
    double stamp = rec->Foes[0]->Stamp;
    cs::Ref<Encounter> e;
    if (state->RefillDue(level, areaId, header, now))                                 // 691-708
    {
        bool noneStanding = !rec->AnyStanding();                                        // l01B6, 710
        e = Generate(all, level, rng, floor, x, y, viewer, idReader, item, -1, nowSec);
        state->StoreRoom(level, areaId, e->ToRecord());                                // fn0F88 (1979)
        written = true;
        e->Fresh = !e->Failed;                                                         // 711-721
        if (!e->Failed && stamp != 0 && noneStanding) state->NoteRefilled(level, header); // 714-716
    }
    else
    {
        e = cs::New<Encounter>();
        e->Take(rec, all);
        Finish(e, all, rng);
    }
    e->Room = Tup_Level_Area_5bf4b{level, areaId};
    return e->Count() > 0 || e->TreasureClass > 0 ? e : nullptr;
}

cs::Ref<RoomRecord> EncounterGenerator::Capture(cs::Ref<RoomRecord> stored, cs::List<cs::Ref<CombatMonster>> members, cs::Ref<CombatEngine> fight, cs::List<cs::Ref<Monster>> all) {
    auto rec = stored->Copy();
    auto list = members.ToList();
    // slot 0's temper and offer are only the group's while it stands (M4EF9 = False, MONSMOD.BAS:777)
    bool slot0Standing = rec->Foes[0]->GroupCount > 0;
    for (int32_t g = 0; g < rec->Foes.Length(); g++) {
        auto s = rec->Foes[g];
        if (s->GroupCount <= 0) continue;
        auto standing = list.Where([&](auto& m) { return m->Group == g && m->Alive(); }).ToList();
        s->GroupCount = standing.Count();
        if (standing.Count() == 0) {
            // KnownAs stays what the group was recognised as (FRMDUNM.BAS:490, MONSMOD.BAS:775-777, 3840)
            auto last = list.FirstOrDefault([&](auto& m) { return m->Group == g && m->Believed != nullptr; });
            if (last)
                for (int32_t i = 0; i < all.Count(); i++) if (all[i] == last->Believed) { s->KnownAs = i; break; }
            continue;
        }
        auto front = standing[0];
        s->Hits = front->Hits();
        s->MaxHits = front->MaxHits;
        if (front->Believed != nullptr)
            for (int32_t i = 0; i < all.Count(); i++) if (all[i] == front->Believed) { s->KnownAs = i; break; }
        s->IdLevel = front->Identified ? 0 : cs::Math::Max(1, s->IdLevel);
    }
    if (slot0Standing && list.Count() > 0) {
        rec->Foes[0]->Engaged = list.Any([&](auto& m) { return m->Hostile; }) ? 1 : 0;
        auto f0 = list.FirstOrDefault([&](auto& m) { return m->Group == 0; });
        rec->Foes[0]->JoinOffer = f0 ? f0->JoinOffer : 0;
    }
    if (fight != nullptr) {
        rec->TreasureClass = fight->ChestClass;
        rec->ChestTrap = fight->ChestTrap;
        rec->ChestLock = fight->ChestLocked;
        // monGetGold empties Fight.Gold (2135)
        if (fight->GoldEarned == 0) rec->Gold = 0;
    }
    return rec;
}

int32_t EncounterGenerator::Int(double d) { return cs::CastDouble<int32_t>(cs::Math::Floor(d)); }

int32_t EncounterGenerator::CInt(double d) { return cs::CastDouble<int32_t>(cs::Math::Round(d, cs::MidpointRounding::ToEven)); }

int64_t EncounterGenerator::CLng(double d) { return cs::CastDouble<int64_t>(cs::Math::Round(d, cs::MidpointRounding::ToEven)); }

void EncounterGenerator::SetGold(cs::Ref<Encounter> e, double d) {
    VbCurrency v = VbCurrency::Zero;
    if (VbCurrency::TryFromDouble(d, v)) e->Gold = v.ToDecimal();
    else e->Messages.Add(GoldOverflowLine);
}

int32_t EncounterGenerator::ClassCount(cs::List<cs::Ref<Monster>> all) {
    return Mdata1::MonsterClasses.Count() > 0 ? Mdata1::MonsterClasses.Count()
        : all.Count() == 0 ? 0 : all.Max([&](auto& m) { return (int32_t)m->Type; }) + 1;
}

cs::List<cs::List<int32_t>> EncounterGenerator::ClassRanges(cs::List<cs::Ref<Monster>> all) {
    cs::List<cs::List<int32_t>> cached;
    if (_ranges.TryGetValue(all, cached) && cached.Count() == ClassCount(all)) return cached;
    int32_t nc = ClassCount(all), n = all.Count();
    auto r = cs::List<cs::List<int32_t>>::New(cs::Math::Max(nc, 1));
    for (int32_t _i = 0; _i < r.Count(); _i++) r[_i] = cs::List<int32_t>::New(2);
    for (int32_t c = 0; c < nc; c++) { r[c][0] = -1; r[c][1] = -1; }
    for (int32_t c = 0; c < nc; c++)
        for (int32_t i = 1; i <= n - 1; i++) {
            if (r[c][0] == -1 && all[i]->Type == c) r[c][0] = i;
            if (all[i]->Type != c && r[c][0] > -1) { r[c][1] = i - 1; i = n - 1; }
        }
    if (nc > 0) r[nc - 1][1] = n - 1;
    _ranges.Set(all, r);
    return r;
}

Tup_Below_Above_4f99c EncounterGenerator::StaleWindow() { return Tup_Below_Above_4f99c{_below, _above}; }

void EncounterGenerator::set_StaleWindow(Tup_Below_Above_4f99c value) { _below = value.Below; _above = value.Above; }

cs::Ref<Encounter> EncounterGenerator::Generate(cs::List<cs::Ref<Monster>> all, int32_t level, cs::Ref<cs::Random> rng, cs::Ref<Floor> floor, int32_t x, int32_t y,
                                                cs::Ref<Character> viewer, cs::Ref<Character> idReader, cs::Func<cs::Ref<Item>(int16_t)> item, int32_t forceLead, double timer) {
    auto Rnd = [&]() { return rng->NextDouble(); };
    auto RndF = [&]() { return (float)rng->NextDouble(); };   // Rnd is a Single; this is for the draws kept in m026A

    // sub0C79 clears Foes(0..3) and Fight (MONSMOD.BAS:1425, 794-804)
    auto e = cs::New<Encounter>();
    auto foes = e->Foes;
    int32_t n = all.Count(), nc = ClassCount(all);
    if (n == 0 || nc == 0) return e;
    auto range = ClassRanges(all);
    // v?0306, ReDim'd once per call (MONSMOD.BAS:1428), held as a double
    auto v = cs::List<double>::New(n);
    auto V = [&](int32_t i) -> double { return i >= 0 && i < n ? v[i] : 0; };   // past the end: Resume Next skips the add
    // m02EE, ReDim'd to NumMonsterClasses - 1: unfilled slots stay 0, which is class 0
    auto list = cs::List<int32_t>::New(nc);
    int32_t listN = 0;

    bool onSquare = floor != nullptr && x >= 0 && y >= 0 && x < floor->Width && y < floor->Height;
    auto Square = [&](int32_t bit) { return onSquare && floor->Has(x, y, bit); };
    int32_t areaId = onSquare ? floor->AreaIdAt(x, y) : ProbeAreaId;
    Floor_Area area = onSquare
        ? (areaId >= 0 && areaId < floor->Areas.Count() ? floor->Areas[areaId] : Floor_Area{})
        : [&] { Floor_Area _o{}; _o.SpawnMask = nc >= 31 ? -1 : (1 << nc) - 1; _o.LairId = 0; return _o; }();

    int32_t meLevel = level;
    int32_t meAlign = viewer != nullptr ? static_cast<int32_t>(viewer->Alignment) : static_cast<int32_t>(Alignment::Neutral);
    auto reader = idReader ? idReader : viewer;

    // m0282, the floor the pick aims at (MONSMOD.BAS:1431)
    int32_t m0282 = level;
    // v?031C: ROCK, bit 19 (1432-1436; MAZEMOD.BAS:1803)
    if (Square(Floor::ROCK)) return e;
    // v?031E: one floor deeper (1437-1439). STUD, bit 22
    bool stud = Square(Floor::STUD);
    if (stud) m0282++;
    int64_t mask = area.SpawnMask;   // m0266, M5D45 (1440-1441)

    int32_t lead = 0, cls = 0, m0284 = 0, spread = 0, m0272 = -1, m0274 = -1;
    int32_t m028C = 0, m028E = 0, m0290 = 0, m02A2 = 0;
    int32_t groups = 0;
    int32_t tries = 0;                                                                 // m02A0, 1430
    float r = 0, acc = 0, W = 0;         // m026A, m0288, m027E are Single (MONSMOD.BAS:1367-1378)
    // hoisted so the gotos do not jump over their initialisation
    bool newbie = false;
    bool classOk = false;

L1EB02:
    lead = -1; cls = -1; m0284 = -1;
    if (mask == 0) return e;                                                         // 1448-1452
    if (forceLead >= 0 && forceLead < n) { lead = forceLead; goto L1F688; }
    // the lair (1454-1463): gc1DC8 = 75
    if (area.LairId > 0)
    {
        if (RndF() * 100.0f <= 75)
        {
            for (int32_t i = 0; i <= n - 1; i++) if (all[i]->Id == area.LairId) { lead = i; break; }
            if (lead < 0) { e->Failed = true; return e; }
            goto L1F688;
        }
    }
    // 1465-1469: mc032C = 18
    newbie = meLevel == 1 && areaId < 18;
    // 1470-1485
    if (Int(Rnd() * 100) + 1 < 2 && !newbie)
        cls = Int(Rnd() * nc);
    else
    {
        listN = 0;
        for (int32_t c = 0; c <= nc - 1; c++)
            if (c < 63 && ((mask >> c) & 1) != 0 && range[c][0] != -1) list[listN++] = c;
        if (listN == 0) cls = Int(Rnd() * nc);
    }
    // 1486-1503
    spread = Rules::EncounterSpread(meLevel, rng, stud);

    // 1504-1557: which class
    if (cls == -1)
    {
        W = 0; int32_t count = 0;
        for (int32_t k = 0; k <= listN - 1; k++)
        {
            int32_t c = list[k];
            m0274 = range[c][1]; m0272 = -1;
            v[k] = 0;
            for (int32_t m = range[c][0]; m <= range[c][1]; m++)
            {
                if (m0272 == -1 && all[m]->LevelFound >= m0282 - spread) m0272 = m;
                if (all[m]->LevelFound > m0282)
                {
                    m0274 = m - 1;
                    if (m0274 < m0272) m0274 = m0272;
                    break;
                }
            }
            if (m0272 > -1 && m0274 > -1)
                for (int32_t m = m0272; m <= m0274; m++)
                {
                    int32_t f = all[m]->LevelFound;
                    if (f < m0282 - spread || f > m0282) continue;
                    W += all[m]->Chance; v[k] += all[m]->Chance; count++;
                }
        }
        if (W == 0 || count == 0)
            // UBound(m02EE) - 1 = NumMonsterClasses - 2 (1540)
            cls = list[CInt(Rnd() * (nc - 1 - 1))];
        else
        {
            // v(0) counted twice, each class tested against the one before it (1542-1552)
            r = RndF() * 101.0f;
            acc = (float)v[0];
            for (int32_t k = 0; k <= listN - 1; k++)
            {
                if (r <= acc / W * 100) { cls = list[k]; break; }
                if (r > acc / W * 100) acc = (float)(acc + v[k]);
            }
            if (cls == -1) cls = list[listN - 1];
        }
    }

L1F1B4:
    if (cls == -1) goto L1EB02;
    if (range[cls][0] < 0) goto L1EB02;
    m0274 = range[cls][1]; m0272 = -1;
    for (int32_t m = range[cls][0]; m <= range[cls][1]; m++)
    {
        if (m0272 == -1) m0272 = m;
        if (all[m]->LevelFound > m0282)
        {
            m0274 = m - 1;
            if (m0274 < m0272) m0274 = m0272;
            break;
        }
    }

L1F2A0:
    W = 0;
    if (m0272 == -1 || m0274 == -1) { e->Restarts++; goto L1EB02; }
    for (int32_t m = m0272; m <= m0274; m++)
    {
        int32_t f = all[m]->LevelFound;
        if (f < m0282 - spread || f > m0282) continue;
        W += all[m]->Chance;
    }
    if (W == 0)
    {
        // 1592-1599
        spread++;
        if (spread > 3) { e->Restarts++; goto L1EB02; }
        goto L1F1B4;
    }
    m0274 = range[cls][1]; m0272 = -1;
    for (int32_t m = range[cls][0]; m <= range[cls][1]; m++)
    {
        v[m] = 0;
        if (m0272 == -1) m0272 = m;
        int32_t f = all[m]->LevelFound;
        if (f < m0282 - spread) { }
        else if (f > m0282)
        {
            m0274 = m - 1;
            if (m0274 < m0272) m0274 = m0272;
            break;
        }
        else v[m] = all[m]->Chance / W * 100;
    }
    if (m0274 < m0272) m0274 = m0272;

L1F4F8:
    r = RndF() * 100.0f;
    if (m0272 == -1)
    {
        // reached from the group loop: L1F2A0 sends it straight back to L1EB02
        spread++;
        goto L1F2A0;
    }
    acc = (float)v[m0272];
    for (int32_t m = m0272 + 1; m <= m0274 + 1; m++)
    {
        if (r <= acc) { lead = m - 1; break; }
        if (m > n - 1) { lead = m - 1; break; }
        if (r > acc) acc = (float)(acc + v[m]);
    }
    if (lead == -1) { e->Restarts++; goto L1EB02; }
    // the newbie guard (1646): table INDEX 4, not id 4 - the Slave
    if (all[lead]->GuildLevel > 2 && lead != 4 && meLevel == 1 && areaId < 18) { e->Restarts++; goto L1EB02; }
    if (all[lead]->Id == Asmodeus::MonsterId) e->AsmodeusCues++;                          // 1649: sub09DE mc032E

L1F688:
    m028C = all[lead]->CompanionType;
    m028E = all[lead]->CompanionSpawnMode;
    m0290 = all[lead]->CompanionSubtype;
    m02A2 = lead;
    (void)(Rnd() * 99);                                                              // 1656, never read
    e->Lead = lead;
    e->LeadMonster = all[lead];
    r = RndF() * 99.0f;                                                              // 1658
    e->TreasureClass = 0;
    {
        auto lm = all[lead];
        // 1661-1665: the lair's own monster adds gc1DCA = 25
        int32_t bonus = 0;
        if (area.LairId > 0 && lm->Id == area.LairId && lm->BoxChance.Length() > 2 && lm->BoxChance[2] > 0) bonus += 25;
        for (int32_t k = 1; k <= 2; k++)
            if (r < lm->BoxChance[k] + bonus) e->TreasureClass = k + 1;
        // 1671-1694: the trap. The chest trap is cleared on every pass; the lock is not.
        e->ChestTrap = 0;
        int32_t chance = e->TreasureClass == 1 ? 25 : e->TreasureClass == 2 ? 75 : e->TreasureClass == 3 ? 90 : 0;
        if (e->TreasureClass > 0 && lm->TrapFlags > 0)
        {
            int32_t t = Int(Rnd() * 100) + 1;
            if (t < chance)
            {
                e->ChestTrap = -1;
                do
                {
                    t = Int(Rnd() * (11 + 1));
                    if ((lm->TrapFlags & (1 << t)) != 0) e->ChestTrap = t;
                } while (e->ChestTrap == -1);
            }
        }
        // 1697-1702: not on an anti-magic (18) or an extinguisher (10) square
        if (!Square(Floor::ANTIMAGIC) && !Square(Floor::EXTINGUISHER))
        {
            int32_t t = Int(Rnd() * 100) + 1;
            if (t <= lm->LockedChance && e->TreasureClass > 1) e->ChestLock = 1;
        }
        m0284 = lead;
        // 1704-1712: the number of groups, mc0186 = 4
        groups = lm->NumGroups;
        if (Int(Rnd() * 100) < groups / (4.0 * 2) * 100)
            groups = groups - Int(Rnd() * (lm->NumGroups / 2.0 + 1));
        if (groups < 1) groups = 1;
        else if (groups > 4) groups = 4;
        // 1713-1724: hostility, on Foes(0) alone
        foes[0]->Engaged = 1;
        if (lm->AlignmentValue == 0 && meAlign == 0) foes[0]->Engaged = 0;
        else if (lm->AlignmentValue == 1) foes[0]->Engaged = Int(Rnd() * 100) < 40 ? 0 : 1;
        foes[0]->GroupCount = 1;
    }

    for (int32_t g = 0; g <= groups - 1; g++)
    {
        if (g == 0)
        {
            m0282 = all[lead]->LevelFound;
            goto L2028A;
        }
        switch (m028E)
        {
            case 0: _below = 3; _above = -1; break;
            case 1: _below = 3; _above = 0; break;
            case 2: _above = 0; _below = 0; break;                     // + m0292, always 0
            case 3: m0284 = m02A2; goto L2028A;
            case 4: m0284 = 4; goto L2028A;
            case 5: m0284 = 5; goto L2028A;
            case 6: m0284 = 4; goto L2028A;
            default:
                if (m028E > 7) { m0284 = m028E - 7; goto L2028A; }
                // 7, or anything below 0: no Case, the Static window stands
                break;
        }
        if (m0282 + _above < 1) _above = 0;
        m0272 = -1; m0274 = -1; W = 0;
        // a CompanionType outside the class table is taken as a class with nobody in it (1984-1986)
        classOk = m028C >= 0 && m028C < nc;
        for (int32_t m = classOk ? range[m028C][0] : -1; classOk && m <= range[m028C][1] && m >= 0; m++)
        {
            auto cm = all[m];
            if (cm->LevelFound < m0282 - _below) { }
            else if (cm->LevelFound > m0282 + _above) break;
            else if (cm->InGroup == 1) { }
            else if (m != m02A2)
            {
                if ((m0290 > 0 && cm->Subtype == m0290) || m0290 == 0) W += cm->Chance;
            }
        }
        // 1781-1783: nobody to come along - back to L1F4F8
        if (W == 0) goto L1F4F8;
        m0274 = range[m028C][1];
        for (int32_t m = range[m028C][0]; m <= range[m028C][1] && m >= 0; m++)
        {
            v[m] = 0;
            if (m0272 < 0) m0272 = m;
            auto cm = all[m];
            if (cm->LevelFound < m0282 - _below) { }
            else if (cm->LevelFound > m0282 + _above) { m0274 = m - 1; break; }
            else if (cm->InGroup == 1) { }
            else if (m != m02A2)
            {
                if ((m0290 > 0 && cm->Subtype == m0290) || m0290 == 0) v[m] = cm->Chance / W * 100;
            }
        }
        if (m0274 < 0) m0274 = m0272;
        tries = 0;                                                                 // m02A0, 1804

    L2003C:
        m0284 = -1;
        r = RndF() * 99.0f;
        acc = (float)V(m0274);
        // top down; the first step adds v(m0274) a second time
        for (int32_t c = m0274 + 1; c >= m0272 + 1; c--)
        {
            if (r > acc) acc = (float)(acc + V(c - 1));
            if (r <= acc && c - 1 != lead)
            {
                if (m0290 > 0)
                {
                    if (all[c - 1]->Subtype != m0290)
                    {
                        tries++;
                        if (tries > 150) m0290 = 0;
                        if (tries > 300)
                        {
                            e->Messages.Add("TryNumber > 300!");
                            e->Failed = true;
                            return Finish(e, all, rng);
                        }
                        acc = (float)(acc + V(c));
                        m0284 = -1;
                    }
                    else { m0284 = c - 1; break; }
                }
                else { m0284 = c - 1; break; }
            }
        }
        // 1838-1840
        if (m0290 == 0 && m0284 >= 0) m0290 = all[m0284]->CompanionSubtype;
        if (m0284 > -1)
        {
            m028C = all[m0284]->CompanionType;
            m028E = all[m0284]->CompanionSpawnMode;
            m0290 = all[m0284]->CompanionSubtype;
            m02A2 = m0284;
            m0282 = all[m0284]->LevelFound;
        }

    L2028A:
        if (m0284 == -1)
        {
            // 1850-1853: and round again, the try count not reset
            e->Messages.Add("Error finding like-types for '" + all[lead]->Name + "'.");
            goto L2003C;
        }
        if (m0284 < n) FillGroup(e, g, m0284, all, rng, Square(Floor::FOG), reader, item, meAlign, viewer, range, timer);
    }

    // 1969-1978: the gold scales with every group standing, the stale ones included
    int32_t standing = 0;
    for (auto& s : foes) if (s->GroupCount > 0) standing += s->GroupCount;
    SetGold(e, e->Gold.ToDouble() * (standing / 2.0));
    if (e->ChestLock == 1) SetGold(e, e->Gold.ToDouble() * 1.5);
    return Finish(e, all, rng);
}

void EncounterGenerator::FillGroup(cs::Ref<Encounter> e, int32_t g, int32_t m0284, cs::List<cs::Ref<Monster>> all, cs::Ref<cs::Random> rng, bool fog,
                                   cs::Ref<Character> reader, cs::Func<cs::Ref<Item>(int16_t)> item, int32_t meAlign, cs::Ref<Character> me, cs::List<cs::List<int32_t>> range, double timer) {
    auto Rnd = [&]() { return rng->NextDouble(); };
    auto s = e->Foes[g];
    auto mon = all[m0284];
    // 1854-1858
    s->GroupCount = mon->InGroup;
    s->GroupCount = s->GroupCount - CInt(Rnd() * (mon->InGroup / 2.0));
    if (s->GroupCount < 1) s->GroupCount = 1;
    // 1859-1868: the group's gold, into the Currency Fight.Gold
    if (g == 0) e->Gold = 0;
    int32_t gf = mon->GoldFactor;
    int64_t gold = CLng(gf * cs::Math::Pow(10, gf - .5) / 4);
    // the two draws in C#'s left-to-right order
    double _r1 = Rnd();
    double _r2 = Rnd();
    gold = CLng((gold / 4.0 + _r1 * (gold / 2.0) + _r2 * (gold / 2.0)) * cs::Math::Log((mon->LevelFound + 1) / cs::Math::Log(2)));
    if (gold < 500) gold = Int(Rnd() * 500) + 2;
    double lg = cs::Math::Log(s->GroupCount + 1);
    SetGold(e, e->Gold.ToDouble() + gold * (lg * (lg / 2)));
    // 1869-1879: ONE set of hits, for the member at the front, with no floor under it
    s->Att = mon->Att; s->Def = mon->Def; s->Hits = mon->Hits;
    int32_t sign = Int(Rnd() * 2);
    int32_t swing = Int(Rnd() * (mon->Hits * .5) / 2) + 1;
    s->Hits = sign == 0 ? s->Hits - swing : s->Hits + swing;
    s->MaxHits = s->Hits;
    s->Alignment = mon->AlignmentValue;
    s->Ref = m0284; s->KnownAs = m0284;
    s->Stamp = timer;                                                              // 1883: M4EE6 = Timer
    // 1884-1887: known, unless met in fog (bit 20)
    s->IdLevel = fog ? 1 : 0;
    s->KnownAs = KnownAsIndex(all, m0284, reader, item, rng, range);
    // 1948-1967: the offer to join; Foes(0)'s is wiped on every pass
    e->Foes[0]->JoinOffer = 0;
    if (s->Engaged == 0)
    {
        if (mon->AlignmentValue == 1 || mon->AlignmentValue == meAlign)
        {
            int32_t guildLevel = me != nullptr && me->Guilds.Count() > 0 ? me->Guild()->Level : 0;
            if (mon->GuildLevel <= guildLevel)
            {
                int32_t t = Int(Rnd() * 3000) + 1;
                double bar = (me ? me->Stats[static_cast<int32_t>(Stat::Cha)] : 0) * 10;
                bar = bar - (mon->LevelFound - 1) * 10;
                if (bar < 0) bar = 0;
                if (t <= bar)
                {
                    int32_t most = s->GroupCount;
                    if (most > 4) most = 4;
                    s->JoinOffer = Int(Rnd() * most) + 1;
                }
            }
        }
    }
}

int32_t EncounterGenerator::KnownAsIndex(cs::List<cs::Ref<Monster>> all, int32_t refIndex, cs::Ref<Character> reader, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Ref<cs::Random> rng, cs::List<cs::List<int32_t>> range) {
    auto Rnd = [&]() { return rng->NextDouble(); };
    auto mon = all[refIndex];
    int32_t tried = 0;                                                                 // m02B2
    double skill = IdSkill(reader, item) - cs::Math::Log(mon->GuildLevel) * (mon->LevelFound + 1);
    double bar = 50 + skill * 10;
    if (bar > 95) bar = 95;
    if (bar < 5) bar = 5;
    int32_t roll = Int(Rnd() * 100) + 1;
    auto kin = cs::List<int32_t>::New();
    int32_t me = 0;
    for (int32_t i = 0; i <= all.Count() - 1; i++)
    {
        if (all[i]->Subtype == mon->Subtype) kin.Add(i);
        if (i == refIndex) me = kin.Count() - 1;
    }
    if (!(roll > bar && kin.Count() > 1)) return refIndex;
    int32_t spread = CInt(Rnd() * (cs::Math::Log(mon->LevelFound) + 2.5));
    int32_t cls = mon->Type;
    int32_t first = cls >= 0 && cls < range.Count() ? range[cls][0] : -1;
    int32_t last = cls >= 0 && cls < range.Count() ? range[cls][1] : -1;
    int32_t pick = 0;
    do
    {
        bool up = Int(Rnd() * 100) + 1 < bar;
        pick = Int(Rnd() * (spread + 1.5));
        if (up) pick = me + pick <= kin.Count() - 1 ? kin[me + pick] : -1;
        else pick = me - pick >= 0 ? kin[me - pick] : -1;
        if (pick < first) pick = -1;
        if (pick > last) pick = -1;
        tried++;
        if (tried > 100) pick = refIndex;
    } while (pick == -1);
    return pick;
}

double EncounterGenerator::IdSkill(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (c == nullptr) return 0;
    return item
        ? Rules::IdentifySkill(c->TopGuildLevel(), c->EffectiveStat(static_cast<int32_t>(Stat::Int), item), c->EffectiveStat(static_cast<int32_t>(Stat::Wis), item))
        : Rules::IdentifySkill(c->TopGuildLevel(), c->Stats[static_cast<int32_t>(Stat::Int)], c->Stats[static_cast<int32_t>(Stat::Wis)]);
}

cs::Ref<Encounter> EncounterGenerator::Finish(cs::Ref<Encounter> e, cs::List<cs::Ref<Monster>> all, cs::Ref<cs::Random> rng) {
    (void)rng;
    bool hostile = e->Foes[0]->Engaged != 0;
    for (int32_t g = 0; g < e->Foes.Length(); g++)
    {
        auto s = e->Foes[g];
        if (s->GroupCount <= 0 || s->Ref < 0 || s->Ref >= all.Count()) continue;
        auto def = all[s->Ref];
        auto seen = s->KnownAs >= 0 && s->KnownAs < all.Count() ? all[s->KnownAs] : def;
        for (int32_t i = 0; i < s->GroupCount; i++)
        {
            bool front = i == 0;
            e->Add([&] {
                auto _o = cs::New<CombatMonster>();
                _o->Def = def;
                _o->set_Hits(front ? s->Hits : cs::Math::Max(1, (int32_t)def->Hits));
                _o->MaxHits = front ? s->MaxHits : def->Hits;
                _o->Unrolled = !front;
                // a record at nought with its count above it stands (CombatMonster.AtNought)
                _o->AtNought = front && s->Hits <= 0;
                _o->Alignment = s->Alignment;
                _o->Group = g;
                _o->Identified = s->IdLevel == 0;
                _o->Believed = seen;
                _o->Hostile = hostile;
                _o->JoinOffer = g == 0 && i == 0 ? e->Foes[0]->JoinOffer : 0;
                return _o;
            }());
        }
    }
    return e;
}

cs::Ref<CombatEngine> EncounterGenerator::ClearedByJoining(cs::Ref<Party> party, cs::List<cs::Ref<CombatMonster>> emptied, cs::Ref<DungeonState> state, int32_t level, int32_t areaId,
                                                           int32_t headerAreas, cs::DateTime now, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Ref<cs::Random> rng,
                                                           cs::List<cs::Ref<Monster>> all) {
    auto fight = cs::New<CombatEngine>(party, emptied, item, rng);
    if (areaId > 0)
    {
        state->StoreRoom(level, areaId, Capture(state->LoadRoom(level, areaId), emptied, nullptr, all));
        state->MarkCleared(level, areaId, now, headerAreas);
    }
    fight->AwardSpoils();
    if (areaId > 0) state->StoreRoom(level, areaId, Capture(state->LoadRoom(level, areaId), fight->Monsters, fight, all));
    return fight;
}

cs::Ref<Monster> EncounterGenerator::BelievedToBe(cs::List<cs::Ref<Monster>> all, cs::Ref<Monster> def, cs::Ref<Character> viewer, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item) {
    int32_t index = -1;
    for (int32_t i = 0; i < all.Count(); i++) if (all[i] == def) { index = i; break; }
    if (index < 0 || viewer == nullptr) return def;
    return all[KnownAsIndex(all, index, viewer, item, rng, ClassRanges(all))];
}

cs::String EncounterGenerator::AcceptJoin(cs::Ref<Character> who, cs::List<cs::Ref<CombatMonster>> group, cs::Func<cs::Ref<Item>(int16_t)> item,
                                          cs::Ref<cs::Random> rng, int32_t target) {
    if (group.Count() == 0 || group[0]->JoinOffer <= 0) return "";
    int32_t offered = group[0]->JoinOffer;
    if (who->Companions->FreeSlots() == 0) return "Join - No room for more Companions!";

    group[0]->JoinOffer = 0;
    auto _alive = group.FirstOrDefault([&](auto& m) { return m->Alive(); });
    auto front = _alive ? _alive : group[0];
    int32_t hits = front->Hits(), max = front->MaxHits > 0 ? front->MaxHits : front->Hits();
    auto def = front->Def;
    // CombatOrders(Me).M547A: M547A = fn0DCD(-1) (MONSMOD.BAS:737), unless chosen since
    auto record = target >= 0 && target < group.Count() ? group[target] : group.FirstOrDefault([&](auto& m) { return m->Alive(); });
    for (int32_t i = 0; i < offered; i++)
    {
        // Foes(0).GroupCount > 0: only the first group
        if (who->Companions->FreeSlots() == 0 || group.Count() == 0 || !CombatMonster::SameGroup(group[0], front)) break;
        who->Companions->Add([&] {
            auto _o = cs::New<Companion>();
            _o->Ref = def->Ref;                  // p00D6.Ref, the front one (MAINMOD2.BAS:388)
            _o->MonsterId = def->Id;
            _o->Name = "";
            _o->Hits = hits;
            _o->MaxHits = max;
            _o->Alignment = front->CompanionAlignment();   // Foes(0).Alignment (MAINMOD2.BAS:392)
            _o->BindingLevel = FreeWill;
            _o->IdLevel = CombatEngine::CharmIdLevel(who, def, item);
            return _o;
        }(), true);   // fn1163 leaves the slot's name as it was (MAINMOD2.BAS:388-400)
        // GroupCount - 1 (4105)
        group.RemoveAt(0);
    }
    // "If Foes(0).GroupCount > 0 Then sub0D08 Me" (MAINMOD2.BAS:4110-4111)
    auto next = group.FirstOrDefault([&](auto& m) { return m->Alive() && CombatMonster::SameGroup(m, front); });
    if (next != nullptr && rng != nullptr)
    {
        auto aimed = record == nullptr ? nullptr : group.FirstOrDefault([&](auto& m) { return m->Alive() && CombatMonster::SameGroup(m, record); });
        if (aimed != nullptr && (aimed->Unrolled || aimed->Group >= 0)) aimed->LoadRecord(rng);
        if (next->Unrolled) next->TakeRecord(front->Hits(), front->MaxHits);
    }
    return "";
}

cs::String EncounterGenerator::NameFor(cs::Ref<Monster> def, cs::Ref<Character> viewer, int32_t count, bool identified) {
    return NameFor(def, viewer, count, identified ? 0 : 1);
}

cs::String EncounterGenerator::NameFor(cs::Ref<Monster> def, cs::Ref<Character> viewer, int32_t count, int32_t idLevel) {
    bool invisible = (def->SpecialPropertyFlags & (1 << Effects::Invisible)) != 0;
    if (invisible && viewer != nullptr && !viewer->SeeingInvisible && viewer->DungeonLevel > 0)
        return count > 1 ? "Unseen Monsters" : "Unseen Monster";

    cs::String name;
    if (!Mdata1::Loaded) name = def->Name;
    else if (idLevel == 1) name = Mdata1::MonsterClassName(def->Type, def->Name);
    else if (idLevel == 2) name = Mdata1::MonsterGroupName(def->Type, def->Name);
    else name = def->Name;
    return Pluralise(name, count);
}

cs::String EncounterGenerator::Pluralise(cs::String name, int32_t count) {
    auto Tail = [&](int32_t n) -> cs::String { return (int32_t)name.size() >= n ? cs::Str::Substring(name, (int32_t)name.size() - n) : name; };
    auto Drop = [&](int32_t n) -> cs::String { return cs::Str::Substring(name, 0, (int32_t)name.size() - n); };

    {
        cs::String t3 = Tail(3);
        if (t3 == "sae" || t3 == "lex" || t3 == "ish" || t3 == "asu" || t3 == "shi" ||
            t3 == "toa" || t3 == "ngi" || t3 == "ubi" || t3 == "kul") return name;
    }
    if (Tail(2) == "yr" || Tail(2) == "ee") return name;
    if (count <= 1) return name;

    if (Tail(3) == "man") return Drop(3) + "men";
    { cs::String t = Tail(3); if (t == "bus" || t == "gus" || t == "eet") return Drop(2) + "i"; }
    { cs::String t = Tail(2); if (t == "ry" || t == "py") return Drop(1) + "ies"; }
    if (Tail(2) == "ch") return Tail(5) == "Leech" ? name + "es" : name + "s";
    if (Tail(3) == "sus") return Drop(1) + "mi";
    { cs::String t = Tail(3); if (t == "ief" || t == "arf" || t == "Elf" || t == "olf") return Drop(1) + "ves"; }
    if (Tail(3) == "mmy") return Drop(1) + "ies";
    return Tail(1) == "s" ? name : name + "s";
}

cs::String EncounterGenerator::Describe(cs::List<cs::Ref<CombatMonster>> group, cs::Ref<Character> viewer) {
    auto list = group.ToList();
    auto parts = list.GroupBy([&](auto& m) { return m->Seen()->Name; }).Select([&](auto& g) -> cs::String {
        return g.Count() == 1
            ? "a " + NameFor(g.Items.First()->Seen(), viewer, 1, g.Items.First()->Identified ? 0 : 1)
            : cs::ToStr(g.Count()) + " " + NameFor(g.Items.First()->Seen(), viewer, g.Count(), g.Items.First()->Identified ? 0 : 1);
    }).ToList();
    cs::String s = parts.Count() <= 1 ? (parts.Count() == 0 ? cs::String("something") : parts.First())
        : cs::Str::Join(", ", parts.Take(parts.Count() - 1)) + " and " + parts[parts.Count() - 1];
    return cs::Str::ToUpper(cs::String(1, s[0])) + cs::Str::Substring(s, 1) + (list.Count() == 1 ? " attacks!" : " attack!");
}

cs::String EncounterGenerator::Plural(cs::String name) {
    if (cs::Str::EndsWith(name, "s") || cs::Str::EndsWith(name, "x") || cs::Str::EndsWith(name, "ch") || cs::Str::EndsWith(name, "sh")) return name + "es";
    if (cs::Str::EndsWith(name, "f")) return cs::Str::Substring(name, 0, (int32_t)name.size() - 1) + "ves";
    if (cs::Str::EndsWith(name, "y") && (int32_t)name.size() > 1 && !cs::Str::Contains(cs::String("aeiou"), name[name.size() - 2])) return cs::Str::Substring(name, 0, (int32_t)name.size() - 1) + "ies";
    return name + "s";
}

}  // namespace mc
