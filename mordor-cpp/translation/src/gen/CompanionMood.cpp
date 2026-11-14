#include "core.h"

namespace mc {

cs::Ref<CompanionMood_Step> CompanionMood::Round(cs::Ref<Character> who, int32_t index, cs::Ref<Monster> def, cs::Ref<cs::Random> rng, bool isLeader) {
    // Ref > -1 And Hits > 0 (sub0C00, MONSMOD.BAS:617): an empty slot or a fallen companion is passed over
    cs::Ref<Companion> c = who->Companions->Index(index);
    if (c == nullptr || c->Hits <= 0) return cs::New<CompanionMood_Step>();

    // gc1D78: bound for life, and nothing below moves it
    if (c->BindingLevel >= Binding::ForLife) return cs::New<CompanionMood_Step>();

    cs::String drifted{};
    if (rng->NextDouble() * 200 < Drift(who, c, def)) {
        // towards zero from either side
        c->BindingLevel += c->BindingLevel > 0 ? -1 : 1;
        drifted = c->BindingLevel >= 0
            ? "\"" + cs::Str::TrimEnd(who->Name) + "\" - " + cs::ToStr(who->CompanionName(index)) + " has broken a level of binding!"
            : "\"" + cs::Str::TrimEnd(who->Name) + "\" - " + cs::ToStr(who->CompanionName(index)) + " is becoming restless!";
    }

    bool leaving = rng->NextDouble() * 200 < Binding::ChanceToLeave(
                       c->BindingLevel, def->GuildLevel, who->TopGuildLevel(),
                       who->Stats.Length() > static_cast<int32_t>(Stat::Cha) ? who->Stats[static_cast<int32_t>(Stat::Cha)] : 10,
                       c->Hits, c->MaxHits)
                   || c->BindingLevel == 0;

    return [&] {
        auto _o = cs::New<CompanionMood_Step>();
        _o->Drifted = drifted;
        _o->Leaving = leaving;
        _o->Left = leaving
            ? (isLeader ? cs::ToStr(who->CompanionName(index)) + " Left!"
                        : "\"" + cs::Str::TrimEnd(who->Name) + "\": " + cs::ToStr(who->CompanionName(index)) + " Left!")
            : cs::String();
        return _o;
    }();
}

double CompanionMood::Drift(cs::Ref<Character> who, cs::Ref<Companion> c, cs::Ref<Monster> def) {
    double drift;
    if (c->BindingLevel < 0) {
        drift = c->MaxHits <= 0 ? 0 : (double)(c->MaxHits - c->Hits) / c->MaxHits * 15;
        return cs::Math::Max(0.5, drift);
    }
    double strength = def->GuildLevel / cs::Math::Log(3) * 2;
    double hold = who->TopGuildLevel() / cs::Math::Log(12) * cs::Math::Abs(c->BindingLevel / 2.0);
    drift = strength - hold;
    return cs::Math::Max(1, drift);
}

}  // namespace mc
