#include "core.h"

namespace mc {

cs::String SpellInfo::Special(cs::Ref<Spell> s) {
    return SpecialFor(s, s->Effect >= MassOffset ? s->Effect - MassOffset : s->Effect);
}

// a C# null result is returned as the empty string
cs::String SpellInfo::SpecialFor(cs::Ref<Spell> s, int32_t e) {
    if (e == 0 || (e >= OnlyAffectsBase && e <= 99)) {
        if (s->Category == HealClass) return "Heals wounds";
        if (s->Category == CharmClass) return "Attempts to Bind/Charm Target(s)";
        return s->Effect == 100 ? "Kills target(s) or has no effect" : "Damages or Kills Target(s)";
    }
    if (e == 1) return "Removes Poison";
    if (e == 2) return "Removes Disease";
    if (e == 3) return "Levitates a Character";
    if (e == 4) return "Turns character Invisible";
    if (e == 5) return "Allows character to see Invisible";
    if (e == 6) return "Protects a Character";
    if (e == 7) return "Teleports a Character";
    if (e == 8) return "Displaces a Character";
    if (e == 9) return "Opens a door in Wall";
    if (e == 10) return "Removes Paralysis";
    if (e == 11) return "Detects Rock";
    if (e == 12) return "Detects Depth";
    if (e == 13) return "Determines Location";
    if (e == 14) return "Locates a Character";
    if (e == 15) return "Pulls Character from Rock";
    if (e == 16) return "Sets Sanctuary Coords";
    if (e == 17) return "Teleports to Sanctuary Coords";
    if (e == 18) return "Removes Poison and Disease";
    if (e >= 20 && e <= 25) return "Raises the Dead";
    if (e == 30) return "Unlocks Chest";
    if (e == 31) return "Unlocks Door";
    if (e == 32) return "Unlocks any locked object";
    if (e >= ResistBase && e < ResistBase + 11) {
        int32_t which = s->Effect - ResistBase;
        return which >= 0 && which < Librarian::Resistances.Length() ? "Resists " + Librarian::Resistances[which] : cs::String();
    }
    if (e == 100) return "Kills target(s) or has no effect";
    if (e >= 40 && e <= 42) return "Increases the Bind Level of a Companion";
    if (e == 150) return "Binds a companion to the caster for life";
    return "Unknown!";
}

cs::List<CharStatFile_Piece> SpellInfo::Paint(cs::Ref<Spell> s, cs::Ref<Character> c) {
    auto p = cs::List<CharStatFile_Piece>::New();
    auto Put = [&](int32_t row, int32_t col, cs::String text, bool bold = false, bool italic = false) {
        p.Add([&] { CharStatFile_Piece _o{}; _o.Row = row; _o.Col = col; _o.Text = text; _o.Bold = bold; _o.Italic = italic; return _o; }());
    };
    auto N = [&](int64_t v) -> cs::String { return cs::ToStr(v); };  // CStr

    Put(2, 1, "Spell:", true);
    Put(2, 9, "\"" + cs::Str::TrimEnd(s->Name) + "\"");
    Put(3, 1, "Class:", true);
    // gv07B0$ is ReDim'd 0 To 18 with eighteen names, so slot 18 is blank and past it the page stops
    if (s->Category < 0 || s->Category > Librarian::SpellSchools.Length()) return p;
    Put(3, 9, s->Category == Librarian::SpellSchools.Length() ? cs::String("") : Librarian::SpellSchools[s->Category]);
    // "15/5 ... <base>/<levelmodifier>" (help): M5B33 and M5B41 + 1
    if (s->Damage2 > 0) {
        Put(4, 1, s->Category == HealClass ? "Heal:" : "Damage:", true);
        Put(4, 9, N(s->Damage2) + "/" + N(s->SpecialEffect + 1));
    }
    Put(5, 1, "Guild:", true);
    Put(5, 9, GuildData::All.Count() > 0 ? GuildData::All[Spellbook::TeachingGuild(c, s)]->Name : cs::String(""));
    Put(6, 1, "Points:", true);
    Put(6, 9, N(Spellbook::Cost(s, c)));
    if (s->ResistIndex > 0) {
        Put(8, 1, "Resistance:", true);  // eleven letters from column 1, so it runs into column 9
        if (s->ResistIndex >= Librarian::Resistances.Length()) return p;  // Subscript out of range: the page stops
        Put(8, 9, Librarian::Resistances[s->ResistIndex]);
    }
    Put(4, 14, "Stats Required", true);
    int32_t n = 0;
    for (int32_t i = 0; i < 6 && i < s->Required.Length(); i++)
        if (s->Required[i] > 0) Put(5 + n++, 16, cs::Str::Substring(RaceData::StatNamesLong[i], 0, 3) + " (" + N(s->Required[i]) + ")");

    int32_t e = s->Effect;
    if (e >= MassOffset) {
        Put(12, 1, "This spell affects the entire party", false, true);
        e -= MassOffset;
    }
    cs::String special = SpecialFor(s, e);
    if (special.empty()) return p;  // "Resists" indexed past the list: the error ends the page here
    Put(10, 1, "Special", true);
    Put(11, 1, special);
    // "Only Affects Xs" with the raw effect, so a mass one would index past the classes
    if (e >= OnlyAffectsBase && e <= 99) {
        // MonsterClasses is ReDim'd 0 To NumMonsterClasses (MODULE1.BAS:709): one blank record on the end
        int32_t cls = s->Effect - OnlyAffectsBase, classes = Mdata1::MonsterClasses.Count();
        if (cls > classes) return p;
        Put(12, 1, "Only Affects " + (cls == classes ? cs::String("") : Mdata1::MonsterClassName(cls)) + "s");
    }
    if (s->Damage2 > 0 && s->Category != HealClass) {
        Put(14, 1, "This spell will affect " + N(s->Damage1) + " groups,");
        Put(15, 1, N(s->AffectGroup) + " monsters max in each group.");
    }
    Put(17, 1, "This spell is currently cast at a spell");
    int32_t level = Spellbook::CasterSpellLevel(c, s);
    cs::String trained = Spellbook::TrainingGuild < GuildData::All.Count() ? GuildData::All[Spellbook::TrainingGuild]->Name : cs::String("");
    Put(18, 1, "level of " + N(level) + " by your " + trained + " training.");
    return p;
}

}  // namespace mc
