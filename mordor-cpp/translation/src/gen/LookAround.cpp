#include "core.h"

namespace mc {

std::tuple<cs::List<LookAround_Entry>, int32_t, int32_t> LookAround::Walk(cs::Ref<Character> leader, cs::List<cs::Ref<Character>> roster,
    cs::Func<std::tuple<int32_t, int32_t>(cs::Ref<Character>)> placeOf, bool everywhere,
    cs::Func<cs::Nullable<LookAround_Listed>(cs::Ref<Character>)> listed) {
    auto here = cs::List<LookAround_Entry>::New();
    int32_t living = 0, dead = 0;
    auto [leaderLevel, leaderArea] = placeOf(leader);
    for (auto& r : roster) {
        bool isLeader = cs::Str::EqualsIgnoreCase(cs::Str::Trim(r->Name), cs::Str::Trim(leader->Name));
        auto [level, area] = placeOf(r);
        if (!everywhere && (level != leaderLevel || (area != leaderArea && !isLeader))) continue;
        cs::Nullable<LookAround_Listed> l = listed ? listed(r) : cs::Nullable<LookAround_Listed>{};
        bool countedDead = (l.has_value() ? l.value().Dead : r->IsDead) && !isLeader;
        if (countedDead) dead++; else living++;
        here.Add([&] {
            LookAround_Entry _o{};
            _o.Name = cs::Str::TrimEnd(r->Name);
            _o.Guild = r->Guild()->Guild;
            _o.Level = r->Guild()->Level;
            _o.Hits = l.has_value() ? l.value().Hits : r->CurHits;
            _o.CountedDead = countedDead;
            return _o;
        }());
    }
    return std::make_tuple(here, living, dead);
}

void LookAround_ListedParty::Sync(cs::List<cs::Ref<Character>> party) {
    if (party.Count() <= 1) return;
    for (auto& m : party) _rows[m] = [&] { LookAround_Listed _o{}; _o.Hits = m->CurHits; _o.Dead = m->IsDead; return _o; }();
}

cs::Nullable<LookAround_Listed> LookAround_ListedParty::For(cs::Ref<Character> c, cs::Ref<Character> shown) {
    LookAround_Listed l{};
    if (!_rows.TryGetValue(c, l)) return {};
    if (c == shown) {
        LookAround_Listed w = l;
        w.Hits = c->CurHits;
        return w;
    }
    return l;
}

cs::List<cs::Ref<Character>> LookAround::Roster(cs::List<cs::Ref<Character>> party, cs::List<cs::Ref<Character>> saved) {
    auto Live = [&](cs::Ref<Character> s) -> cs::Ref<Character> {
        return party.FirstOrDefault([&](auto& m) { return cs::Str::EqualsIgnoreCase(cs::Str::Trim(m->Name), cs::Str::Trim(s->Name)); });
    };
    auto list = saved.Select([&](auto& s) { auto lv = Live(s); return lv ? lv : s; }).ToList();
    for (auto& m : party) if (!list.Contains(m)) list.Add(m);
    return list;
}

cs::List<CharStatFile_Piece> LookAround::Paint(cs::List<LookAround_Entry> here) {
    auto piece = [](int32_t row, int32_t col, cs::String text, bool bold) {
        CharStatFile_Piece _o{};
        _o.Row = row;
        _o.Col = col;
        _o.Text = text;
        _o.Bold = bold;
        return _o;
    };
    auto p = cs::List<CharStatFile_Piece>::Of({
        piece(2, 1, "Name", true), piece(2, 9, "Guild", true), piece(2, 15, "Level", true), piece(2, 19, "Hits", true),
    });
    for (int32_t i = 0; i < here.Count(); i++) {
        auto e = here[i];
        int32_t row = i + 3;
        p.Add(piece(row, 1, e.Name, false));
        p.Add(piece(row, 9, e.Guild, false));
        p.Add(piece(row, 15, cs::ToStr(e.Level), false));
        p.Add(piece(row, 19, cs::ToStr(e.Hits), false));
    }
    return p;
}

cs::String LookAround::Readout(cs::Ref<Character> shown, cs::Ref<Character> leader, int32_t x, int32_t y, int32_t level, Facing facing, int32_t living, int32_t dead) {
    if (shown->IsDead) return "(You are Dead)";
    cs::String s = !leader->LostPosition ? cs::ToStr(x) + "," + cs::ToStr(y) + "," + cs::ToStr(level) + " "
                 : !leader->LostLevel ? "(Lost on " + cs::ToStr(level) + ") "
                 : cs::String("(Lost) ");
    s += leader->LostFacing ? cs::String("?") : ToStr(facing);
    return s + (dead > 0 ? " [ " + cs::ToStr(living) + "+" + cs::ToStr(dead) + " ]" : " [ " + cs::ToStr(living) + " ]");
}

}  // namespace mc
