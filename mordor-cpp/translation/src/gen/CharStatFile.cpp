#include "core.h"

namespace mc {

bool CharStatFile_Piece::OnScreen() { return Row != -1 && Col != -1; }

cs::String CharStatFile::Write(cs::Ref<Character> c, cs::String gameDir, cs::Func<cs::Ref<Item>(int16_t)> item) {
    return Write(c, PortFiles::CharStatPlaces(gameDir), item);
}

cs::String CharStatFile::Write(cs::Ref<Character> c, cs::List<cs::String> places, cs::Func<cs::Ref<Item>(int16_t)> item) {
    try { return PortFiles::Append(places, Compose(Paint(c, item, true, cs::DateTime::Now()))); }
    catch (...) { return cs::String(); }
}

cs::String CharStatFile::Compose(cs::List<CharStatFile_Piece> pieces) {
    auto s = cs::New<cs::StringBuilder>();
    for (auto& p : pieces) s->Append(p.Bold ? "" : "\t").Append(p.Text).Append("\r\n");
    return s->ToString();
}

cs::String CharStatFile::Str(int64_t n) { return n < 0 ? cs::ToStr(n) : " " + cs::ToStr(n); }

cs::String CharStatFile::FormatNumber(int64_t n) { return n == 0 ? cs::String("0") : cs::Format(n, "###,###,###,###,###"); }

cs::String CharStatFile::FormatNumber(VbCurrency v) {
    if (v.IsWhole()) return FormatNumber(v.Whole());
    cs::Decimal m = cs::Decimal::Round(cs::Math::Abs(v.ToDecimal()), 0, cs::MidpointRounding::AwayFromZero);
    cs::String s = cs::String(v < VbCurrency::Zero ? "-" : "") + (m == cs::Decimal(0) ? cs::String("") : cs::Format(m, "###,###,###,###,###"));
    return (int32_t)s.size() == 0 ? cs::String("0") : s;
}

cs::String CharStatFile::CStrSingle(float v) { return cs::Format(v, "G7"); }

cs::List<CharStatFile_Piece> CharStatFile::Paint(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item, bool toFile, cs::DateTime now) {
    auto p = cs::List<CharStatFile_Piece>::New();
    auto Put = [&](int32_t row, int32_t col, cs::String text, bool bold) { p.Add(CharStatFile_Piece{row, col, text, bold, false}); };

    if (toFile) {
        Put(-1, -1, "  VERSION: " + Version, true);
        Put(-1, -1, "CHARACTER: \"" + cs::Str::TrimEnd(c->Name) + "\" (" + Librarian::Alignments[static_cast<int32_t>(c->Alignment)] + " " + c->Race + ")", true);
        Put(-1, -1, "", true);
        Put(-1, -1, "ITEM INFO", true);
        for (int32_t i = 0; i < Character::PackSize; i++) {
            auto slot = c->Pack()[i];
            if (slot == nullptr || slot->ItemId <= 0) continue;
            auto it = item(slot->ItemId);
            if (!it) continue;
            Put(-1, -1, cs::String(slot->Equipped ? "* " : "  ") + Str(i) + ". " + Loot::NameFor(it, slot->Known(), slot->Alignment, slot->Charges), false);
        }
        Put(-1, -1, "", true);
        Put(-1, -1, "BANK ITEM INFO", true);
        for (int32_t i = 0; i < c->Bank.Count(); i++) {
            auto slot = c->Bank[i];
            if (slot->ItemId <= 0) continue;
            auto it = item(slot->ItemId);
            if (!it) continue;
            Put(-1, -1, Str(i) + ". " + Loot::NameFor(it, slot->Known(), slot->Alignment, slot->Charges), false);
        }
        Put(-1, -1, "", true);
    }

    Put(2, 1, "Kills:", true);
    Put(2, 6, FormatNumber((int64_t)c->Kills), false);
    Put(3, 1, "Died:", true);
    Put(3, 6, Str(c->Deaths), false);
    Put(4, 1, "Comp:", true);
    Put(4, 6, Str(c->Complications), false);
    Put(7, 1, "Mon Quests:", true);
    Put(7, 8, Str(c->MonsterQuestsDone), false);
    Put(8, 1, "Item Quests:", true);
    Put(8, 8, Str(c->ItemQuestsDone), false);

    // mdrcharSetAD mode 0 strips the stats from Att/Def; best guild's Att + 10 (mc0782) is the guild part
    auto gad = Rules::BestGuildAD(c);
    auto ga = gad.att;
    auto gd = gad.def;
    int32_t guildAtt = Rules::BaseAD + ga, guildDef = Rules::BaseAD + gd;
    cs::List<int32_t> withMods = cs::Enumerable::Range(0, 6).Select([&](auto& i) { return c->StatWithMods(i, item); }).ToArray();
    int32_t statsAD = Rules::StatsAD(withMods);
    int32_t itemAtt = CombatEngine::WornSum(c, item, [&](cs::Ref<Item> i) { return (int32_t)i->Att; }),
            itemDef = CombatEngine::WornSum(c, item, [&](cs::Ref<Item> i) { return (int32_t)i->Def; });
    // stored Att/Def carry ADShift (MONSMOD.BAS:3884-3906); the report's arithmetic, kept
    int32_t held = -c->StoredADShift(item);
    int32_t att = guildAtt + statsAD - held + itemAtt, def = guildDef + statsAD - held + itemDef;
    itemAtt = att - statsAD - guildAtt; itemDef = def - statsAD - guildDef;
    Put(12, 1, " Item A/D:", true);
    Put(12, 8, Str(itemAtt) + "/" + cs::ToStr(itemDef), false);
    Put(10, 1, "Guild A/D:", true);
    Put(10, 8, Str(guildAtt) + "/" + cs::ToStr(guildDef), false);
    Put(11, 1, "Stats A/D:", true);
    Put(11, 8, Str(att - itemAtt - guildAtt) + "/" + cs::ToStr(def - itemDef - guildDef), false);
    Put(-1, -1, "Total A/D:", true);
    Put(-1, -1, Str(att) + "/" + Str(def), false);

    // DateTime.FromOADate (CVDate)
    auto FromOADate = [&](double d) {
        if (d >= 2958466.0 || d <= -657435.0) throw cs::ArgumentException("Not a legal OleAut date.");
        int64_t millis = cs::CastDouble<int64_t>(d * 86400000.0 + (d >= 0 ? 0.5 : -0.5));
        if (millis < 0) millis -= (millis % 86400000LL) * 2;
        millis += 59926435200000LL;
        cs::DateTime dt;
        dt.ticks = millis * 10000LL;
        return dt;
    };

    Put(14, 1, "Time:", true);
    Put(14, 8, now.ToString("HH:mm:ss"), false);   // Time$
    Put(15, 1, "Date:", true);
    Put(15, 8, now.ToString("d"), false);          // Date, the short date
    Put(16, 1, "Play Time:", true);
    Put(16, 8, cs::Format(c->PlaySeconds / 60 / 60, "###,###.00") + " Hrs", false);
    Put(17, 1, "Created on:", true);
    Put(17, 8, FromOADate(c->CreatedOn).ToString("d"), false);   // CVDate
    Put(-1, -1, "", true);
    Put(-1, -1, "", true);

    Put(2, 13, "Stats Nat(Mod)", true);
    auto max = c->RaceDef()->Max;
    for (int32_t i = 0; i < 6; i++) {
        int32_t mods = withMods[i] - c->Stats[i];
        // bold when the natural stat is at least the race maximum (M553C) plus five
        Put(3 + i, 14, cs::Str::Substring(RaceData::StatNamesLong[i], 0, 3) + " " + Str(c->Stats[i]) + " (" + cs::ToStr(mods) + ")",
            c->Stats[i] >= max[i] + 5);
    }

    Put(-1, -1, "", true);
    Put(-1, -1, "GUILD INFO", true);
    for (auto& g : GuildData::All) {
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        auto fad = Rules::GuildADSingle(g->Name, m->Level);
        auto fa = fad.att;
        auto fd = fad.def;
        Put(-1, -1, g->Name + " (" + cs::ToStr(m->Level) + ") " + CStrSingle(fa) + "/" + CStrSingle(fd), false);
    }
    Put(-1, -1, "", true);
    Put(-1, -1, "Abilities (Lead Guild)", true);
    for (int32_t a = 0; a < Rules::AbilityNames.Length(); a++) {
        auto lead = Rules::AbilityLead(c, a, item);
        if (lead.Modifier > 0) Put(-1, -1, Rules::AbilityNames[a] + " (" + lead.Guild + ")", false);
    }
    Put(-1, -1, "", true);

    Put(10, 13, "Total Exp.", true);
    Put(11, 14, FormatNumber((int64_t)c->Guilds.Sum([&](auto& g) { return g->Experience; })), false);
    Put(13, 13, "Total Gold", true);
    Put(14, 14, FormatNumber(c->Purse()), false);
    // the footer, If p06E4 = True, is never reached
    return p;
}

}  // namespace mc
