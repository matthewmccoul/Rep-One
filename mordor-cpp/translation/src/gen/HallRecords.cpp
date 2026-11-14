#include "core.h"

namespace mc {

int64_t HallRecords_Record::ValueWhole() { return Value.Whole(); }
void HallRecords_Record::set_ValueWhole(int64_t value) { Value = VbCurrency::Saturate(value); }
cs::Nullable<cs::Decimal> HallRecords_Record::ValueExact() { return Value.ExactForSave(); }
void HallRecords_Record::set_ValueExact(cs::Nullable<cs::Decimal> value) { _exact = value; }
void HallRecords_Record::OnDeserialized() {
    if (_exact.has_value()) Value = VbCurrency::FromSave(Value.Whole(), _exact);
    _exact = {};
}

cs::String HallRecords::LogName(HallRecords_Kind k) {
    switch (k) {
        case HallRecords_Kind::Strength: return "Strongest Character";
        case HallRecords_Kind::Intelligence: return "most Intelligent Character";
        case HallRecords_Kind::Wisdom: return "Wisest Character";
        case HallRecords_Kind::Constitution: return "Healthiest Character";
        case HallRecords_Kind::Charisma: return "most Attractive Character";
        case HallRecords_Kind::Dexterity: return "Quickest Character";
        case HallRecords_Kind::DeadliestCreature: return "most powerful creature defeated";
        case HallRecords_Kind::MostExperience: return "most experience";
        case HallRecords_Kind::Wealthiest: return "wealthiest character";
        case HallRecords_Kind::Fighting: return "highest fighting ability";
        case HallRecords_Kind::Magic: return "highest spell knowledge & power level";
        default: return "highest thieving ability";
    }
}

cs::String HallRecords::Heading(HallRecords_Kind k) {
    switch (k) {
        case HallRecords_Kind::Strength: return "Strongest";
        case HallRecords_Kind::Intelligence: return "Smartest";
        case HallRecords_Kind::Wisdom: return "Wisest";
        case HallRecords_Kind::Constitution: return "Healthiest";
        case HallRecords_Kind::Charisma: return "Most Attractive";
        case HallRecords_Kind::Dexterity: return "Quickest";
        case HallRecords_Kind::DeadliestCreature: return "Deadliest Creature Defeated";
        case HallRecords_Kind::MostExperience: return "Most Experienced Explorer";
        case HallRecords_Kind::Wealthiest: return "Wealthiest Explorer";
        case HallRecords_Kind::Fighting: return "Master of Fighting";
        case HallRecords_Kind::Magic: return "Master of Magic";
        default: return "Master of Thieving";
    }
}

cs::Ref<HallRecords_Record> HallRecords::For(HallRecords_Kind k) {
    cs::Ref<HallRecords_Record> r{};
    return Records.TryGetValue(static_cast<int32_t>(k), r) ? r : cs::New<HallRecords_Record>();
}

cs::String HallRecords::Describe(HallRecords_Kind k, cs::Func<cs::String(VbCurrency)> valueText) {
    auto r = For(k);
    cs::String who = r->Day != 0 ? "Set on " + GuildLog::FromDaySerial(r->Day).ToString("d") + " by " : "Set by Original RecordHolder ";
    // RTrim(M73FE), sub32F6 (FRMHALL.BAS:130)
    return who + "\"" + cs::Str::TrimEnd(r->Holder) + "\"\n" + valueText(r->Value);
}

cs::String HallRecords::Check(HallRecords_Kind k, cs::Ref<Character> c, VbCurrency value, cs::Func<int32_t(int32_t)> monsterPower, bool alone) {
    auto r = For(k);

    if (k == HallRecords_Kind::DeadliestCreature) {
        // the value is a monster id; compared by power. The original zeroes the stored value first
        // so the plain comparison below always passes.
        int32_t power = monsterPower ? monsterPower(cs::Wrap<int32_t>(value.RoundToWhole())) : 0;
        if (power <= (monsterPower ? monsterPower(cs::Wrap<int32_t>(r->Value.RoundToWhole())) : 0)) return "";
        r = [&] { auto _o = cs::New<HallRecords_Record>(); _o->Value = VbCurrency::Saturate(-1); _o->Holder = r->Holder; _o->Day = r->Day; return _o; }();
    }

    if (value <= r->Value) return "";

    bool alreadyTheirs = cs::Str::TrimEnd(r->Holder) == cs::Str::TrimEnd(c->Name);
    Records[static_cast<int32_t>(k)] = [&] { auto _o = cs::New<HallRecords_Record>(); _o->Value = value; _o->Holder = c->Name; _o->Day = GuildLog::Today(); return _o; }();

    // improving your own record is silent, and so is the creature record (original sets one flag for both)
    if (alreadyTheirs || k == HallRecords_Kind::DeadliestCreature) return "";
    // MODULE11.BAS:505-509: "You have just set..." for a lone adventurer, the name otherwise
    return alone ? "You have just set the record for " + LogName(k) + "!"
                 : "\"" + c->Name + "\" just set the record for " + LogName(k) + "!";
}

cs::String HallRecords::CheckAndLog(HallRecords_Kind k, cs::Ref<Character> c, VbCurrency value, cs::String savesDir, cs::Func<int32_t(int32_t)> monsterPower, bool alone) {
    cs::String said = Check(k, c, value, monsterPower, alone);
    if (said.empty()) return "";
    GuildLog::Append(savesDir, GuildData::IndexOf(c->Guild()->Guild),
                     GuildLog_Wrote::SetRecord(c->Name, LogName(k), CharStatFile::FormatNumber(For(k)->Value)));
    return said;
}

int32_t HallRecords::MonsterPower(cs::Ref<Monster> m) {
    // MODULE11.BAS:546 assigns Att*2 + STR*10 + Hits/10 (float division) to an Integer, so
    // that sum is rounded before the rest is added in integer arithmetic
    int32_t first = cs::CastDouble<int32_t>(cs::Math::Round(m->Att * 2 + m->Stats.ElementAtOrDefault(0) * 10 + m->Hits / 10.0, cs::MidpointRounding::ToEven));
    return first
        + m->Stats.ElementAtOrDefault(5) * 2
        + m->GuildLevel
        + 75 * CountBits(m->SpecialAttackFlags, 14)
        + 100 * CountBits(m->SpellFlags, 18);
}

int32_t HallRecords::CountBits(int64_t mask, int32_t width) {
    int32_t n = 0;
    for (int32_t i = 0; i < width; i++) if ((mask & (1LL << i)) != 0) n++;
    return n;
}

int32_t HallRecords::FightingSkill(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    // l012E is an Integer, so every += rounds (banker's), and so does the Integer return
    int32_t total = Rules::AbilityRatingExact(c, 4, gear);
    for (auto& a : cs::List<int32_t>::Of({2, 1, 3}))
        total = cs::CastDouble<int32_t>(cs::Math::Round(total + Rules::AbilityRatingExact(c, a, gear) * 0.2, cs::MidpointRounding::ToEven));
    return cs::CastDouble<int32_t>(cs::Math::Round(total / (100 + 100 * 0.2 + 100 * 0.2 + 100 * 0.2) * 100, cs::MidpointRounding::ToEven));
}

int32_t HallRecords::ThievingSkill(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item) {
    int32_t v = Rules::AbilityRatingExact(c, 0, item) * c->EffectiveStat(static_cast<int32_t>(Stat::Dex), item);
    if (v < 1) v = 1;
    return cs::CastDouble<int32_t>(cs::Math::Round(v / (100.0 * 35) * 100, cs::MidpointRounding::ToEven));
}

int32_t HallRecords::MagicSkill(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> spells) {
    auto all = spells.ToList();   // every record, id 0 included - MODULE11.BAS:568 loops 0 To gv1BF8-1
    // both accumulators are Integers in the original, rounding at every step
    int32_t have = 0, possible = 0;
    for (auto& s : all) possible += s->Level * 10;
    possible += 255 * all.Count();
    if (possible <= 0) return 0;

    for (auto& s : all)
        for (auto& g0 : GuildData::All) {   // fn122A-style table order for the tie
            auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g0->Name; });
            if (m == nullptr) continue;
            if (m->Level <= 0) continue;
            auto g = GuildData::ByName(m->Guild);
            double factor = s->Category >= 0 && s->Category < g->SpellRate.Length() ? (double)g->SpellRate[s->Category] : 0;
            if (factor <= 0) continue;
            if (s->Level * factor > m->Level / 2.0 + 0.5) continue;
            have += s->Level * 10;
            have = cs::CastDouble<int32_t>(cs::Math::Round(have + (m->Level / 2.0 + 0.5), cs::MidpointRounding::ToEven));
            break;   // the first guild that can is the one that counts
        }
    return cs::CastDouble<int32_t>(cs::Math::Round(have / (double)possible * 100, cs::MidpointRounding::ToEven));
}

int64_t HallRecords::TotalExperience(cs::Ref<Character> c) {
    return (int64_t)c->Guilds.Sum([&](auto& g) { return (int64_t)g->Experience; });
}

cs::String HallRecords::PathFor(cs::String savesDir) { return cs::Path::Combine(savesDir, "records.json"); }

cs::Ref<HallRecords> HallRecords::Fresh() {
    auto h = cs::New<HallRecords>();
    auto Set = [&](HallRecords_Kind k, int64_t value, cs::String holder) {
        h->Records[static_cast<int32_t>(k)] = [&] { auto _o = cs::New<HallRecords_Record>(); _o->Value = VbCurrency::Saturate(value); _o->Holder = holder; _o->Day = 0; return _o; }();
    };

    Set(HallRecords_Kind::Strength, 14, "Crashland");
    Set(HallRecords_Kind::Intelligence, 15, "Mager");
    Set(HallRecords_Kind::Wisdom, 18, "Theshal");
    Set(HallRecords_Kind::Constitution, 14, "Orgal");
    Set(HallRecords_Kind::Charisma, 15, "Alaya");
    Set(HallRecords_Kind::Dexterity, 17, "Nimblefingers");
    Set(HallRecords_Kind::DeadliestCreature, 1, "Crashland");
    Set(HallRecords_Kind::MostExperience, 1322451, "Crashland");
    Set(HallRecords_Kind::Wealthiest, 5466453, "Nimblefingers");
    Set(HallRecords_Kind::Fighting, 17, "Morgul");
    Set(HallRecords_Kind::Magic, 6, "Ge'nal");
    Set(HallRecords_Kind::Thieving, 15, "Nimblefingers");
    return h;
}

cs::Ref<HallRecords> HallRecords::Load(cs::String savesDir) {
    try {
        cs::String p = PathFor(savesDir);
        if (cs::File::Exists(p)) {
            auto r = cs::Json::Deserialize<cs::Ref<HallRecords>>(cs::File::ReadAllText(p));
            return r ? r : cs::New<HallRecords>();
        }
    } catch (...) {
    }
    // no file yet means a new game, and a new game's Hall is not empty
    return Fresh();
}

void HallRecords::Save(cs::String savesDir) {
    try {
        cs::Directory::CreateDirectory(savesDir);
        cs::File::WriteAllText(PathFor(savesDir), cs::Json::Serialize(*this));
    } catch (...) {
    }
}

}  // namespace mc
