#include "core.h"

namespace mc {

cs::String Spellbook_SpellRow::GuildText() {
    return (int32_t)GuildData::All[Guild]->Name.size() > 3 ? cs::Str::Substring(GuildData::All[Guild]->Name, 0, 3) : GuildData::All[Guild]->Name;
}

cs::String Spellbook_SpellRow::PointsText() {
    return Points > 999 ? cs::String("***") : cs::ToStr(Points);
}

bool Spellbook_Castable::FromItem() {
    return Slot != nullptr;
}

cs::List<int32_t> Spellbook::SpellClasses(cs::Ref<Character> c) {
    auto kinds = cs::List<int32_t>::New();
    for (int32_t k = 0; k < ClassCount; k++) {
        for (auto& m : c->Guilds) {
            if (m->Level <= 0) continue;
            auto def = GuildData::ByName(m->Guild);
            if (k < def->SpellRate.Length() && def->SpellRate[k] > 0) { kinds.Add(k); break; }
        }
    }
    return kinds;
}

cs::List<cs::Ref<Spellbook_SpellRow>> Spellbook::ClassSpells(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all, int32_t kind) {
    auto rows = cs::List<cs::Ref<Spellbook_SpellRow>>::New();
    int32_t bestGuild = 0;                                      // l008A, a procedure-level Dim
    for (int32_t s = 0; s < all.Count(); s++) {                 // 83
        auto sp = all[s];
        if (sp->Category != kind) continue;                     // 84
        int32_t points = 0;                                     // l0080
        float rate = 0;                                         // l007C
        int32_t bestSl = 0;                                     // l0088
        bool listed = false;                                    // l0086
        cs::Ref<Spellbook_SpellRow> row = nullptr;
        for (int32_t g = 0; g < GuildData::All.Count(); g++) {  // 89
            auto def = GuildData::All[g];
            auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == def->Name; });
            if (m == nullptr || m->Level <= 0) continue;        // 90
            if (kind < def->SpellMax.Length() && sp->Level <= def->SpellMax[kind]) {   // 91
                float r = kind < def->SpellRate.Length() ? def->SpellRate[kind] : 0;
                if (rate == 0) rate = r;                        // 92-94
                if (r <= rate) {                                // 95
                    rate = r;
                    double half = m->Level / 2.0 + 0.5;
                    float product = (float)(sp->Level * rate);  // Integer * Single is a Single
                    if (product <= half) {                      // 97
                        if (half > bestSl) {                    // 98-101
                            bestGuild = g;
                            bestSl = cs::CastDouble<int32_t>(cs::Math::Round(half, cs::MidpointRounding::ToEven));
                        }
                        listed = true;                          // 102-107
                        int32_t level = cs::CastDouble<int32_t>(cs::Math::Floor(product));   // 110
                        if (rate == 0) rate = 1;                // 113-115
                        row = [&] { auto _o = cs::New<Spellbook_SpellRow>(); _o->Spell = sp; _o->SpellIndex = s; _o->Guild = bestGuild; _o->Level = level; _o->Points = 0; return _o; }();  // 108-111
                        int32_t cost = Cost(sp, c);             // fn1485
                        if (cost <= points || points == 0) {    // 116-119
                            points = cost;
                            auto copy = cs::New<Spellbook_SpellRow>(*row);
                            copy->Points = points;
                            row = copy;
                        }
                    }
                }
            }
            if (!listed) rate = 0;                              // 122
        }
        if (row != nullptr) rows.Add(row);
    }
    return rows;
}

bool Spellbook::ShownBold(cs::Ref<Character> c, cs::Ref<Spell> s, cs::Func<cs::Ref<Item>(int16_t)> item) {
    return CastGate::Unfit(c, s, 0, true, item) == nullptr;
}

bool Spellbook::IsHealing(cs::Ref<Spell> s) {
    return s->Category == Healing && s->Damage2 > 0;
}

bool Spellbook::IsCharm(cs::Ref<Spell> s) {
    return s->Category == Charm;
}

bool Spellbook::IsCure(cs::Ref<Spell> s) {
    auto e = SpellEffect::Of(s);
    return e == SpellEffect::CurePoison || e == SpellEffect::CureDisease || e == SpellEffect::CureParalysis;
}

bool Spellbook::IsCureParalysis(cs::Ref<Spell> s) {
    return SpellEffect::Of(s) == SpellEffect::CureParalysis;
}

bool Spellbook::IsUtility(cs::Ref<Spell> s) {
    int32_t e = SpellEffect::Of(s);
    return SpellEffect::IsState(e) || SpellEffect::ResistKind(e) >= 0 || IsBinding(s) || OpensLockUpTo(s) > 0
        || SpellEffect::IsRaise(e) || (e == SpellEffect::Teleport || e == SpellEffect::Displacement)
        || (e == SpellEffect::SoulSearch || e == SpellEffect::RetrieveSoul)
        || (e == SpellEffect::Fate || e == SpellEffect::SetSanctuary || e == SpellEffect::Sanctuary
            || e == SpellEffect::Passwall || e == SpellEffect::DetectRock
            || e == SpellEffect::DepthPerception || e == SpellEffect::FindDirection);
}

bool Spellbook::IsBinding(cs::Ref<Spell> s) {
    int32_t e = SpellEffect::Of(s);
    return (e >= SpellEffect::BindFirst && e <= SpellEffect::BindLast) || e == SpellEffect::BindForLife;
}

int32_t Spellbook::OpensLockUpTo(cs::Ref<Spell> s) {
    return SpellEffect::OpensLockUpTo(SpellEffect::Of(s));
}

bool Spellbook::IsOffensive(cs::Ref<Spell> s) {
    return s->Damage2 > 0 && s->Category != Healing && s->Category != Resist && s->Category != Movement && s->Category != Sight && s->Category != Opening && s->Category != Control && s->Category != Charm;
}

bool Spellbook::IsGroup(cs::Ref<Spell> s) {
    return s->AffectGroup >= 6;
}

int32_t Spellbook::Cost(cs::Ref<Spell> s, cs::Ref<Character> c) {
    int32_t best = 0;      // l0480
    int32_t floor = -1;    // l0486, the smallest rated level of any guild accepted so far
    for (auto& m : c->Guilds) {
        auto def = GuildData::ByName(m->Guild);
        if (!def->HasSpellTable() || m->Level <= 0) continue;
        if (s->Category < 0 || s->Category >= def->SpellMax.Length()) continue;
        if (s->Level > def->SpellMax[s->Category]) continue;

        int32_t sl = Rules::SpellLevel(m->Level);
        // the spell's own Level and M556D, raw (2880, 2893): no floor of 1 on either
        int32_t rated = 0;
        int32_t cost = Rules::Fn1485Price(s->Level, s->U4, def->SpellRate[s->Category], sl, rated);

        if (cost > best && best != 0) continue;
        best = cost;
        if (rated < floor || floor == -1) floor = rated;
        if (best < floor) best = floor;
    }
    // no guild reaches the spell: l0480 stays nought (2901)
    return best;
}

cs::List<Tup_Index_Def_Level_2e78f> Spellbook::ReachingGuilds(cs::Ref<Character> c, cs::Ref<Spell> s) {
    cs::List<Tup_Index_Def_Level_2e78f> _result = cs::List<Tup_Index_Def_Level_2e78f>::New();
    for (int32_t g = 0; g < GuildData::All.Count(); g++) {
        auto def = GuildData::All[g];
        if (s->Category < 0 || s->Category >= def->SpellRate.Length()) continue;
        if (!(def->SpellRate[s->Category] > 0 && def->SpellMax[s->Category] >= s->Level)) continue;
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == def->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        _result.Add(Tup_Index_Def_Level_2e78f{g, def, cs::CastDouble<int32_t>(cs::Math::Floor(m->Level / 2.0 + 0.5))});
    }
    return _result;
}

int32_t Spellbook::TeachingGuild(cs::Ref<Character> c, cs::Ref<Spell> s) {
    int32_t best = 0, guild = 0;
    for (auto& t : ReachingGuilds(c, s)) {
        auto g = t.Index;
        auto sl = t.Level;
        if (sl > best) { best = sl; guild = g; }
    }
    return guild;
}

int32_t Spellbook::CasterSpellLevel(cs::Ref<Character> c, cs::Ref<Spell> s) {
    int32_t best = 0;
    for (auto& t : ReachingGuilds(c, s)) {
        auto g = t.Index;
        auto sl = t.Level;
        if (sl > best) { TrainingGuild = g; best = sl; }
    }
    return best;
}

int32_t Spellbook::SpellLevelFor(cs::Ref<Character> c, cs::Ref<Spell> s) {
    CasterSpellLevel(c, s);   // every cast asks casterSpellLevel, and that is what moves gv1D08
    int32_t best = 0;
    for (auto& m : c->Guilds) {
        if (m->Level <= 0) continue;
        auto def = GuildData::ByName(m->Guild);
        // a guild with no rate for the school reaches nothing (MAINMOD2.BAS:2856)
        bool reaches = s->Category >= 0 && s->Category < def->SpellRate.Length()
            && def->SpellRate[s->Category] > 0 && def->SpellMax[s->Category] >= s->Level;
        if (reaches) best = cs::Math::Max(best, Rules::SpellLevel(m->Level));
    }
    return cs::Math::Max(1, best);
}

int32_t Spellbook::FindByName(cs::List<cs::Ref<Spell>> all, cs::String typed, cs::Func<void(cs::String)> say) {
    int32_t found = 0, at = -1;
    for (int32_t i = 0; i < all.Count(); i++) {
        cs::String name = all[i]->Name;
        if ((int32_t)name.size() < (int32_t)typed.size()) continue;
        if (!cs::Str::StartsWithIgnoreCase(name, typed)) continue;
        if (cs::Str::EqualsIgnoreCase(name, typed)) { at = i; found = 1; break; }
        found++;
        at = i;
    }
    if (found == 0) { say("Spell not found!"); return -1; }
    if (found > 1) { say("Spell name is ambiguous!"); return -1; }
    return at;
}

cs::List<cs::Ref<Spell>> Spellbook::Known(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all) {
    auto known = cs::List<cs::Ref<Spell>>::New();
    for (auto& m : c->Guilds) {
        auto def = GuildData::ByName(m->Guild);
        for (auto& s : all) {
            // no Id == 0 guard: MDATA2's record 0 is a real spell (FRMCHAR3.BAS:83)
            if (known.Contains(s) || !(IsOffensive(s) || IsHealing(s) || IsCure(s) || IsCharm(s) || IsUtility(s))) continue;
            if (def->CanCast(s->Category, s->Level, m->Level)) known.Add(s);
        }
    }
    return known.OrderByThenBy([&](auto& s) { return s->Level; }, [&](auto& s) { return s->Category; }, false, false).ToList();
}

cs::List<cs::Ref<Spellbook_Castable>> Spellbook::Castables(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all, cs::Func<cs::Ref<Item>(int16_t)> item) {
    auto list = Known(c, all).Select([&](auto& s) {
        auto _o = cs::New<Spellbook_Castable>();
        _o->Spell = s; _o->Slot = nullptr; _o->Item = nullptr; _o->SpCost = Cost(s, c);
        return _o;
    }).ToList();
    for (auto& slot : c->Inventory()) {
        if (!slot->Identified() || slot->Charges <= 0) continue;
        auto it = item(slot->ItemId);
        if (it == nullptr) continue;
        // M56C4 is one-based, a record position (MAINMOD2.BAS:6112, 6127)
        if (it->SpellIndex <= 0 || it->SpellIndex - 1 >= all.Count()) continue;
        auto sp = all[it->SpellIndex - 1];
        // worn and own-alignment checks (MAINMOD2.BAS:6108-6119)
        if (ItemUse::MustBeEquippedToUse(it) && !slot->Equipped) continue;
        if (slot->Alignment != ItemAlignment::Unaligned
            && ItemAlignments::AsCharacter(slot->Alignment) != c->Alignment) continue;
        if (IsOffensive(sp) || IsHealing(sp) || IsCure(sp) || IsCharm(sp) || IsUtility(sp)) {
            auto _o = cs::New<Spellbook_Castable>();
            _o->Spell = sp; _o->Slot = slot; _o->Item = it; _o->SpCost = 0;
            list.Add(_o);
        }
    }
    return list;
}

bool Spellbook::IsConsumable(cs::Ref<Item> it) {
    return ItemUse::ConsumedWhenEmpty(it);
}

bool Spellbook::SpendCharge(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it) {
    slot->Charges--;
    if (slot->Charges <= 0 && IsConsumable(it)) { c->RemoveFromPack(slot); return true; }
    return false;
}

cs::Ref<Spell> Spellbook::BestHealing(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> all) {
    return Known(c, all).Where([&](auto& s) { return IsHealing(s) && Cost(s, c) <= c->CurSpellPoints; }).OrderByDescending([&](auto& s) { return s->Damage2; }).FirstOrDefault();
}

int32_t Spellbook::MaxSpellPoints(cs::Ref<Character> c) {
    return (c->Stats[static_cast<int32_t>(Stat::Int)] + c->Stats[static_cast<int32_t>(Stat::Wis)]) * 5;
}

}  // namespace mc
