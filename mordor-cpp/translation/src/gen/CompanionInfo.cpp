#include "core.h"

namespace mc {

cs::String CompanionInfo::IdLevelText(int32_t idLevel) {
    if (idLevel == KnowAll) return "You know All";
    if (idLevel == MoreToLearn) return "More to learn";
    if (idLevel == LotsMoreToLearn) return "Lots more to learn";
    return "You know Nothing";
}

cs::List<CharStatFile_Piece> CompanionInfo::Paint(cs::Ref<Companion> c, int32_t index, cs::Ref<Monster> def, cs::Ref<Character> owner,
                                                  cs::Func<cs::Ref<Item>(int16_t)> item, Alignment storeAlignment) {
    auto p = cs::List<CharStatFile_Piece>::New();
    auto Put = [&](int32_t row, int32_t col, cs::String text, bool bold, bool italic) {
        p.Add(CharStatFile_Piece{row, col, text, bold, italic});
    };
    auto N = [&](int64_t v) -> cs::String { return cs::ToStr(v); };
    bool owned = index > -1 && c != nullptr;
    int32_t id = owned ? c->IdLevel : KnowAll;

    cs::String head = owned
        ? "Comp. #" + N(index + 1) + " " + (cs::Str::Trim(c->Name) != "" ? "\"" + cs::Str::TrimEnd(c->Name) + "\" " : cs::String("")) + "(" + def->Name + ")"
        : "STORE: " + def->Name;   // Monsters().Name, not RTrim'd
    Put(2, 1, head, true, false);
    Put(4, 1, "Size:", true, false);
    Put(4, 8, Librarian::Sizes.ElementAtOrDefault((int32_t)def->Size), false, false);
    if (id <= LotsMoreToLearn) {
        Put(5, 1, "Type:", true, false);
        Put(5, 8, Mdata1::MonsterGroupName((int32_t)def->Type, ""), false, false);
    }
    if (id <= MoreToLearn) {
        int32_t align = owned ? AlignmentOf(c, def) : static_cast<int32_t>(storeAlignment);
        Put(6, 1, "Align:", true, false);
        Put(6, 8, Librarian::Alignments.ElementAtOrDefault(align), false, false);
        Put(7, 1, "A/D:", true, false);
        Put(7, 8, N(def->Att) + "/" + N(def->Def), false, false);
        Put(8, 1, "Avg. Hits:", true, false);
        Put(8, 8, N(def->Hits), false, false);
    }
    if (owned) {
        Put(10, 1, c->BindingLevel < 0 ? "Content Level:" : "Bind Level:", true, false);
        Put(11, 1, N(cs::Math::Abs(c->BindingLevel)) + " (" + Binding::Describe(c->BindingLevel, LeaveChance(c, def, owner, item)) + ")", false, false);
    }
    if (id <= MoreToLearn) {
        Put(13, 1, "Stats", true, false);
        Put(14, 1, cs::Str::Join("/", def->Stats.Take(6).Select([&](auto& v) { return N(v); })), false, false);
    }
    if (id == KnowAll) {
        // the heading at row 13, the lines from row 14 at column 9 - over the stats string when that is wide enough, as there
        Put(13, 9, "Special", true, false);
        int32_t n = 0;
        for (int32_t i = 0; i < Librarian::MonsterProperties.Length(); i++)
            if (Bit(def->SpecialPropertyFlags, i)) Put(13 + ++n, 9, Librarian::MonsterProperties[i], false, true);
        for (int32_t i = 0; i < Librarian::MonsterAttacks.Length(); i++)
            if (Bit(def->SpecialAttackFlags, i)) Put(13 + ++n, 9, "Can " + Librarian::MonsterAttacks[i], false, true);
        for (int32_t i = 0; i < Librarian::SpellSchools.Length(); i++)
            if (Bit(def->SpellFlags, i)) Put(13 + ++n, 9, "Can Cast \"" + Librarian::SpellSchools[i] + "\" Spells", false, true);
        for (int32_t i = 0; i < Librarian::Resistances.Length() && i < def->Resistances.Length(); i++)
            if (def->Resistances[i] > 0) Put(13 + ++n, 9, "Is " + Librarian::Resistances[i] + " Resistant", false, true);
        if (n == 0) Put(14, 9, "(None)", false, true);
    }
    Put(16, 1, "ID Lev:", true, false);
    Put(17, 1, IdLevelText(id), false, false);
    return p;
}

void CompanionInfo::Sharpen(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Func<cs::Ref<Item>(int16_t)> item) {
    int32_t now = CombatEngine::CharmIdLevel(owner, def, item);
    if (now < c->IdLevel) c->IdLevel = now;
}

bool CompanionInfo::Bit(int64_t mask, int32_t b) {
    return (mask & (int64_t)(1ULL << (b & 63))) != 0;
}

int32_t CompanionInfo::AlignmentOf(cs::Ref<Companion> c, cs::Ref<Monster> def) {
    switch (c->Alignment) {
        case ItemAlignment::Good: return static_cast<int32_t>(Alignment::Good);
        case ItemAlignment::Neutral: return static_cast<int32_t>(Alignment::Neutral);
        case ItemAlignment::Evil: return static_cast<int32_t>(Alignment::Evil);
        default: return def->AlignmentValue;
    }
}

// Stats + StatMods of Charisma, as companionLeaveChance reads it (MONSMOD.BAS:2061)
double CompanionInfo::LeaveChance(cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Ref<Character> owner, cs::Func<cs::Ref<Item>(int16_t)> item) {
    return Binding::ChanceToLeave(c->BindingLevel, def->GuildLevel, owner->TopGuildLevel(),
                                  owner->StatWithMods(static_cast<int32_t>(Stat::Cha), item), c->Hits, c->MaxHits);
}

}  // namespace mc
