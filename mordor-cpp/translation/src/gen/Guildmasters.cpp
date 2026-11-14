#include "core.h"

namespace mc {

int32_t Guildmasters::Seed(cs::String savesDir) {
    int32_t added = 0;
    for (int32_t g = 0; g < GuildData::All.Count() && g < Seeded.Length(); g++) {
        if (Held.ContainsKey(g) && Held.Get(g)->Name != "") continue;
        Held[g] = [&] { auto _o = cs::New<Holder>(); _o->Name = Seeded[g].Name; _o->Level = Seeded[g].Level; return _o; }();
        GuildLog::Append(savesDir, g, GuildLog_Wrote::AppointedGuildmaster(Seeded[g].Name));
        added++;
    }
    if (added > 0) Save(savesDir);
    return added;
}

cs::Ref<Guildmasters> Guildmasters::LoadSeeded(cs::String savesDir) {
    auto t = Load(savesDir);
    t->Seed(savesDir);
    return t;
}

cs::Ref<Guildmasters_Holder> Guildmasters::For(int32_t guild) {
    cs::Ref<Holder> h{};
    return Held.TryGetValue(guild, h) ? h : cs::New<Holder>();
}

cs::String Guildmasters::Describe(int32_t guild, cs::Ref<Character> viewer) {
    auto h = For(guild);
    if (h->Name == viewer->Name) return " You are the GuildMaster";
    if (h->Name == "") return " There is no GuildMaster";
    return " " + cs::ToStr(h->Name) + " (Level " + cs::ToStr(h->Level) + ")";
}

Tup_qualified_changedHands_6bbff Guildmasters::Claim(cs::Ref<Guildmasters> table, int32_t guild, cs::Ref<Character> c) {
    auto m = c->Guilds.FirstOrDefault([&](auto& g) { return GuildData::IndexOf(g->Guild) == guild; });
    int32_t level = m ? m->Level : 0;
    if (level <= 0) return Tup_qualified_changedHands_6bbff{false, false};

    auto held = table->For(guild);
    if (held->Level >= level && held->Name != "") return Tup_qualified_changedHands_6bbff{false, false};

    bool changedHands = held->Name != c->Name;
    table->Held[guild] = [&] { auto _o = cs::New<Holder>(); _o->Name = c->Name; _o->Level = level; return _o; }();
    return Tup_qualified_changedHands_6bbff{true, changedHands};
}

cs::Ref<Item> Guildmasters::CrestFor(cs::List<cs::Ref<Item>> items, int32_t guild) {
    return items.FirstOrDefault([&](auto& i) { return i->Id == CrestIdBase + guild; });
}

Tup_award_message_f842a Guildmasters::AwardCrest(cs::Ref<Character> c, int32_t guild, cs::List<cs::Ref<Item>> items) {
    auto crest = CrestFor(items, guild);
    if (crest == nullptr) return Tup_award_message_f842a{Award::NotApplicable, ""};
    // Items(Pack(l0170).Ref).Id = l0172 (FRMCTYG.BAS:698-702)
    if (c->Inventory().Any([&](auto& s) { return s->ItemId == crest->Id; })) return Tup_award_message_f842a{Award::AlreadyHeld, ""};
    // M5860(l0170).Ref > 0 and Items(Ref).Id = l0172, then "If l0176 = True Then Exit Sub"
    // (FRMCTYG.BAS:703-721)
    if (c->Bank.Take(Character::BankSlots).Any([&](auto& s) { return s->ItemId > 0 && s->ItemId == crest->Id; }))
        return Tup_award_message_f842a{Award::AlreadyHeld, ""};

    if (c->FirstFreeSlot() < 0)
        return Tup_award_message_f842a{Award::NoRoom, "You were awarded the '" + cs::ToStr(crest->Name) + "', but you had no room on your character for it!"};

    c->PutInPack([&] {
        auto _o = cs::New<InventorySlot>();
        _o->ItemId = crest->Id;
        _o->set_Known(IdLevel::All);
        _o->set_Identified(true);
        _o->Charges = static_cast<int16_t>(crest->Charges);
        // FRMCTYG.BAS:745 copies the item's curse onto the slot; no crest in MDATA3 is cursed
        _o->SlotCursed = crest->Cursed != 0;
        return _o;
    }());
    return Tup_award_message_f842a{Award::Given, cs::String("Congratulations!  For achieving the title of Guildmaster, you have been awarded the ")
                + "'" + cs::ToStr(crest->Name) + "'!\n\nMay the Crest help you with survival, exploration and bring you luck."};
}

cs::String Guildmasters::PathFor(cs::String savesDir) {
    return cs::Path::Combine(savesDir, "guildmasters.json");
}

cs::Ref<Guildmasters> Guildmasters::Load(cs::String savesDir) {
    try
    {
        cs::String p = PathFor(savesDir);
        if (cs::File::Exists(p)) { auto r = cs::Json::Deserialize<cs::Ref<Guildmasters>>(cs::File::ReadAllText(p)); return r ? r : cs::New<Guildmasters>(); }
    }
    catch (...) { }
    return cs::New<Guildmasters>();
}

void Guildmasters::Save(cs::String savesDir) {
    try
    {
        cs::Directory::CreateDirectory(savesDir);
        cs::File::WriteAllText(PathFor(savesDir), cs::Json::Serialize(*this, cs::Json::Indented()));
    }
    catch (...) { }
}

}  // namespace mc
