#include "core.h"

namespace mc {

cs::List<CharStatFile_Piece> GuildTabPage::Paint(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item, cs::List<cs::Ref<Spell>> spells) {
    auto p = cs::List<CharStatFile_Piece>::New();
    auto Put = [&](int32_t row, int32_t col, cs::String text, bool bold, bool italic) {
        p.Add([&] {
            CharStatFile_Piece _o{};
            _o.Row = row;
            _o.Col = col;
            _o.Text = text;
            _o.Bold = bold;
            _o.Italic = italic;
            return _o;
        }());
    };
    auto N = [&](int64_t v) -> cs::String { return cs::ToStr(v); };

    int32_t n = 0;
    Put(2, 1, "Guilds", true, false);
    for (auto& g : GuildData::All) {
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        n++;
        Put(n + 2, 2, g->Name + " (" + N(m->Level) + ")", false, false);
    }

    // gv141C.M5493 as the page's calls leave it; the first call (ability 4) always writes it
    cs::String lead = GuildData::All.Count() > 0 ? GuildData::All[0]->Name : cs::String("");
    auto Chance = [&](int32_t ability) {
        auto won = Rules::AbilityLead(c, ability, item);
        if (won.Guild != "") lead = won.Guild;
    };

    Put(n + 4, 1, "Total Skill", true, false);
    int32_t fighting = HallRecords::FightingSkill(c, item);
    for (int32_t a : cs::List<int32_t>::Of({4, 2, 1, 3})) Chance(a);
    Put(n + 5, 2, N(fighting) + " Fighting", false, false);
    Put(n + 6, 2, N(HallRecords::MagicSkill(c, spells)) + " Magical", false, false);
    int32_t thieving = HallRecords::ThievingSkill(c, item);
    Chance(0);
    Put(n + 7, 2, N(thieving) + " Thieving", false, false);
    Put(n + 9, 1, "Current Guild", true, false);
    Put(n + 10, 2, cs::deref(c->Guild()).Guild, false, false);

    int32_t row = 0;
    Put(2, 11, "Guild Abilities", true, false);
    for (int32_t a = 0; a < Rules::AbilityNames.Length(); a++) {
        int32_t rating = Rules::AbilityRatingExact(c, a, item);
        Chance(a);
        if (rating <= 0) continue;
        row += 3;
        Put(row, 12, Rules::AbilityNames[a], true, false);
        Put(row + 2, 13, N(rating) + " from " + lead, false, true);
    }
    return p;
}

}  // namespace mc
