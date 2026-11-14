#include "core.h"

namespace mc {

cs::String ImportNote::ToString() { return ToStr(Kind) + ": " + Where + ": " + Text; }

static cs::Ref<ImportFile> MakeImportFile(cs::String name, cs::String holds, cs::String status, int32_t records) {
    auto _o = cs::New<ImportFile>();
    _o->Name = name; _o->Holds = holds; _o->Status = status; _o->Records = records;
    return _o;
}

OriginalSaves::OriginalSaves(cs::String dataDir, cs::List<cs::Ref<Item>> items, cs::List<cs::Ref<Monster>> monsters) {
    DataDir = dataDir;
    _items = items;
    _monsters = monsters;
    _itemById = items.GroupBy([&](auto& i) { return i->Id; })
                    .ToDictionary([&](auto& g) { return g.Key; }, [&](auto& g) { return g.Items.First(); });
}

void OriginalSaves::Note(NoteKind k, cs::String where, cs::String text) {
    auto _o = cs::New<ImportNote>();
    _o->Kind = k; _o->Where = where; _o->Text = text;
    Notes.Add(_o);
}

cs::String OriginalSaves::FindDataDir(cs::String picked) {
    if (cs::File::Exists(cs::Path::Combine(picked, "MDATA4.MDR"))) return picked;
    cs::String sub = cs::Path::Combine(picked, "DATA");
    if (cs::File::Exists(cs::Path::Combine(sub, "MDATA4.MDR"))) return sub;
    return "";
}

cs::Ref<OriginalSaves> OriginalSaves::Read(cs::String dataDir, cs::List<cs::Ref<Item>> items, cs::List<cs::Ref<Monster>> monsters) {
    auto s = cs::New<OriginalSaves>(dataDir, items, monsters);
    s->Guard("MDATA4.MDR", "characters", [&](cs::String p, cs::String f) { return s->ReadCharacters(p, f); });
    s->Guard("MDATA6.MDR", "the General Store's shelves", [&](cs::String p, cs::String f) { return s->ReadStore(p, f); });
    s->Guard("MDATA7.MDR", "the guildmasters", [&](cs::String p, cs::String f) { return s->ReadGuildmasters(p, f); });
    s->Guard("MDATA8.MDR", "the automap and the deepest level reached", [&](cs::String p, cs::String f) { return s->ReadAutomap(p, f); });
    s->Guard("MDATA9.MDR", "the guild log", [&](cs::String p, cs::String f) { return s->ReadGuildLog(p, f); });
    s->Guard("MDATA10.MDR", "the dungeon's rooms and respawn tallies", [&](cs::String p, cs::String f) { return s->ReadRooms(p, f); });
    s->Guard("MDATA12.MDR", "the remembered parties", [&](cs::String p, cs::String f) { return s->ReadParties(p, f); });
    s->Guard("MDATA13.MDR", "the Library", [&](cs::String p, cs::String f) { return s->ReadLibrary(p, f); });
    s->Guard("MDATA14.MDR", "the Hall of Records", [&](cs::String p, cs::String f) { return s->ReadHall(p, f); });
    s->Guard("MDATA15.MDR", "the Confinement's pens", [&](cs::String p, cs::String f) { return s->ReadPens(p, f); });
    s->CompareTables();
    return s;
}

void OriginalSaves::Guard(cs::String file, cs::String holds, cs::Func<int32_t(cs::String, cs::String)> read) {
    cs::String path = cs::Path::Combine(DataDir, file);
    if (!cs::File::Exists(path)) { Files.Add(MakeImportFile(file, holds, "absent", 0)); return; }
    try {
        int32_t records = read(path, file);
        if (!Files.Any([&](auto& f) { return f->Name == file; })) Files.Add(MakeImportFile(file, holds, "read", records));
    } catch (const std::exception& ex) {
        Files.Add(MakeImportFile(file, holds, cs::String("refused: ") + ex.what(), 0));
        Note(NoteKind::Refused, file, cs::String("could not be read: ") + ex.what());
    }
}

void OriginalSaves::Refuse(cs::String file, cs::String holds, cs::String why) {
    Files.Add(MakeImportFile(file, holds, "refused: " + why, 0));
    Note(NoteKind::Refused, file, why);
}

bool OriginalSaves::StampOk(cs::Ref<VbFile> f, cs::String file, cs::String holds) {
    cs::String stamp = f->VarString(1);
    if (VbLayout::VersionAccepted(stamp)) return true;
    Refuse(file, holds, "stamped with version \"" + stamp + "\", which Mordor " + VbLayout::Version + " refuses (fn0326)");
    return false;
}

void OriginalSaves::CompareTables() {
    auto Compare = [&](cs::String file, int32_t len, int32_t count, cs::String what) {
        auto f = VbFile::Open(cs::Path::Combine(DataDir, file), len);
        if (f == nullptr) return;
        int32_t theirs = f->Integer(3);
        if (theirs != count)
            Note(NoteKind::Warning, file, "this DATA folder's " + what + " table has " + cs::ToStr(theirs) + " entries and the port's game folder's " + cs::ToStr(count) + ": "
                 + "the saves index the table by position, so they were made with other game data");
    };
    Compare("MDATA3.MDR", 125, _items.Count(), "item");
    Compare("MDATA5.MDR", 160, _monsters.Count(), "monster");
}

int32_t OriginalSaves::ReadCharacters(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::CharactersLen);
    if (!StampOk(f, file, "characters")) return f->RecordCount();
    CharacterCount = f->Integer(2);
    // mdrmazGetChars (MAZEMOD.BAS:203-224): blank (deleted) records skipped without counting
    int32_t rec = 3, taken = 0;
    while (rec <= f->RecordCount()) {
        auto raw = f->Record(rec);
        VbRecord r(raw, VbLayout::Character);
        cs::String name = VbFile::RTrim(r.FixedString("Name"));
        if (name != "") {
            if (taken >= CharacterCount)
                Note(NoteKind::Warning, file + " record " + cs::ToStr(rec), "holds \"" + name + "\" past the count of " + cs::ToStr(CharacterCount) + ": the original lists no record past its count, so it is not imported");
            else {
                taken++;
                Characters.Add(Convert(rec, name, raw));
            }
        }
        rec++;
    }
    if (taken < CharacterCount)
        Note(NoteKind::Warning, file, "the count says " + cs::ToStr(CharacterCount) + " characters and the file holds " + cs::ToStr(taken));
    // grouped ignoring case (StringComparer.OrdinalIgnoreCase); the key is the first member's
    auto dup = Characters.GroupBy([&](auto& c) { return cs::Str::ToLower(Character::SafeFileName(c->Name)); })
                   .Where([&](auto& g) { return g.Count() > 1; });
    for (auto& g : dup)
        Note(NoteKind::Warning, file, cs::ToStr(g.Count()) + " characters would share the file name \"" + Character::SafeFileName(g.Items.First()->Name) + ".json\"; only the first is imported");
    return f->RecordCount();
}

cs::Ref<ImportedCharacter> OriginalSaves::Convert(int32_t recordNo, cs::String name, cs::List<uint8_t> raw) {
    cs::String where = "\"" + name + "\"";
    VbRecord r(raw, VbLayout::Character);
    auto Refused = [&](cs::String why) {
        Note(NoteKind::Refused, where, why);
        auto _o = cs::New<ImportedCharacter>();
        _o->Record = recordNo; _o->Name = name; _o->Raw = raw; _o->Refusal = why;
        return _o;
    };

    int32_t race = r.Integer("RaceIndex");
    if (race < 0 || race >= RaceData::All.Count()) return Refused("race " + cs::ToStr(race) + " is not in MDATA1");
    int32_t current = r.Integer("CurrentGuild");
    if (current < 0 || current >= GuildData::All.Count()) return Refused("current guild " + cs::ToStr(current) + " is not in MDATA1");

    auto c = [&] {
        auto _o = cs::New<Character>();
        _o->Name = name;
        _o->Race = RaceData::All[race]->Name;
        _o->StatMax = RaceData::All[race]->Max.Clone();
        return _o;
    }();

    int32_t align = r.Integer("Alignment");
    if (align < 0 || align > 2) { Note(NoteKind::Warning, where, "alignment " + cs::ToStr(align) + " is not Good, Neutral or Evil (gv074C); taken as Neutral"); align = 1; }
    c->Alignment = static_cast<Alignment>(align);
    int32_t sex = r.Integer("M5829");
    if (sex < 0 || sex > 1) { Note(NoteKind::Warning, where, "sex " + cs::ToStr(sex) + " is not Male or Female (gv06B6); taken as Male"); sex = 0; }
    c->Sex = static_cast<Sex>(sex);

    for (int32_t i = 0; i < 6; i++) c->Stats[i] = r.Integer("Stats(" + cs::ToStr(i) + ")");
    if (r.Integer("Stats(6)") != 0) Note(NoteKind::Warning, where, "Stats(6) holds " + cs::ToStr(r.Integer("Stats(6)")) + "; nothing reads a seventh stat and the port has none");

    float age = r.Single("Age");
    c->Age = cs::CastDouble<int32_t>(cs::Math::Round(age, cs::MidpointRounding::ToEven));
    if ((float)c->Age != age) Note(NoteKind::Info, where, "age " + cs::ToStr(age) + " days is kept as " + cs::ToStr(c->Age) + " (the port counts whole days)");

    c->MaxHits = r.Integer("MaxHits");
    c->CurHits = r.Integer("Hits");
    c->CurSpellPoints = r.Integer("SpellPoints");

    // X and Y are 1-based in the record and 0-based in the port (DungeonWindow shows X + 1)
    c->DungeonLevel = r.Integer("DungeonLevel");
    int32_t x = r.Integer("X"), y = r.Integer("Y");
    if (x < 1 || y < 1) Note(NoteKind::Warning, where, "position " + cs::ToStr(x) + "," + cs::ToStr(y) + " is off the map's 1-based squares; kept as 0,0");
    c->X = cs::Math::Max(0, x - 1);
    c->Y = cs::Math::Max(0, y - 1);
    int32_t facing = r.Integer("Facing");
    if (facing < 1 || facing > 4) { Note(NoteKind::Warning, where, "facing " + cs::ToStr(facing) + " is not 1 to 4 (gv06E8 + 1); taken as North"); facing = 1; }
    c->Facing = facing;
    c->Area = (int32_t)r.Integer("M59BB");
    c->SanctuaryX = r.Integer("Sanctuary.M4E78");
    c->SanctuaryY = r.Integer("Sanctuary.M4E7D");
    c->SanctuaryLevel = r.Integer("Sanctuary.Level");
    c->LostLevel = r.Integer("M59F4(0)") != 0;
    c->LostPosition = r.Integer("M59F4(1)") != 0;
    c->LostFacing = r.Integer("M59F4(2)") != 0;

    c->GoldOnHand = r.Currency("Gold");
    c->GoldInBank = r.Currency("BankGold");

    // ---- the guilds: Level > 0 is joined (FRMCTYG.BAS:98-103)
    auto joined = cs::List<cs::Ref<GuildMembership>>::New();
    for (int32_t g = 0; g < 16; g++) {
        auto gr = r.Sub("GuildsJoined(" + cs::ToStr(g) + ")");
        int32_t level = gr.Integer("Level");
        if (level <= 0) {
            if (level < 0 || (!gr.IsZero() && g != current))
                Note(NoteKind::Info, where, "guild slot " + cs::ToStr(g) + " holds a level of " + cs::ToStr(level) + " and other values; not joined, so not imported");
            continue;
        }
        if (g >= GuildData::All.Count()) { Note(NoteKind::Warning, where, "guild slot " + cs::ToStr(g) + " has level " + cs::ToStr(level) + " but MDATA1 has only " + cs::ToStr(GuildData::All.Count()) + " guilds; dropped"); continue; }
        // the current guild's experience is kept on the character itself, M579B (FRMCTYG.BAS:148-150)
        VbCurrency exp = g == current ? r.Currency("M579B") : gr.Currency("M579B");
        auto m = [&] {
            auto _o = cs::New<GuildMembership>();
            _o->Guild = GuildData::All[g]->Name;
            _o->Level = level;
            _o->Experience = exp.RoundToWhole();
            _o->QuestType = gr.Integer("M55CB");
            _o->QuestTarget = gr.Integer("M57A9");
            _o->QuestDone = gr.Integer("M57B2") != 0;
            return _o;
        }();
        if (exp.Raw % VbCurrency::Scale != 0) Note(NoteKind::Info, where, m->Guild + " experience " + exp.ToString() + " is kept as " + cs::ToStr(m->Experience));
        auto pad = Rules::GuildADSingle(m->Guild, level);
        float pa = pad.att, pd = pad.def;
        float sa = gr.Single("Att"), sd = gr.Single("Def");
        if (cs::CastDouble<int32_t>(sa) != cs::CastDouble<int32_t>(pa) || cs::CastDouble<int32_t>(sd) != cs::CastDouble<int32_t>(pd))
            Note(NoteKind::Warning, where, m->Guild + " level " + cs::ToStr(level) + ": the record's guild A/D is " + cs::ToStr(cs::CastDouble<int32_t>(sa)) + "/" + cs::ToStr(cs::CastDouble<int32_t>(sd)) + ", the port works out " + cs::ToStr(cs::CastDouble<int32_t>(pa)) + "/" + cs::ToStr(cs::CastDouble<int32_t>(pd)) + " from the level (it keeps no guild A/D of its own)");
        if (g == current) joined.Insert(0, m); else joined.Add(m);
    }
    if (joined.Count() == 0 || joined[0]->Guild != GuildData::All[current]->Name)
        return Refused("the current guild, " + GuildData::All[current]->Name + ", has no level: the record joins no guild the port can make current");
    c->Guilds = joined;
    if (joined.Count() > 1)
        Note(NoteKind::Info, where, "the current guild comes first and the rest follow by guild index (the port keeps guilds as a list, the original by index)");

    // ---- the pack: 40 slots, 0 the Hands; Ref < 1 past slot 0 is empty (MAINMOD2.BAS:1723)
    for (int32_t i = 1; i < Character::PackSize; i++)
        c->PutInSlot(i, SlotFrom(r.Sub("Pack(" + cs::ToStr(i) + ")"), where, "pack slot " + cs::ToStr(i + 1)));
    auto hands = r.Sub("Pack(0)");
    if (hands.Integer("Ref") != 0 || hands.Integer("Id") != 0)
        Note(NoteKind::Warning, where, "pack slot 1 (the Hands) holds item Ref " + cs::ToStr(hands.Integer("Ref")) + " Id " + cs::ToStr(hands.Integer("Id")) + "; the port's slot 1 is always the Hands");
    if (!r.Sub("Pack(40)").IsZero()) Note(NoteKind::Warning, where, "Pack(40), past the forty slots mc0190 walks, is not empty; nothing reads it");

    // ---- the bank: 40 slots (mc00A2); an emptied one stays a hole (FRMCITYB.BAS:187-199)
    auto bank = cs::List<cs::Ref<InventorySlot>>::New();
    for (int32_t i = 0; i < Character::BankSlots; i++) {
        auto slot = SlotFrom(r.Sub("M5860(" + cs::ToStr(i) + ")"), where, "bank slot " + cs::ToStr(i + 1));
        bank.Add(slot ? slot : cs::New<InventorySlot>());
    }
    while (bank.Count() > 0 && bank[bank.Count() - 1]->ItemId <= 0) bank.RemoveAt(bank.Count() - 1);
    c->Bank = bank;
    if (!r.Sub("M5860(40)").IsZero()) Note(NoteKind::Warning, where, "M5860(40), past the bank's forty slots, is not empty; nothing reads it");

    // ---- companions: Ref > -1 is a companion (MAINMOD2.BAS:383-400, 4745)
    for (int32_t s = 0; s < CompanionSlots::Slots; s++) {
        auto cr = r.Sub("Companions(" + cs::ToStr(s) + ")");
        int32_t cref = cr.Integer("Ref");
        cs::String cname = VbFile::RTrim(cr.FixedString("Name"));
        int32_t calign = cr.Integer("Alignment");
        auto comp = [&] {
            auto _o = cs::New<Companion>();
            _o->Ref = cref;
            _o->MonsterId = cr.Integer("M57D4");
            _o->Name = cname == "" ? cs::String(" ") : cname;
            _o->Hits = cr.Integer("Hits");
            _o->MaxHits = cr.Integer("MaxHits");
            // the foe's alignment, 0 Good to 2 Evil (fn1163, MAINMOD2.BAS:392); the port keeps the item scheme, one more
            _o->Alignment = calign >= 0 && calign <= 2 ? static_cast<ItemAlignment>(calign + 1) : ItemAlignment::Unaligned;
            _o->IdLevel = cr.Integer("IdLevel");
            _o->BindingLevel = cr.Integer("BindingLevel");
            return _o;
        }();
        if (cref > -1) {
            if (calign < 0 || calign > 2) Note(NoteKind::Warning, where, "companion " + cs::ToStr(s + 1) + "'s alignment " + cs::ToStr(calign) + " is not 0 to 2; kept as Unaligned");
            if (cref >= _monsters.Count())
                Note(NoteKind::Warning, where, "companion " + cs::ToStr(s + 1) + " is monster table index " + cs::ToStr(cref) + ", past MDATA5's " + cs::ToStr(_monsters.Count()) + "; the port settles it from its Id " + cs::ToStr(comp->MonsterId) + " on load");
            else {
                auto mon = _monsters[cref];
                if (mon->Id != comp->MonsterId)
                    Note(NoteKind::Warning, where, "companion " + cs::ToStr(s + 1) + ": table index " + cs::ToStr(cref) + " is " + mon->Name + " (Id " + cs::ToStr(mon->Id) + ") but the record's Id is " + cs::ToStr(comp->MonsterId));
                if (cr.Integer("Att") != mon->Att || cr.Integer("Def") != mon->Def)
                    Note(NoteKind::Info, where, "companion " + cs::ToStr(s + 1) + " (" + mon->Name + ") carries A/D " + cs::ToStr(cr.Integer("Att")) + "/" + cs::ToStr(cr.Integer("Def")) + "; the port uses the monster's " + cs::ToStr(mon->Att) + "/" + cs::ToStr(mon->Def) + " and keeps none per companion");
            }
            c->Companions->SetIndex(s, comp);
        } else {
            comp->Ref = -1;
            c->Companions->SetRemains(s, comp);
        }
    }
    if (!r.Sub("Companions(4)").IsZero() && r.Sub("Companions(4)").Integer("Ref") > -1)
        Note(NoteKind::Warning, where, "Companions(4), a fifth slot past the four mc00E0 walks, holds a companion; nothing reads it");

    // ---- carrying: fn04CD drops a carried body when the record is loaded (MAINMOD.BAS:354-356)
    int32_t carry = r.Integer("M5943");
    c->CarryingCompanion = carry >= 0 && carry < CompanionSlots::Slots ? carry : -1;
    c->CarryingCharacter = "";
    if (carry >= CompanionSlots::Slots)
        Note(NoteKind::Info, where, "was carrying a body; the original puts it down when the character is next loaded (fn04CD), and so does the import");

    // ---- ailments (MAINMOD2.BAS:51-65, sub048C)
    int32_t poison = r.Integer("Ailments(0)"), disease = r.Integer("Ailments(1)");
    c->PoisonLevel = poison; c->Poisoned = poison != 0;
    c->DiseaseLevel = disease; c->Diseased = disease != 0;
    c->ParalysisLevel = r.Integer("Ailments(2)");
    c->Levitating = r.Integer("Ailments(3)") != 0;
    c->InvisibleNow = r.Integer("Ailments(4)") != 0;
    c->SeeingInvisible = r.Integer("Ailments(5)") != 0;
    c->Protection = r.Integer("Ailments(6)");
    if (r.Integer("Ailments(7)") != 0) Note(NoteKind::Warning, where, "Ailments(7) holds " + cs::ToStr(r.Integer("Ailments(7)")) + "; nothing reads it");
    c->SpellEffects = r.Long("SpellEffects");
    for (int32_t i = 0; i < Effects::ResistanceKinds; i++) c->SpellResist[i] = r.Integer("SpellResistances(" + cs::ToStr(i) + ")");

    // ---- death: DeathState 0 alive, 1 Dead, 2 Stoned ... 8 Rock (gv0814)
    int32_t death = r.Integer("DeathState");
    if (death < 0 || death > 8) { Note(NoteKind::Warning, where, "DeathState " + cs::ToStr(death) + " is not 0 to 8; taken as Dead"); death = 1; }
    c->IsDead = death != 0;
    c->CauseOfDeath = death <= 1 ? Rules::DeathOrdinary : death;
    c->Stoned = death == Rules::DeathStoned;
    c->RaiseKind = r.Integer("M58DC");
    c->RaiseChance = r.Integer("M58E9");
    c->RaisedBy = VbFile::RTrim(r.FixedString("M58F9"));

    // ---- the tallies, M5906 (CHARCREA.BAS:248, MAINMOD2.BAS:5262-5290)
    auto Whole = [&](int32_t i) {
        return (int32_t)cs::Math::Clamp<int64_t>(r.Currency("M5906(" + cs::ToStr(i) + ")").RoundToWhole(), (int64_t)INT32_MIN, (int64_t)INT32_MAX);
    };
    c->Kills = Whole(0);
    c->Deaths = Whole(1);
    c->Complications = Whole(2);
    c->MonsterQuestsDone = Whole(3);
    c->ItemQuestsDone = Whole(4);
    c->PlaySeconds = r.Currency("M5906(5)").ToDouble();
    c->CreatedOn = r.Currency("M5906(6)").ToDouble();
    for (int32_t i = 7; i <= 8; i++)
        if (r.Currency("M5906(" + cs::ToStr(i) + ")").Raw != 0) Note(NoteKind::Warning, where, "M5906(" + cs::ToStr(i) + ") holds " + r.Currency("M5906(" + cs::ToStr(i) + ")").ToString() + "; nothing reads it");

    // ---- the Control Panel's options, M5914 (FRMCONTR.BAS:157-160, FRMPINFO.BAS:16, 53)
    c->Options = [&] {
        auto _o = cs::New<CharacterOptions>();
        _o->AutoChangeLevel = r.Integer("M5914(0)") != 0;
        _o->Hide = r.Integer("M5914(2)") != 0;
        _o->PreloadCharacter = r.Integer("M5914(3)") != 0;
        _o->NoPerceptionMessages = r.Integer("M5914(4)") != 0;
        return _o;
    }();
    c->CombatSetting = r.Integer("M5914(5)");

    // the password, M594C; a declined one is a lone space (CHARCREA.BAS:170)
    cs::String pw = VbFile::RTrim(r.FixedString("M594C"));
    c->Password = pw == "" ? Character::NoPassword : pw;

    // ---- the windows, M5958, in twips; the port keeps pixels at 96 dpi
    auto win = WindowLayout::Fresh();
    for (int32_t i = 0; i < win.Length() && i < 21; i++) {
        auto w = r.Sub("M5958(" + cs::ToStr(i) + ")");
        win[i] = [&] {
            auto _o = cs::New<WindowRect>();
            _o->Left = w.Long("M425F") / (double)WindowLayout::TwipsPerPixel;
            _o->Top = w.Long("M4258") / (double)WindowLayout::TwipsPerPixel;
            _o->Width = w.Long("M4216") / (double)WindowLayout::TwipsPerPixel;
            _o->Height = w.Long("M420C") / (double)WindowLayout::TwipsPerPixel;
            _o->State = w.Integer("M5801");
            return _o;
        }();
    }
    c->Windows = win;
    if (!r.Sub("M5958(20)").IsZero()) Note(NoteKind::Info, where, "window placement 20 is not empty; no form uses it and the port keeps twenty");

    // ---- spell buffers, M59B0(0 to 9)
    for (int32_t i = 0; i < SpellBuffers::Count; i++) c->Buffers[i] = r.Integer("M59B0(" + cs::ToStr(i) + ")");
    int32_t b10 = r.Integer("M59B0(10)");
    if (b10 != 0 && b10 != -1) Note(NoteKind::Info, where, "M59B0(10), an eleventh buffer no key reaches, holds " + cs::ToStr(b10));

    c->Swings = r.Integer("M56AF");

    c->MaxSpellPoints = Spellbook::MaxSpellPoints(c);
    if (c->CurSpellPoints > c->MaxSpellPoints)
        Note(NoteKind::Warning, where, cs::ToStr(c->CurSpellPoints) + " spell points against the " + cs::ToStr(c->MaxSpellPoints) + " (INT + WIS) * 5 allows; kept");
    int32_t swings = c->Swings;
    c->RecalcSwings();
    if (c->Swings != swings) Note(NoteKind::Info, where, "extra swings " + cs::ToStr(swings) + " in the record; the port's load works out " + cs::ToStr(c->Swings) + " from the guild levels");
    c->Swings = swings;

    cs::Func<cs::Ref<Item>(int16_t)> Lookup = [&](int16_t id) -> cs::Ref<Item> {
        cs::Ref<Item> it{};
        return _itemById.TryGetValue(id, it) ? it : nullptr;
    };
    for (int32_t i = 0; i < 6; i++) {
        int32_t mods = c->StatWithMods(i, Lookup) - c->Stats[i], stored = r.Integer("StatMods(" + cs::ToStr(i) + ")");
        if (mods != stored)
            Note(NoteKind::Warning, where, RaceData::StatNames[i] + " modifier: the record has " + cs::ToStr(stored) + ", the port works out " + cs::ToStr(mods) + " from the equipped gear (it keeps none of its own)");
    }
    if (r.Integer("StatMods(6)") != 0) Note(NoteKind::Warning, where, "StatMods(6) holds " + cs::ToStr(r.Integer("StatMods(6)")) + "; nothing reads it");

    // Att and Def are running totals in the record; the port keeps only ADShift
    int32_t statsAD = c->StatsADNow(Lookup);
    auto gagd = Rules::BestGuildAD(c);
    int32_t ga = gagd.att, gd = gagd.def;
    int32_t ia = c->Inventory().Where([&](auto& s) { return s->Equipped; }).Sum([&](auto& s) { auto it = Lookup(s->ItemId); return it ? (int32_t)it->Att : 0; });
    int32_t id = c->Inventory().Where([&](auto& s) { return s->Equipped; }).Sum([&](auto& s) { auto it = Lookup(s->ItemId); return it ? (int32_t)it->Def : 0; });
    int32_t att = cs::CastDouble<int32_t>(cs::Math::Round(r.Single("Att"), cs::MidpointRounding::ToEven)), def = cs::CastDouble<int32_t>(cs::Math::Round(r.Single("Def"), cs::MidpointRounding::ToEven));
    int32_t shiftA = att - (Rules::BaseAD + statsAD + ga + ia), shiftD = def - (Rules::BaseAD + statsAD + gd + id);
    if (shiftA == shiftD) {
        c->ADShift = shiftA;
        if (shiftA != 0) {
            // {shiftA:+0;-0}
            cs::String signed_ = shiftA >= 0 ? "+" + cs::ToStr(shiftA) : "-" + cs::ToStr(-(int64_t)shiftA);
            Note(NoteKind::Info, where, "Att/Def " + cs::ToStr(att) + "/" + cs::ToStr(def) + " are " + signed_ + " beyond base, best guild, stats and gear; kept as the port's ADShift"
                 + (c->ParalysisLevel != 0 ? cs::String(" (paralysed: the stat part is off while held)") : cs::String("")));
        }
    } else {
        c->ADShift = 0;
        Note(NoteKind::Warning, where, "Att/Def " + cs::ToStr(att) + "/" + cs::ToStr(def) + " differ from what the port works out by " + cs::ToStr(shiftA) + " and " + cs::ToStr(shiftD) + ", which one ADShift cannot hold; the port's own figures are used");
    }
    int64_t cap = Rules::Fn13ED(c->Guild()->Level, c->ActiveGuildDef()->ExpFactor, c->RaceDef()->ExpFactor);
    if (r.Long("M5970") != cap)
        Note(NoteKind::Info, where, "the stored experience cap M5970 is " + cs::ToStr(r.Long("M5970")) + ", fn13ED gives " + cs::ToStr(cap) + "; the port works it out each time");

    for (auto& unused : cs::List<cs::String>::Of({"M56F7", "M58A6", "M59CB", "M59D9"}))
        if (r.Integer(unused) != 0) Note(NoteKind::Info, where, unused + " holds " + cs::ToStr(r.Integer(unused)) + "; nothing in the game reads it");
    for (auto& unused : cs::List<cs::String>::Of({"M587C", "M588A"}))
        if (r.Single(unused) != 0) Note(NoteKind::Info, where, unused + " holds " + cs::ToStr(r.Single(unused)) + "; nothing in the game reads it");
    if (r.Integer("M5962") != recordNo)
        Note(NoteKind::Info, where, "M5962, the record number the game keeps while loaded, says " + cs::ToStr(r.Integer("M5962")) + "; the character is in record " + cs::ToStr(recordNo));

    auto _res = cs::New<ImportedCharacter>();
    _res->Record = recordNo; _res->Name = name; _res->Raw = raw; _res->Character = c;
    return _res;
}

cs::Ref<InventorySlot> OriginalSaves::SlotFrom(VbRecord s, cs::String where, cs::String which) {
    int32_t rf = s.Integer("Ref");
    if (rf < 1) return nullptr;
    int16_t id = s.Integer("Id");
    if (rf < _items.Count()) {
        auto it = _items[rf];
        if (it->Id != id)
            Note(NoteKind::Warning, where, which + ": table index " + cs::ToStr(rf) + " is " + it->Name + " (Id " + cs::ToStr(it->Id) + ") but the slot's Id is " + cs::ToStr(id) + "; the table's is used, as the game reads Items(Ref)");
        id = it->Id;
    } else {
        Note(NoteKind::Warning, where, which + ": table index " + cs::ToStr(rf) + " is past MDATA3's " + cs::ToStr(_items.Count()) + " items; kept by its Id " + cs::ToStr(id)
             + (_itemById.ContainsKey(id) ? cs::String("") : cs::String(", which no item has")));
    }
    int32_t known = s.Integer("IdLevel");
    if (known < 0 || known > 3) { Note(NoteKind::Warning, where, which + ": ID level " + cs::ToStr(known) + " is not 0 to 3; taken as fully known"); known = 0; }
    int32_t align = s.Integer("Alignment");
    if (align < 0 || align > 3) { Note(NoteKind::Warning, where, which + ": alignment " + cs::ToStr(align) + " is not U, G, N or E; taken as Unaligned"); align = 0; }
    auto _o = cs::New<InventorySlot>();
    _o->ItemId = id;
    _o->Equipped = s.Integer("Equipped") != 0;
    _o->Charges = s.Integer("Charges");
    // the original counts identification down, 0 complete (StoreStock.IdCountdown)
    _o->set_Known(static_cast<IdLevel>(3 - known));
    _o->SlotCursed = s.Integer("Cursed") != 0;
    _o->Alignment = static_cast<ItemAlignment>(align);
    return _o;
}

int32_t OriginalSaves::ReadStore(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::StoreLen);
    if (!StampOk(f, file, "the General Store's shelves")) return f->RecordCount();
    int32_t n = f->Integer(3);
    if (n != _items.Count())
        Note(NoteKind::Warning, file, "has " + cs::ToStr(n) + " shelves and MDATA3 " + cs::ToStr(_items.Count()) + " items; each shelf is taken by its item Id, as fn0425 re-maps a store file");
    auto stock = cs::New<StoreStock>();
    for (int32_t i = 0; i < n && f->Has(i + 4); i++) {
        auto s = f->Record(i + 4, VbLayout::Shelf);
        int16_t id = s.Integer("Id");
        if (id == 0) continue;
        if (!_itemById.ContainsKey(id)) { Note(NoteKind::Warning, file, "shelf " + cs::ToStr(i) + " holds item Id " + cs::ToStr(id) + ", which MDATA3 has not; dropped"); continue; }
        if (n == _items.Count() && (i >= _items.Count() || _items[i]->Id != id))
            Note(NoteKind::Warning, file, "shelf " + cs::ToStr(i) + " holds Id " + cs::ToStr(id) + " where MDATA3's item " + cs::ToStr(i) + " is Id " + cs::ToStr(i < _items.Count() ? (int32_t)_items[i]->Id : -1) + "; taken by its Id");
        for (int32_t a = 0; a < 4; a++) {
            int32_t count = s.Integer("Align(" + cs::ToStr(a) + ")");
            if (count > 0) stock->Shelf[cs::ToStr(id) + ":" + cs::ToStr(a)] = count;
            else if (count < 0) Note(NoteKind::Warning, file, "item Id " + cs::ToStr(id) + ": a count of " + cs::ToStr(count) + " at alignment " + cs::ToStr(a) + "; dropped");
        }
        if (s.Integer("Align(4)") != 0) Note(NoteKind::Warning, file, "item Id " + cs::ToStr(id) + ": Align(4) holds " + cs::ToStr(s.Integer("Align(4)")) + "; the store has four alignments (gc03E6)");
    }
    Store = stock;
    return f->RecordCount();
}

int32_t OriginalSaves::ReadPens(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::PensLen);
    if (!StampOk(f, file, "the Confinement's pens")) return f->RecordCount();
    int32_t n = f->Integer(3);
    if (n != _monsters.Count())
        Note(NoteKind::Warning, file, "has " + cs::ToStr(n) + " pens and MDATA5 " + cs::ToStr(_monsters.Count()) + " monsters; each pen is taken by its creature's Id, as fn0425 re-maps a companion store file");
    auto pens = cs::New<Confinement>();
    pens->KeyedBy = Confinement::ByRef;
    for (int32_t i = 0; i < n && f->Has(i + 4); i++) {
        auto s = f->Record(i + 4, VbLayout::Shelf);
        int16_t id = s.Integer("Id");
        if (id == 0 && s.IsZero()) continue;
        int32_t at = i < _monsters.Count() && _monsters[i]->Id == id ? i : -1;
        if (at < 0) {
            at = _monsters.ToList().FindIndex([&](auto& m) { return m->Id == id; });
            if (at < 0) { Note(NoteKind::Warning, file, "pen " + cs::ToStr(i) + " holds monster Id " + cs::ToStr(id) + ", which MDATA5 has not; dropped"); continue; }
            Note(NoteKind::Warning, file, "pen " + cs::ToStr(i) + " holds Id " + cs::ToStr(id) + "; filed under table index " + cs::ToStr(at) + ", the first creature with it");
        }
        // Align(0 to 2), the monster alignments; the port's key is the item scheme, one more
        for (int32_t a = 0; a < 3; a++) {
            int32_t count = s.Integer("Align(" + cs::ToStr(a) + ")");
            if (count > 0) pens->Pens[cs::ToStr(at) + ":" + cs::ToStr(a + 1)] = count;
        }
        for (int32_t a = 3; a <= 4; a++)
            if (s.Integer("Align(" + cs::ToStr(a) + ")") != 0) Note(NoteKind::Warning, file, _monsters[at]->Name + ": Align(" + cs::ToStr(a) + ") holds " + cs::ToStr(s.Integer("Align(" + cs::ToStr(a) + ")")) + "; a creature is Good, Neutral or Evil");
    }
    Pens = pens;
    return f->RecordCount();
}

int32_t OriginalSaves::ReadGuildmasters(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::GuildmastersLen);
    if (!StampOk(f, file, "the guildmasters")) return f->RecordCount();
    // T564C, a variable-length Name and a Level, one record per guild from record 2
    auto gm = cs::New<mc::Guildmasters>();
    for (int32_t g = 0; g < GuildData::All.Count(); g++) {
        if (!f->Has(g + 2)) { Note(NoteKind::Warning, file, "has no record for " + GuildData::All[g]->Name + "; the port appoints its own guildmaster there"); continue; }
        auto rec = f->Record(g + 2);
        int32_t next = 0;
        cs::String name = VbFile::VarString(rec, 0, next);
        int32_t level = next + 2 <= rec.Length() ? cs::BinaryPrimitives::ReadInt16LittleEndian(rec.Raw().data() + next) : 0;
        if (name == "") continue;
        auto holder = cs::New<Guildmasters_Holder>();
        holder->Name = name; holder->Level = level;
        gm->Held[g] = holder;
    }
    Guildmasters = gm;
    return f->RecordCount();
}

int32_t OriginalSaves::ReadAutomap(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::AutomapLen);
    if (!StampOk(f, file, "the automap")) return f->RecordCount();
    DeepestReached = (int32_t)f->Integer(2);
    // fn0A34 (MAZEMOD.BAS:732): record (L - 1) * 900 + x + 30 * (y - 1) + 2
    const int32_t W = 30, H = 30;
    auto map = cs::New<SharedMap>();
    for (int32_t level = 1; level <= 15; level++) {
        auto cells = cs::List<int32_t>::New();
        for (int32_t i = 1; i <= W * H; i++) {
            int32_t rec = (level - 1) * W * H + i + 2;
            if (!f->Has(rec)) break;
            auto bytes = f->Record(rec);
            if (bytes.Length() < 4) throw cs::ArgumentOutOfRangeException("length");
            int64_t word = (uint32_t)cs::BinaryPrimitives::ReadInt32LittleEndian(bytes.Raw().data());
            if (word == 0) continue;
            int32_t portIndex = i - 1;   // (y - 1) * 30 + (x - 1), the port's 0-based y * width + x
            cs::String key = cs::ToStr(level) + ":" + cs::ToStr(portIndex);
            if ((word & (1LL << BeenHereBit)) != 0) cells.Add(portIndex);
            int64_t flags = word & ~(1LL << BeenHereBit);
            if (flags != 0) {
                map->Seen[key] = flags;
                // mdrmazPerceiveSquare, MAZEMOD.BAS:632-634; the port asks Noticed
                auto bits = cs::List<int32_t>::New();
                for (int32_t b = 0; b < 32; b++) if ((flags & (1LL << b)) != 0) bits.Add(b);
                map->Noticed[key] = bits;
            }
        }
        if (cells.Count() > 0) map->Explored[level] = cells;
    }
    Automap = map;
    return f->RecordCount();
}

int32_t OriginalSaves::ReadGuildLog(cs::String path, cs::String file) {
    // Binary, not records (sub0578, HALL.BAS:25-50)
    cs::List<uint8_t> d = cs::File::ReadAllBytes(path);
    auto entries = cs::List<cs::Ref<GuildLog_Entry>>::New();
    if (d.Length() >= 2) {
        int32_t count = cs::BinaryPrimitives::ReadInt16LittleEndian(d.Raw().data());
        int32_t at = 2;
        while (at + 8 <= d.Length()) {
            int32_t guild = cs::BinaryPrimitives::ReadInt16LittleEndian(d.Raw().data() + at);
            int32_t day = cs::BinaryPrimitives::ReadInt32LittleEndian(d.Raw().data() + at + 2);
            int32_t len = cs::BinaryPrimitives::ReadInt16LittleEndian(d.Raw().data() + at + 6);
            if (len < 0 || at + 8 + len > d.Length()) { Note(NoteKind::Warning, file, "entry " + cs::ToStr(entries.Count() + 1) + " runs past the end of the file; the log is taken up to it"); break; }
            auto e = cs::New<GuildLog_Entry>();
            e->Guild = guild; e->Day = day; e->Text = VbFile::Text(d.GetRange(at + 8, len));
            entries.Add(e);
            at += 8 + len;
        }
        if (count != entries.Count())
            Note(NoteKind::Info, file, "the header counts " + cs::ToStr(count) + " entries and the file holds " + cs::ToStr(entries.Count()) + "; every entry is kept (the header is only bumped, FRMGLDL reads the records)");
        for (auto& e : entries.Where([&](auto& e) { return e->Guild < 0 || e->Guild >= GuildData::All.Count(); }).Take(1))
            Note(NoteKind::Warning, file, "an entry is filed under guild " + cs::ToStr(e->Guild) + ", which MDATA1 has not");
    }
    GuildLog = entries;
    Files.Add(MakeImportFile(file, "the guild log", "read, " + cs::ToStr(entries.Count()) + " entries", entries.Count()));
    return entries.Count();
}

int32_t OriginalSaves::ReadRooms(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::RoomsLen);
    int32_t levels = f->Integer(1);
    if (levels != 15) Note(NoteKind::Warning, file, "record 1 says " + cs::ToStr(levels) + " levels where sub04A3 writes 15 (mc0118); 15 are read");
    auto state = cs::New<DungeonState>();
    int32_t rooms = 0;
    for (int32_t level = 1; level <= 15; level++) {
        // fn0DDC (MONSMOD.BAS:2213): the level's respawn tally at record 1 + level
        if (f->Has(1 + level)) state->Tally[level] = (int32_t)f->Integer(1 + level);
        for (int32_t area = 1; area <= 200; area++) {
            // fn0E38 (MONSMOD.BAS:2380-2382)
            int32_t first = (area - 1) * 5 + 17 + (level - 1) * 1000;
            if (!f->Has(first)) break;
            auto foes = cs::Enumerable::Range(0, 4).Select([&](int32_t s) { return f->Record(first + s, VbLayout::Foe); }).ToArray();
            auto fight = f->Record(first + 4, VbLayout::Fight);
            if (foes.All([&](auto& x) { return x.IsZero(); }) && fight.IsZero()) continue;
            auto room = cs::New<RoomRecord>();
            room->Foes = foes.Select([&](auto& x) {
                auto _o = cs::New<FoeSlot>();
                _o->Att = x.Integer("Att"); _o->Def = x.Integer("Def"); _o->Hits = x.Integer("Hits"); _o->MaxHits = x.Integer("MaxHits");
                _o->Alignment = x.Integer("Alignment"); _o->Engaged = x.Integer("Engaged"); _o->Ref = x.Integer("Ref");
                _o->GroupCount = x.Integer("GroupCount"); _o->Stamp = x.Single("M4EE6"); _o->IdLevel = x.Integer("IdLevel");
                _o->JoinOffer = x.Integer("M4EF9"); _o->KnownAs = x.Integer("KnownAs");
                return _o;
            }).ToArray();
            room->TreasureClass = fight.Integer("TreasureClass");
            room->Lead = fight.Integer("M4F24");
            room->Gold = fight.Currency("Gold").ToDecimal();
            room->ChestTrap = fight.Integer("ChestTrap");
            room->ChestLock = fight.Integer("ChestLock");
            for (auto& fs : room->Foes)
                if (fs->GroupCount > 0 && (fs->Ref < 0 || fs->Ref >= _monsters.Count()))
                    Note(NoteKind::Warning, file, "level " + cs::ToStr(level) + " area " + cs::ToStr(area) + ": a standing group of monster table index " + cs::ToStr(fs->Ref) + ", past MDATA5's " + cs::ToStr(_monsters.Count()));
            state->StoreRoom(level, area, room);
            rooms++;
        }
    }
    Dungeon = state;
    Files.Add(MakeImportFile(file, "the dungeon's rooms and respawn tallies", "read, " + cs::ToStr(rooms) + " rooms with records", f->RecordCount()));
    return f->RecordCount();
}

int32_t OriginalSaves::ReadParties(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::PartiesLen);
    if (f->Length() == 0) { Files.Add(MakeImportFile(file, "the remembered parties", "read, empty (no party remembered)", 0)); return 0; }
    // fn179B (MULTIMOD.BAS:387-444): record 1 a count, then four names a party from record 2
    int32_t count = f->Integer(1);
    int32_t groups = (f->RecordCount() - 1) / 4;
    for (int32_t g = 0; g < groups; g++) {
        int32_t rec = 2 + g * 4;
        cs::String leader = f->VarString(rec);
        if (cs::Str::Trim(leader) == "") continue;
        auto members = cs::List<cs::String>::Of({leader});
        for (int32_t m = 1; m < 4; m++) {
            cs::String who = f->VarString(rec + m);
            if (cs::Str::Trim(who) != "") members.Add(who);
        }
        auto p = cs::New<ImportedParty>();
        p->Leader = leader; p->Members = members;
        Parties.Add(p);
    }
    if (count != groups)
        Note(NoteKind::Info, file, "the header counts " + cs::ToStr(count) + " parties and the file holds " + cs::ToStr(groups) + " groups of four; every named group is read");
    Files.Add(MakeImportFile(file, "the remembered parties", "read, " + cs::ToStr(Parties.Count()) + " parties", f->RecordCount()));
    return f->RecordCount();
}

int32_t OriginalSaves::ReadLibrary(cs::String path, cs::String file) {
    auto Empty = [](VbRecord r) {
        return r.Integer("IdLevel") == 0 && r.Currency("M5C00").Raw == 0 && VbFile::RTrim(r.FixedString("M5C09")) == ""
            && r.Long("M5C21") == 0 && VbFile::RTrim(r.FixedString("M5C2F")) == "" && r.Integer("M5C3D") == 0;
    };
    auto Fill = [](cs::Ref<LibraryRecord> e, VbRecord r) {
        e->Id = r.Integer("Id");
        e->Known = r.Integer("IdLevel");
        e->NSeen = r.Currency("M5C00").RoundToWhole();
        e->Name = VbFile::RTrim(r.FixedString("M5C09"));
        e->MonId = r.Integer("M5C14");
        e->LastSeen = r.Long("M5C21");
        e->Location = VbFile::RTrim(r.FixedString("M5C2F"));
        e->NDeaths = r.Integer("M5C3D");
    };

    auto f = VbFile::Open(path, VbLayout::LibraryLen);
    if (!StampOk(f, file, "the Library")) return f->RecordCount();
    int32_t nMon = f->Integer(2), nItems = f->Integer(3);
    Note(NoteKind::Info, file, "its counts of monsters and items found (" + cs::ToStr(f->Integer(4)) + " and " + cs::ToStr(f->Integer(5)) + ", records 4 and 5) have no place in the port's library.json: the port keeps no such counts");
    // sub0F9A (HALL.BAS:60-80): monster i at record 6 + i, item j at 6 + monsters + j
    bool monOk = nMon == _monsters.Count(), itemOk = nItems == _items.Count();
    if (!monOk) Note(NoteKind::Warning, file, "has " + cs::ToStr(nMon) + " monster records and MDATA5 " + cs::ToStr(_monsters.Count()) + "; each is taken by its Id (mdrUpdateLibraryFile re-indexes the same way)");
    if (!itemOk) Note(NoteKind::Warning, file, "has " + cs::ToStr(nItems) + " item records and MDATA3 " + cs::ToStr(_items.Count()) + "; each is taken by its Id");
    auto lib = cs::New<Librarian_Store>();
    auto monsterList = _monsters.ToList();
    for (int32_t i = 0; i < nMon && f->Has(6 + i); i++) {
        auto rec = f->Record(6 + i, VbLayout::Library);
        if (Empty(rec)) continue;
        int32_t at = monOk ? i : monsterList.FindIndex([&](auto& m) { return m->Id == rec.Integer("Id"); });
        if (at < 0) { Note(NoteKind::Warning, file, "monster record " + cs::ToStr(i) + " has Id " + cs::ToStr(rec.Integer("Id")) + ", which MDATA5 has not; dropped"); continue; }
        Fill(lib->EditMonster(at), rec);
    }
    for (int32_t j = 0; j < nItems && f->Has(6 + nMon + j); j++) {
        auto rec = f->Record(6 + nMon + j, VbLayout::Library);
        if (Empty(rec)) continue;
        int16_t id = rec.Integer("Id");
        if (id == 0 && itemOk && j < _items.Count()) id = _items[j]->Id;
        if (!_itemById.ContainsKey(id)) { Note(NoteKind::Warning, file, "item record " + cs::ToStr(j) + " has Id " + cs::ToStr(id) + ", which MDATA3 has not; dropped"); continue; }
        Fill(lib->EditItem(id), rec);
    }
    Library = lib;
    return f->RecordCount();
}

int32_t OriginalSaves::ReadHall(cs::String path, cs::String file) {
    auto f = VbFile::Open(path, VbLayout::HallLen);
    // no version here: sub0595 writes the first record over the stamp (MAINMOD.BAS:607-610); fn1C07 reads record k + 1 (HALL.BAS:617)
    auto hall = cs::New<HallRecords>();
    for (int32_t ki = 0; ki <= 11; ki++) {
        auto k = static_cast<HallRecords_Kind>(ki);
        int32_t rec = (int32_t)k + 1;
        if (!f->Has(rec)) { Note(NoteKind::Warning, file, "has no record for " + HallRecords::Heading(k) + "; the port's own first holder stands"); continue; }
        auto r = f->Record(rec, VbLayout::Hall);
        auto _o = cs::New<HallRecords_Record>();
        _o->Value = r.Currency("M73F5");
        _o->Holder = VbFile::RTrim(r.FixedString("M73FE"));
        _o->Day = r.Long("M7406");
        hall->Records[(int32_t)k] = _o;
    }
    Hall = hall;
    return f->RecordCount();
}

}  // namespace mc
