#include "core.h"

namespace mc {

bool ItemInfoPage::IsItemPage(cs::List<CharStatFile_Piece> pieces) {
    return pieces.Any([&](auto& p) { return p.Row == 2 && p.Col == 1 && p.Bold && p.Text == "Item"; });
}

cs::String ItemInfoPage::VbSingle(float v) {
    // .NET "G7": 7 significant digits, scientific when exponent < -5+1 or >= 7 (same rule as C's %G)
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.7G", (double)v);
    cs::String s(buf);
    if (cs::Str::StartsWith(s, cs::String("0."))) return cs::Str::Substring(s, 1);
    if (cs::Str::StartsWith(s, cs::String("-0."))) return "-" + cs::Str::Substring(s, 2);
    return s;
}

cs::List<CharStatFile_Piece> ItemInfoPage::Paint(cs::Ref<Item> it, ItemAlignment alignment, IdLevel known, cs::List<cs::Ref<Spell>> spells) {
    auto p = cs::List<CharStatFile_Piece>::New();
    auto Put = [&](int32_t row, int32_t col, cs::String text, bool bold = false, bool italic = false) {
        p.Add([&] { CharStatFile_Piece _o{}; _o.Row = row; _o.Col = col; _o.Text = text; _o.Bold = bold; _o.Italic = italic; return _o; }());
    };
    auto Str = [&](int64_t n) -> cs::String { return n < 0 ? cs::ToStr(n) : " " + cs::ToStr(n); };
    auto N = [&](int64_t n) -> cs::String { return cs::ToStr(n); };
    bool all = known == IdLevel::All, mostly = known >= IdLevel::Mostly, partial = known >= IdLevel::Partial;

    Put(2, 1, "Item", true);
    cs::String name;
    if (mostly) {
        name = cs::Str::TrimEnd(it->Name);
        if (static_cast<int32_t>(alignment) > 0 && all) name += " " + AlignmentTags[cs::Math::Clamp<int32_t>(static_cast<int32_t>(alignment), 0, 3)];
        if (all && it->Cursed != 0) name += " (cursed)";
    } else
        name = "<Unknown>";
    Put(2, 6, name);

    if (mostly) {
        cs::String ad = N(it->Att) + "/" + N(it->Def);
        int32_t typeIndex = Mdata1::Loaded && it->Type >= 0 && it->Type < Mdata1::ItemTypes.Count() ? Mdata1::ItemTypes[it->Type].Index : 0;
        if (it->DamageMod != 0 && all && typeIndex == 0)
            ad += " [" + VbSingle(it->DamageMod) + "]";
        Put(3, 1, "Att/Def", true);
        Put(3, 6, ad);
    }

    Put(4, 1, "Class", true);
    cs::String cls = partial ? Mdata1::ItemTypeName(it->Type) : Mdata1::ItemDescriptionName(it->Type);
    if (all) cls += it->ClassRestricted == -1 ? " (Class Restricted)" : " (Not Class Restricted)";
    Put(4, 6, cls);

    if (mostly) {
        Put(ReqRow, 1, "Req", true);
        for (int32_t i = 0; i < 6; i++) {
            Put(StatNameRow, i * 2 + 3, cs::Str::Substring(RaceData::StatNamesLong[i], 0, 3), true);
            Put(ReqRow, i * 2 + 4, N(i < it->StatsRequired.Length() ? it->StatsRequired[i] : 0));
        }
    }
    if (all) {
        Put(ModRow, 1, "Mod", true);
        for (int32_t i = 0; i < 6; i++) Put(ModRow, i * 2 + 4, N(i < it->StatsMod.Length() ? it->StatsMod[i] : 0));
    }

    cs::String l075A = "";  // a plain Dim, "" on each call, shared by rows 10 and 11
    if (all) {
        auto st = it->SpecialType;
        if (st >= 1 && st <= 10) l075A = "Modifies Stat(s)";
        else if (st == 11) l075A = "Adds 50 Spell Points";
        else if (st == 12) l075A = "Adds 200 Spell Points";
        else if (st == 13 || st == 14) l075A = "Subtracts Age";
        else l075A = l075A;
        Put(10, 1, l075A);
    }
    if (mostly) {
        if (it->NHands > 0) {
            if (it->Swings == 1) l075A = ", 1 swing";
            else if (it->Swings > 1) l075A = ", " + N(it->Swings) + " swings";
            Put(11, 1, HandWords[cs::Math::Clamp<int32_t>((int32_t)it->NHands, 0, HandWords.Length() - 1)] + "-handed Item" + l075A);
        } else
            Put(11, 1, "Requires no hands");
    }
    if (all && it->SpellIndex != 0) {
        int32_t s = it->SpellIndex - 1;
        cs::String spell = s >= 0 && s < spells.Count() ? cs::Str::TrimEnd(spells[s]->Name) : cs::String("");
        Put(12, 1, "Casts \"" + spell + "\"");
        Put(13, 1, it->SpellLvl > 0 ? "at a Spell Level of " + N(it->SpellLvl) : cs::String("at the casters Spell Level"));
    }

    if (mostly) {
        int32_t n = 0;
        Put(10, 13, "Allowed Guilds", true);
        for (int32_t g = 0; g < GuildData::All.Count(); g++) {
            if ((it->Guilds & (1 << g)) == 0) continue;
            n++;
            Put(10 + n, 13, GuildData::All[g]->Name);
            Put(10 + n, 19, all ? Str(Character::UseLevel(it, GuildData::All[g])) : cs::String("?"));
        }
    }
    if (all) {
        int32_t n = 0;
        for (int32_t a = 0; a < Librarian::ItemAbilities.Length() && a < 6; a++)
            if ((it->Abilities & (1 << a)) != 0) Put(13 + ++n, 2, Librarian::ItemAbilities[a], false, true);
        for (int32_t r = 0; r < Effects::ResistanceKinds; r++)
            if ((it->ResistanceFlags & (1 << r)) != 0) Put(13 + ++n, 2, Librarian::Resistances[r] + " Res", false, true);
    }

    Put(20, 1, "ID Lev:", true);
    Put(21, 1, Loot::IdLevelName(known));
    if (it->SpecialType >= 500 && it->SpecialType <= 500 + GuildData::All.Count() - 1)
        Put(23, 1, "This is a Guild-Issued Item", true);
    return p;
}

}  // namespace mc
