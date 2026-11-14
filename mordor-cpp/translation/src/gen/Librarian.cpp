#include "core.h"

namespace mc {

cs::String Librarian::Size(int32_t i) { return i >= 0 && i < Sizes.Length() ? Sizes[i] : cs::String("oddly"); }
cs::String Librarian::Align(int32_t i) { return i >= 0 && i < Alignments.Length() ? Alignments[i] : cs::String("Neutral"); }
bool Librarian::Bit(int64_t mask, int32_t b) { return (mask & (1LL << b)) != 0; }

cs::String Librarian::Listed(cs::List<cs::String> parts) { return cs::Str::Join(", ", parts); }

cs::String Librarian::DescribeMonster(cs::Ref<Monster> m, cs::Ref<LibraryRecord> e, int32_t today, cs::String plural)
{
    auto s = cs::New<cs::StringBuilder>();
    s->Append("MONSTER: ").Append(m->Name).Append("\r\n\r\n");

    switch (e->Known)
    {
        case VeryLittle:
            s->Append("All that is known about " + plural + " is that they are " + Size(m->Size) + " in Size.");
            break;
        case AGoodDealMore:
            s->Append("All that is known about " + plural + " is that they are " + Size(m->Size) + " sized "
                      + Mdata1::MonsterClassName(m->Type) + "s.");
            break;
        default:
            s->Append(plural + " are " + Size(m->Size) + " sized " + Mdata1::MonsterClassName(m->Type) + "s who are "
                      + "generally " + Align(m->AlignmentValue) + " in nature.\r\n");
            s->Append("With an average A/D of " + cs::ToStr(m->Att) + "/" + cs::ToStr(m->Def) + ", and average hits of " + cs::ToStr(m->Hits) + " and a ");
            for (int32_t i = 0; i < 6 && i < m->Stats.Length(); i++)
                if (m->Stats[i] > 0) s->Append(cs::ToStr(m->Stats[i]) + " " + RaceData::StatNamesLong[i] + ", ");
            break;
    }

    if (e->Known == AlmostEverything)
        s->Append("it is not yet known if or what special abilities this creature has.");
    else if (e->Known == Everything)
    {
        s->Append("it is also known that these creatures");
        auto told = cs::List<cs::String>::New();
        int32_t attacks = 0;
        for (int32_t i = 0; i < MonsterProperties.Length(); i++)
            if (Bit(m->SpecialPropertyFlags, i)) told.Add(MonsterProperties[i]);
        for (int32_t i = 0; i < MonsterAttacks.Length(); i++)
            if (Bit(m->SpecialAttackFlags, i)) { attacks++; told.Add(MonsterAttacks[i]); }
        for (int32_t i = 0; i < SpellSchools.Length(); i++)
            if (Bit(m->SpellFlags, i)) told.Add("\"" + SpellSchools[i] + "\" Spells");

        if (told.Count() == 0) s->Append(" have no special attacks or abilities.");
        else s->Append(attacks == 0 ? cs::String(" abilities include: ") : cs::String(" abilities and attacks include: "))
                .Append(Listed(told)).Append('.');

        s->Append("\r\n").Append(plural).Append(" are ");
        auto res = cs::List<cs::String>::New();
        for (int32_t i = 0; i < Resistances.Length() && i < m->Resistances.Length(); i++)
            if (m->Resistances[i] > 0) res.Add(cs::ToStr(m->Resistances[i]) + "% " + Resistances[i]);
        s->Append(res.Count() == 0 ? cs::String("not resistant to anything.") : Listed(res) + " resistant.");
    }

    cs::String estimate;
    if (e->Known == Everything) estimate = "It is estimated that all is known about this creature.";
    else if (e->Known == AlmostEverything) estimate = "It is estimated that almost everything is known about this creature.";
    else if (e->Known == AGoodDealMore) estimate = "It is estimated that there is a good amount more to learn about this creature.";
    else estimate = "It is estimated that very little is known about this creature.";
    s->Append("\r\n\r\n").Append(estimate);

    s->Append("\r\n\r\n");
    s->Append(e->NDeaths == 0
        ? cs::String("There are no known deaths associated with this monster.")
        : "Records have it that Explorers have been killed by this creature " + cs::ToStr(e->NDeaths) + " "
          + (e->NDeaths == 1 ? cs::String("time.") : cs::String("times.")));

    s->Append("\r\n");
    if (e->NSeen > 0) s->Append("A total of " + cs::ToStr(e->NSeen) + " of these creatures have been seen.\r\n");

    s->Append(cs::Str::TrimEnd(e->Name) == ""
        ? cs::String("It is not known who first recorded this Creature.")
        : "\"" + cs::Str::TrimEnd(e->Name) + "\" was the first to record this Creature.");

    s->Append("\r\n");
    s->Append(cs::Str::TrimEnd(e->Location) == ""
        ? cs::String("This creature was last seen by a lost character, ")
        : "This creature was last seen at " + cs::Str::TrimEnd(e->Location) + ", ");
    s->Append(DaysAgo(e->LastSeen, today));
    return s->ToString();
}

cs::String Librarian::DescribeItem(cs::Ref<Item> it, cs::Ref<LibraryRecord> e, int32_t today, cs::Func<cs::String(int32_t)> monsterName)
{
    bool all = e->Known == Everything;
    auto s = cs::New<cs::StringBuilder>();
    s->Append("ITEM: ").Append(it->Name).Append("\r\n\r\n");
    s->Append("This item is a member of the " + Mdata1::ItemTypeName(it->Type) + " class.\r\n");
    s->Append("Offering an A/D of " + cs::ToStr(it->Att) + "/" + cs::ToStr(it->Def));

    // the damage modifier is only shown for a weapon - class 0 - and only when all is known
    if (all && it->DamageMod != 0 && Character::WearClassOf(it) == 0)
        s->Append(" and a damage modifier of " + cs::ToStr(it->DamageMod));
    s->Append(" to a properly aligned character, this item ");

    if (all && it->Cursed != 0) s->Append("is CURSED, ");

    if (it->NHands > 0)
    {
        if (it->Swings == 1) s->Append("offers 1 swing and ");
        else if (it->Swings > 1) s->Append("offers " + cs::ToStr(it->Swings) + " swings and ");
        s->Append("is a " + Hand(it->NHands) + "-handed Item");
    }
    else s->Append("requires no hands");

    if (all && it->SpellIndex > 0)
        s->Append(" and Casts \"" + SpellName(it->SpellIndex) + "\" up to " + cs::ToStr(it->Charges) + " times.");
    else s->Append('.');
    s->Append("\r\n");

    if (all)
    {
        s->Append("This item ");
        auto told = cs::List<cs::String>::New();
        for (int32_t i = 0; i < ItemAbilities.Length(); i++)
            if (Bit(it->Abilities, i)) told.Add(ItemAbilities[i] + " Ability");
        for (int32_t i = 0; i < Resistances.Length(); i++)
            if (Bit(it->ResistanceFlags, i)) told.Add(Resistances[i] + " Resistance");
        s->Append(told.Count() > 0
            ? "also offers the following when equipped: " + Listed(told) + "."
            : cs::String("offers no special abilities or resistances."));
    }
    s->Append("\r\n");

    s->Append("Requiring ");
    auto needs = cs::List<cs::String>::New();
    for (int32_t i = 0; i < 6 && i < it->StatsRequired.Length(); i++)
        if (it->StatsRequired[i] != 0) needs.Add(cs::ToStr(it->StatsRequired[i]) + " " + RaceData::StatNamesLong[i]);
    s->Append(needs.Count() > 0 ? Listed(needs) + " to equip or use," : cs::String("no stats to equip or use,"));

    auto mods = cs::List<cs::String>::New();
    for (int32_t i = 0; i < 6 && i < it->StatsMod.Length(); i++)
        if (it->StatsMod[i] != 0) mods.Add(cs::ToStr(it->StatsMod[i]) + " " + RaceData::StatNamesLong[i]);

    if (all)
    {
        if (it->SpecialType >= 1 && it->SpecialType <= 10)
        {
            if (mods.Count() > 0) s->Append(" and adding " + Listed(mods) + " permanently when used,");
        }
        else if (it->SpecialType == 11) s->Append(" and adding 50 Spell Points when used,");
        else if (it->SpecialType == 12) s->Append(" and adding 200 Spell Points when used,");
        else if (it->SpecialType == 13) s->Append(" and subtracting 1 year of age when used,");
        else if (it->SpecialType == 14) s->Append(" and subtracting 10 years of age when used,");
        else
        {
            if (mods.Count() > 0) s->Append(" and modifying " + Listed(mods) + " when equipped,");
        }
        s->Append(it->ClassRestricted != 0
            ? cs::String(" this is a Class Restricted Item ") : cs::String(" this is a non-Class Restricted Item "));
        s->Append("that is usable by the following Guilds: ");
    }
    else s->Append(" this item is usable by the following Guilds: ");

    auto guilds = cs::List<cs::String>::New();
    for (int32_t i = 0; i < GuildData::All.Count(); i++)
        if (Bit(it->Guilds, i))
            guilds.Add(all
                ? GuildData::All[i]->Name + " (" + cs::ToStr(Character::UseLevel(it, GuildData::All[i])) + ")"
                : GuildData::All[i]->Name);
    s->Append(Listed(guilds)).Append(".\r\n\r\n");

    s->Append(all
        ? cs::String("It is estimated that all is known about this Item.")
        : cs::String("It is estimated that almost everything is known about this Item."));
    s->Append("\r\n\r\n");

    if (e->NSeen > 0)
    {
        s->Append("Reports have it that " + cs::ToStr(e->NSeen) + " of these items have been found, the last one from a ");
        cs::String from = e->MonId >= 0 && monsterName ? monsterName(e->MonId) : cs::String("");
        s->Append(from == "" ? cs::String("unknown monster on level ") : from + " on level ");
        s->Append(cs::Str::TrimEnd(e->Location));
        s->Append(DaysAgoForItem(e->LastSeen, today));
        s->Append("\r\n");
    }

    s->Append(cs::Str::TrimEnd(e->Name) == ""
        ? cs::String("It is not known who first recorded this Item.")
        : "\"" + cs::Str::TrimEnd(e->Name) + "\" was the first to record this Item.");
    return s->ToString();
}

cs::String Librarian::Hand(int32_t n) { return n >= 0 && n < HandWords.Length() ? HandWords[n] : cs::ToStr(n); }

cs::String Librarian::SpellName(int32_t oneBasedIndex) { return SpellNameOf(oneBasedIndex - 1); }

cs::String Librarian::DaysAgo(int32_t seen, int32_t today)
{
    if (today == 0) return "it is not known how many days ago.";
    int32_t days = cs::Math::Abs(seen - today);
    return days == 0 ? cs::String("just Today.") : days == 1 ? cs::String("just 1 day ago.") : cs::ToStr(days) + " days ago.";
}

cs::String Librarian::DaysAgoForItem(int32_t seen, int32_t today)
{
    if (today == 0) return "it is not known how many days ago.";
    int32_t days = cs::Math::Abs(seen - today);
    return days == 0 ? cs::String(" just Today.") : days == 1 ? cs::String(", just 1 day ago.") : ", " + cs::ToStr(days) + " days ago.";
}

int32_t Librarian::Serial(cs::DateTime when)
{
    const int64_t ticksPerDay = 864000000000LL;
    int64_t date = when.ticks - when.ticks % ticksPerDay;
    // new DateTime(1899, 12, 30).Ticks
    const int64_t epoch = 599264352000000000LL;
    double totalDays = (double)(date - epoch) / (double)ticksPerDay;
    return cs::CastDouble<int32_t>(totalDays);
}

// ---------------------------------------------------------------- the shared file

cs::String Librarian_Store::MonsterKey(int32_t tableIndex) { return MonsterPrefix + cs::ToStr(tableIndex); }
cs::String Librarian_Store::ItemKey(int32_t itemId) { return ItemPrefix + cs::ToStr(itemId); }

void Librarian_Store::MonstersSettled()
{
    if (MonstersUnsettled)
        throw cs::InvalidOperationException("the Library was loaded without the monster table, and its creature records are still keyed by Id");
}

cs::Ref<LibraryRecord> Librarian_Store::Get(cs::String k)
{
    cs::Ref<LibraryRecord> e{};
    return Entries.TryGetValue(k, e) && e != nullptr ? e : cs::New<LibraryRecord>();
}

cs::Ref<LibraryRecord> Librarian_Store::Make(cs::String k)
{
    cs::Ref<LibraryRecord> e{};
    if (!Entries.TryGetValue(k, e) || e == nullptr) { e = cs::New<LibraryRecord>(); Entries[k] = e; }
    return e;
}

cs::Ref<LibraryRecord> Librarian_Store::ForMonster(int32_t tableIndex) { MonstersSettled(); return Get(MonsterKey(tableIndex)); }

cs::Ref<LibraryRecord> Librarian_Store::EditMonster(int32_t tableIndex) { MonstersSettled(); return Make(MonsterKey(tableIndex)); }

cs::Ref<LibraryRecord> Librarian_Store::ForItem(int32_t itemId) { return Get(ItemKey(itemId)); }

cs::Ref<LibraryRecord> Librarian_Store::EditItem(int32_t itemId) { return Make(ItemKey(itemId)); }

bool Librarian_Store::KnowsMonster(int32_t tableIndex) { MonstersSettled(); return Known(MonsterKey(tableIndex)); }

bool Librarian_Store::KnowsItem(int32_t itemId) { return Known(ItemKey(itemId)); }

bool Librarian_Store::Known(cs::String k)
{
    cs::Ref<LibraryRecord> e{};
    return Entries.TryGetValue(k, e) && e != nullptr && e->Known > 0;
}

int32_t Librarian_Store::SettleMonsterKeys(cs::List<cs::Ref<Monster>> table)
{
    int32_t moved = 0;
    auto keys = Entries.Keys().Where([&](auto& k) { return cs::Str::StartsWith(k, LegacyMonsterPrefix); }).ToList();
    for (auto& key : keys)
    {
        int16_t id = 0;
        if (!cs::Parse::TryParse<int16_t>(cs::Str::Substring(key, (int32_t)LegacyMonsterPrefix.size()), id)) continue;
        int32_t r = CompanionRef::FromId(table, id, ItemAlignment::Unaligned);
        if (r < 0) continue;
        cs::Ref<LibraryRecord> from = Entries.Get(key);
        Entries.Remove(key);
        cs::String to = MonsterKey(r);
        cs::Ref<LibraryRecord> there{};
        if (Entries.TryGetValue(to, there) && there != nullptr) Merge(there, from);
        else Entries[to] = from;
        moved++;
    }
    MonstersUnsettled = false;
    return moved;
}

void Librarian_Store::Merge(cs::Ref<LibraryRecord> there, cs::Ref<LibraryRecord> newer)
{
    if (there->Known == 0) there->Name = newer->Name;
    if (there->Known < newer->Known) there->Known = newer->Known;
    there->NSeen += newer->NSeen;
    there->NDeaths = cs::Wrap<int16_t>(cs::Math::Min((int32_t)INT16_MAX, (int32_t)there->NDeaths + (int32_t)newer->NDeaths));
    if (newer->Location != "") there->Location = newer->Location;
    if (newer->LastSeen > there->LastSeen) there->LastSeen = newer->LastSeen;
    if (there->Id == 0) there->Id = newer->Id;
}

cs::String Librarian_Store::PathFor(cs::String savesDir) { return cs::Path::Combine(savesDir, "library.json"); }

cs::Ref<Librarian_Store> Librarian_Store::Load(cs::String savesDir, cs::List<cs::Ref<Monster>> monsters)
{
    auto s = Read(savesDir);
    if (s->Entries.Keys().Any([&](auto& k) { return cs::Str::StartsWith(k, LegacyMonsterPrefix); }))
    {
        if (monsters != nullptr) s->SettleMonsterKeys(monsters);
        else s->MonstersUnsettled = true;
    }
    return s;
}

cs::Ref<Librarian_Store> Librarian_Store::Read(cs::String savesDir)
{
    cs::String p = PathFor(savesDir);
    if (!cs::File::Exists(p)) return cs::New<Librarian_Store>();
    cs::String text;
    try { text = cs::File::ReadAllText(p); }
    catch (const std::exception& ex)
    {
        if (Note) Note(p + " could not be read (" + cs::String(ex.what()) + "); the Library is not written to this time");
        return [&] { auto _o = cs::New<Librarian_Store>(); _o->ReadOnly = true; return _o; }();
    }
    try
    {
        auto s = cs::Json::Deserialize<cs::Ref<Librarian_Store>>(text);
        if (s != nullptr)
        {
            if (s->Entries == nullptr) s->Entries = cs::Dictionary<cs::String, cs::Ref<LibraryRecord>>::New();
            // a record written as null (by hand, or cut short) is no record: dropped
            auto nulls = cs::List<cs::KeyValuePair<cs::String, cs::Ref<LibraryRecord>>>::From(s->Entries)
                             .Where([&](auto& kv) { return kv.Value == nullptr; })
                             .Select([&](auto& kv) { return kv.Key; })
                             .ToList();
            for (auto& k : nulls) s->Entries.Remove(k);
            return s;
        }
    }
    catch (const cs::JsonException&) { }
    cs::String aside = p + ".bad-" + cs::DateTime::Now().ToString("yyyyMMdd-HHmmss");
    try
    {
        cs::File::Move(p, aside);
        if (Note) Note(p + " is not a Library file; it was moved aside to " + aside + " and the Library starts empty");
        return [&] { auto _o = cs::New<Librarian_Store>(); _o->MovedAside = aside; return _o; }();
    }
    catch (const std::exception& ex)
    {
        if (Note) Note(p + " is not a Library file and could not be moved aside (" + cs::String(ex.what()) + "); it is not written over");
        return [&] { auto _o = cs::New<Librarian_Store>(); _o->ReadOnly = true; return _o; }();
    }
}

void Librarian_Store::Save(cs::String savesDir)
{
    if (ReadOnly) return;
    cs::String p = PathFor(savesDir);
    try
    {
        cs::Directory::CreateDirectory(savesDir);
        cs::String temp = p + ".tmp";
        cs::File::WriteAllText(temp, cs::Json::Serialize(this, cs::Json::Indented()));
        cs::File::Move(temp, p, true);
    }
    catch (const std::exception& ex) { if (Note) Note(p + " could not be written (" + cs::String(ex.what()) + ")"); }
}

// ----------------------------------------------------------------

void Librarian::FileItem(cs::Ref<Librarian_Store> store, cs::Ref<Character> c, int32_t itemId, IdLevel known)
{
    int16_t grade = known == IdLevel::All ? (int16_t)4 : known == IdLevel::Mostly ? (int16_t)3 : (int16_t)0;
    auto rec = store->EditItem(itemId);
    if (rec->Known == 0) rec->Name = c->Name;
    if (rec->Known < grade) rec->Known = grade;
    rec->Id = cs::Wrap<int16_t>(itemId);
}

void Librarian::FileItem(cs::String savesDir, cs::Ref<Character> c, int32_t itemId, IdLevel known)
{
    auto store = Librarian_Store::Load(savesDir);
    FileItem(store, c, itemId, known);
    store->Save(savesDir);
}

void Librarian::FileFind(cs::Ref<Librarian_Store> store, cs::Ref<Character> c, int32_t itemId, IdLevel known, int16_t fromMonsterId,
                         int32_t today, bool lostLevel, bool lostPosition, int32_t level)
{
    FileItem(store, c, itemId, known);
    auto rec = store->EditItem(itemId);
    rec->MonId = fromMonsterId;
    rec->LastSeen = today;
    rec->Location = !lostLevel && !lostPosition ? cs::ToStr(level) : cs::String("?");
    rec->NSeen++;
}

void Librarian::FileRestock(cs::Ref<Librarian_Store> store, int32_t itemId, int32_t today)
{
    auto rec = store->EditItem(itemId);
    if (rec->Known == 0) rec->Name = "";
    if (rec->Known < Everything) rec->Known = Everything;
    rec->MonId = -2;
    rec->LastSeen = today;
    rec->Location = "?";
    rec->NSeen++;
    rec->Id = cs::Wrap<int16_t>(itemId);
}

int16_t Librarian::ChestFindFrom(cs::Ref<Monster> lead, int32_t leadIndex, int32_t foes0KnownAs, cs::Ref<Character> taker)
{
    return leadIndex == foes0KnownAs && !(MonsterProperty::Has(lead, MonsterProperty::Invisible) && !taker->SeeingInvisible)
        ? lead->Id : (int16_t)-2;
}

cs::String Librarian::FinderName(cs::List<cs::Ref<Monster>> table, int32_t monsterId)
{
    if (table.Count() == 0) return "";
    for (auto& m : table) if (m->Id == monsterId) return m->Name;
    return table[0]->Name;
}

void Librarian::FileCompanion(cs::Ref<Librarian_Store> store, cs::Ref<Character> c, cs::Ref<Monster> def, int32_t idLevel, cs::String place)
{
    if (def->Ref < 0) return;
    auto rec = store->EditMonster(def->Ref);
    int16_t grade = idLevel <= 0 ? (int16_t)4 : idLevel <= 1 ? (int16_t)3 : idLevel <= 2 ? (int16_t)2 : (int16_t)1;
    if (rec->Known == 0) rec->Name = c->Name;
    if (rec->Known < grade) rec->Known = grade;
    if (!place.empty()) rec->Location = place;
    rec->Id = def->Id;
}

void Librarian::FileCompanion(cs::String savesDir, cs::List<cs::Ref<Monster>> table, cs::Ref<Character> c, cs::Ref<Monster> def, int32_t idLevel, cs::String place)
{
    auto store = Librarian_Store::Load(savesDir, table);
    FileCompanion(store, c, def, idLevel, place);
    store->Save(savesDir);
}

cs::String Librarian::PlaceOf(bool lostLevel, bool lostPosition, int32_t level, int32_t x, int32_t y)
{
    return !lostLevel && !lostPosition && level > 0 ? cs::ToStr(x + 1) + "," + cs::ToStr(y + 1) + "," + cs::ToStr(level) : cs::String();
}

void Librarian::FileSighting(cs::Ref<Librarian_Store> store, cs::List<cs::Ref<Monster>> all, int32_t refIndex, int32_t knownAs, int32_t groupCount,
                             bool killed, bool sighting, cs::Ref<Character> reader, cs::String place, int32_t leadLevel, int32_t today,
                             cs::Func<cs::Ref<Item>(int16_t)> item)
{
    bool lost = false;
    if (refIndex < 0) { refIndex = cs::Math::Abs(refIndex); lost = true; }
    else if (refIndex != knownAs) return;
    if (refIndex >= all.Count()) return;
    auto def = all[refIndex];
    if (MonsterProperty::Has(def, MonsterProperty::Invisible) && !reader->SeeingInvisible && reader->DungeonLevel > 0)
        return;
    int32_t roll = CombatEngine::IdLevelRoll(reader, def, item, false);
    int16_t grade = roll <= 0 ? (int16_t)4 : roll <= 1 ? (int16_t)3 : roll <= 2 ? (int16_t)2 : (int16_t)1;
    auto rec = store->EditMonster(CompanionRef::IndexOf(all, def));
    if (rec->Known == 0) rec->Name = lost ? cs::String("") : reader->Name;
    if (rec->Known < grade) rec->Known = grade;
    if (!place.empty()) rec->Location = place;
    rec->LastSeen = today;
    if (sighting)
    {
        if (lost) rec->Location = "?";
        else if (leadLevel == 0 && (rec->Location == "" || rec->LastSeen == 0)) rec->Location = "the City";
    }
    if (killed) rec->NDeaths = cs::Wrap<int16_t>((int32_t)rec->NDeaths + 1);
    if (sighting) rec->NSeen += groupCount;
    rec->Id = def->Id;
}

}  // namespace mc
