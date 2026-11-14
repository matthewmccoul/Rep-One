#include "core.h"

namespace mc {

cs::List<cs::Ref<Spell>> MdrLoader::LoadSpells(cs::String path) {
    auto r = cs::New<RecordReader>(path, 75);
    auto spells = cs::List<cs::Ref<Spell>>::New();
    r->Read(); r->GetString();            // version
    r->Read(); int32_t n = r->GetUWord();  // count
    for (int32_t i = 0; i < n; i++) {
        r->Read();
        auto s = [&] {
            auto _o = cs::New<Spell>();
            _o->Name = r->GetString();
            _o->Id = r->GetWord();
            _o->Category = r->GetWord();
            _o->Level = r->GetWord();
            _o->U4 = r->GetWord();
            _o->KillEffect = r->GetWord();
            _o->AffectMonster = r->GetWord();
            _o->AffectGroup = r->GetWord();
            _o->Damage1 = r->GetWord();
            _o->Damage2 = r->GetWord();
            _o->SpecialEffect = r->GetWord();
            return _o;
        }();
        // Ailments (the effect id) comes before the stat requirements (MAIN.TXT:403-417)
        s->Effect = r->GetWord();
        s->Required = r->GetWords(7);
        s->ResistIndex = r->GetWord();
        spells.Add(s);
    }
    return spells;
}

cs::List<cs::Ref<Item>> MdrLoader::LoadItems(cs::String path) {
    auto r = cs::New<RecordReader>(path, 125);
    auto items = cs::List<cs::Ref<Item>>::New();
    r->Read(); r->GetString();            // version
    r->Read(); r->GetUWord();             // store code
    r->Read(); int32_t n = r->GetUWord();  // count
    for (int32_t i = 0; i < n; i++) {
        r->Read();
        auto it = [&] {
            auto _o = cs::New<Item>();
            _o->Name = r->GetString();
            _o->Id = r->GetWord();
            _o->Att = r->GetWord();
            _o->Def = r->GetWord();
            _o->Price = r->GetDword();
            _o->Floor = r->GetWord();
            _o->Rarity = r->GetWord();
            _o->Abilities = r->GetDword();
            _o->Swings = r->GetWord();
            _o->SpecialType = r->GetWord();
            _o->SpellIndex = r->GetWord();
            _o->SpellId = r->GetWord();
            _o->Charges = r->GetDword();
            _o->Guilds = r->GetDword();
            _o->LevelScale = r->GetWord();
            _o->DamageMod = r->GetFloat();
            _o->AlignmentFlags = r->GetDword();
            _o->NHands = r->GetWord();
            _o->Type = r->GetWord();
            _o->ResistanceFlags = r->GetDword();
            return _o;
        }();
        it->StatsRequired = r->GetWords(7);
        it->StatsMod = r->GetWords(7);
        it->Cursed = r->GetWord();
        it->SpellLvl = r->GetWord();
        it->ClassRestricted = r->GetWord();
        items.Add(it);
    }
    return items;
}

cs::List<cs::Ref<Monster>> MdrLoader::LoadMonsters(cs::String path) {
    auto r = cs::New<RecordReader>(path, 160);
    auto monsters = cs::List<cs::Ref<Monster>>::New();
    r->Read(); r->GetString();            // version
    r->Read();                            // (unused record)
    r->Read(); int32_t n = r->GetUWord();  // count
    for (int32_t i = 0; i < n; i++) {
        r->Read();
        auto m = [&] {
            auto _o = cs::New<Monster>();
            _o->Name = r->GetString();
            _o->Att = r->GetWord();
            _o->Def = r->GetWord();
            _o->Id = r->GetWord();
            _o->Hits = r->GetWord();
            _o->NumGroups = r->GetWord();
            _o->PicId = r->GetWord();
            _o->LockedChance = r->GetWord();
            _o->LevelFound = r->GetWord();
            return _o;
        }();
        m->Resistances = r->GetWords(12);
        m->SpecialPropertyFlags = r->GetDword();
        m->SpecialAttackFlags = r->GetDword();
        m->SpellFlags = r->GetDword();
        m->Chance = r->GetWord();
        m->BoxChance = r->GetWords(4);
        m->AlignmentValue = r->GetWord();
        m->InGroup = r->GetWord();
        m->GoldFactor = r->GetDword();
        m->TrapFlags = r->GetDword();
        m->GuildLevel = r->GetWord();
        m->Stats = r->GetWords(7);
        m->Type = r->GetWord();
        m->DamageMod = r->GetFloat();
        m->CompanionType = r->GetWord();
        m->CompanionSpawnMode = r->GetWord();
        m->CompanionId = r->GetWord();
        m->Items = r->GetWords(11);
        m->Subtype = r->GetWord();
        m->CompanionSubtype = r->GetWord();
        m->Size = r->GetWord();
        m->ItemDropLevel = r->GetWord();
        m->ItemChance = r->GetWord();
        m->Ref = i;  // Monsters(i): table index i is record i + 4 of the file
        monsters.Add(m);
    }
    return monsters;
}

cs::List<cs::Ref<Floor>> MdrLoader::LoadDungeon(cs::String path) {
    auto r = cs::New<RecordReader>(path, 20);
    r->Read(); int32_t numLevels = r->GetUWord();
    auto offsets = cs::List<int32_t>::New(numLevels);
    for (int32_t i = 0; i < numLevels; i++) { r->Read(); offsets[i] = r->GetUWord(); }

    auto dungeon = cs::List<cs::Ref<Floor>>::New();
    for (int32_t i = 0; i < numLevels; i++) {
        r->Seek(offsets[i]);
        dungeon.Add(LoadFloor(r));
    }
    return dungeon;
}

cs::Ref<Floor> MdrLoader::LoadFloor(cs::Ref<RecordReader> r) {
    auto f = cs::New<Floor>();
    r->Read();
    f->Width = r->GetWord();
    f->Height = r->GetWord();
    f->LevelNum = r->GetWord();
    int32_t numAreas = r->GetUWord();
    int32_t numChutes = r->GetUWord();
    int32_t numTeleporters = r->GetUWord();
    f->HeaderAreaCount = numAreas;

    int32_t cells = (int32_t)f->Width * (int32_t)f->Height;
    f->Fields = cs::List<Floor_Field>::New(cells);
    for (int32_t i = 0; i < cells; i++) {
        r->Read();
        f->Fields[i] = [&] { Floor_Field _o{}; _o.AreaId = r->GetWord(); _o.Flags = r->GetIntCurrency(); return _o; }();
    }

    r->Read(); r->GetWord();  // area count (== numAreas)
    for (int32_t i = 0; i < 201; i++) {
        r->Read();
        f->Areas.Add([&] { Floor_Area _o{}; _o.SpawnMask = r->GetDword(); _o.LairId = r->GetWord(); return _o; }());
    }

    r->Read(); r->GetWord();  // teleporter count
    for (int32_t i = 0; i < numTeleporters; i++) {
        r->Read();
        auto t = [&] {
            Floor_Teleporter _o{};
            _o.X = r->GetWord(); _o.Y = r->GetWord(); _o.X2 = r->GetWord(); _o.Y2 = r->GetWord(); _o.Z2 = r->GetWord();
            return _o;
        }();
        if (t.X > 0 && t.Y > 0) f->Teleporters.Add(t);
    }

    r->Read(); r->GetWord();  // chute count
    for (int32_t i = 0; i < numChutes; i++) {
        r->Read();
        auto c = [&] { Floor_Chute _o{}; _o.X = r->GetWord(); _o.Y = r->GetWord(); _o.Depth = r->GetWord(); return _o; }();
        if (c.X > 0 && c.Y > 0) f->Chutes.Add(c);
    }
    return f;
}

cs::Ref<Library> MdrLoader::LoadLibrary(cs::String path) {
    auto r = cs::New<RecordReader>(path, 100);
    auto lib = cs::New<Library>();
    r->Read(); r->GetString();  // version
    r->Read(); int32_t numMonsters = r->GetWord();
    r->Read(); int32_t numItems = r->GetWord();
    r->Read(); lib->MonstersFound = r->GetWord();
    r->Read(); lib->ItemsFound = r->GetWord();
    for (int32_t i = 0; i < numMonsters; i++) { r->Read(); lib->Monsters.Add(ReadLibRecord(r)); }
    for (int32_t i = 0; i < numItems; i++) { r->Read(); lib->Items.Add(ReadLibRecord(r)); }
    return lib;
}

cs::Ref<LibraryRecord> MdrLoader::ReadLibRecord(cs::Ref<RecordReader> r) {
    auto _o = cs::New<LibraryRecord>();
    _o->Id = r->GetWord();
    _o->Known = r->GetWord();
    _o->NSeen = r->GetCurrency();
    _o->Name = cs::Str::TrimEnd(r->GetString(30), cs::String("\0 ", 2));
    _o->MonId = r->GetWord();
    _o->LastSeen = r->GetDword();
    _o->Location = cs::Str::TrimEnd(r->GetString(8), cs::String("\0 ", 2));
    _o->NDeaths = r->GetWord();
    return _o;
}

// Every record in the two ranges is kept, as fn03D0 keeps them (MODULE1.BAS:693-700).

cs::List<cs::Ref<Race>> MdrLoader::LoadRaces(cs::String path) {
    auto races = cs::List<cs::Ref<Race>>::New();
    auto r = cs::New<RecordReader>(path, Mdata1::RecordLength);
    auto at = Mdata1::Read(r)->Races;  // exact, from fn03D0's own read order
    for (int32_t i = at.First; i < at.First + at.Count; i++) {
        r->Seek(i); r->Read();
        cs::String name = r->GetString();
        auto min = r->GetWords(7); auto max = r->GetWords(7); auto res = r->GetWords(12);
        int32_t align = r->GetDword(); int16_t size = r->GetWord(), bonus = r->GetWord(), maxAge = r->GetWord(); float exp = r->GetFloat();
        races.Add([&] {
            auto _o = cs::New<Race>();
            _o->Name = name;
            _o->Min = min.Take(6).Select([&](auto& v) { return (int32_t)v; }).ToArray();
            _o->Max = max.Take(6).Select([&](auto& v) { return (int32_t)v; }).ToArray();
            _o->Size = static_cast<RaceSize>(size);  // as the record has it
            _o->AlignMask = align; _o->BonusPoints = bonus; _o->MaxAge = maxAge; _o->ExpFactor = exp;
            _o->Resistances = res.Take(Effects::ResistanceKinds).Select([&](auto& v) { return (int32_t)v; }).ToArray();
            return _o;
        }());
    }
    return races;
}

cs::List<cs::Ref<GuildDef>> MdrLoader::LoadGuilds(cs::String path) {
    auto guilds = cs::List<cs::Ref<GuildDef>>::New();
    auto r = cs::New<RecordReader>(path, Mdata1::RecordLength);
    auto at = Mdata1::Read(r)->Guilds;  // exact, from fn03D0's own read order
    for (int32_t i = at.First; i < at.First + at.Count; i++) {
        r->Seek(i); r->Read();
        cs::String name = r->GetString();
        int16_t avgHits = r->GetWord(), maxLevel = r->GetWord(), lateHits = r->GetWord(); float expF = r->GetFloat(); r->GetWord();
        auto req = r->GetWords(7); int32_t align = r->GetDword();
        auto abilities = cs::List<float>::New(7);
        for (int32_t k = 0; k < 7; k++) abilities[k] = r->GetFloat();
        r->GetWord(); r->GetFloat(); int16_t quest = r->GetWord();
        auto rate = cs::List<float>::New(19); auto cap = cs::List<int32_t>::New(19);
        for (int32_t k = 0; k < 19; k++) rate[k] = r->GetFloat();
        for (int32_t k = 0; k < 19; k++) cap[k] = cs::CastDouble<int32_t>(r->GetFloat());
        int32_t raceMask = r->GetDword();
        // T5584: Gold, an unknown Single, then Att and Def per level
        int16_t goldFactor = r->GetWord(); float u16 = r->GetFloat();
        float attPerLevel = r->GetFloat(), defPerLevel = r->GetFloat();
        int16_t adMaxLevel = r->GetWord(); float adEvery = r->GetFloat();
        // AH, MH and EP go in as the record has them (MODULE1.BAS:697-700)
        guilds.Add([&] {
            auto _o = cs::New<GuildDef>();
            _o->Name = name; _o->MaxLevel = 999; _o->HitsCapLevel = maxLevel; _o->LateHits = lateHits;
            _o->ReqStats = req.Take(6).Select([&](auto& v) { return (int32_t)v; }).ToArray(); _o->AlignMask = align;
            _o->RaceMask = raceMask; _o->AvgHits = avgHits; _o->ExpFactor = expF; _o->QuestPercent = quest;
            _o->SpellRate = rate; _o->SpellMax = cap; _o->Abilities = abilities;
            _o->GoldFactor = goldFactor; _o->AttPerLevel = attPerLevel; _o->DefPerLevel = defPerLevel; _o->ItemUseFactor = u16;
            _o->AdMaxLevel = adMaxLevel; _o->AdEvery = adEvery;
            return _o;
        }());
    }
    return guilds;
}

cs::List<cs::Ref<HallRecord>> MdrLoader::LoadHallRecords(cs::String path) {
    auto r = cs::New<RecordReader>(path, 45);
    auto records = cs::List<cs::Ref<HallRecord>>::New();
    for (int32_t i = 0; i < 13; i++) {
        r->Read();
        records.Add([&] {
            auto _o = cs::New<HallRecord>();
            _o->Value = r->GetCurrency();
            _o->Name = cs::Str::TrimEnd(r->GetString(30), cs::String("\0 ", 2));
            _o->Date = r->GetDword();
            return _o;
        }());
    }
    return records;
}

}  // namespace mc
