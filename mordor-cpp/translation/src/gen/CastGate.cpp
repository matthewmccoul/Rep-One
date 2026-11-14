#include "core.h"

namespace mc {

SpellUse CastGate::UseOf(cs::Ref<Spell> s) {
    if (s->Effect == SpellEffect::Teleport || s->Effect == SpellEffect::CureParalysis) return SpellUse::Either;
    return s->Damage2 == 0 || s->Category == HealSchool ? SpellUse::NonCombat : SpellUse::Combat;
}

cs::Ref<Refusal> CastGate::Unfit(cs::Ref<Character> caster, cs::Ref<Spell> s, int32_t itemLevel, bool isYou, cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (itemLevel != 0) return nullptr;
    cs::String name = cs::Str::TrimEnd(caster->Name);
    cs::String q = "\"" + cs::Str::TrimEnd(s->Name) + "\"";

    // 437-443: a guild of yours with a rate for the school whose ceiling reaches the spell
    bool knows = false, levelOk = false;
    for (auto& m : caster->Guilds) {
        if (m->Level <= 0) continue;
        auto def = GuildData::ByName(m->Guild);
        if (s->Category < 0 || s->Category >= def->SpellRate.Length()) continue;
        float rate = def->SpellRate[s->Category];
        if (rate > 0 && def->SpellMax[s->Category] >= s->Level) knows = true;
        // 457-469: l011E is an Integer, so Level * rate rounds half to even; l0120 = Int(gl / 2 + .5)
        if (rate > 0 && cs::CastDouble<int32_t>(cs::Math::Round(s->Level * (double)rate, cs::MidpointRounding::ToEven))
                        <= cs::CastDouble<int32_t>(cs::Math::Floor(m->Level / 2.0 + 0.5)))
            levelOk = true;
    }
    if (!knows)
        return cs::New<Refusal>(RefusalKind::NotKnown, isYou ? "You do not know " + q + "." : "\"" + name + "\" does not know " + q + ".");   // 449-451
    if (!levelOk)
        return cs::New<Refusal>(RefusalKind::SpellLevel,
                                isYou ? "Spell Level too low to cast " + q + "!"                          // 477
                                      : "\"" + name + "\", your Spell Level is too low to cast " + q + "!");  // 475

    // 483-496: Stats + StatMods against M5B4F, the six in order
    for (int32_t i = 0; i < 6 && i < s->Required.Length(); i++)
        if (caster->StatWithMods(i, item) < s->Required[i])
            return cs::New<Refusal>(RefusalKind::Stat, isYou ? "Your " + RaceData::StatNamesLong[i] + " is too low to cast " + q + "!"
                                                             : name + "s " + RaceData::StatNamesLong[i] + " is too low to cast " + q + "!");
    return nullptr;
}

cs::Ref<Refusal> CastGate::Refuse(cs::Ref<Character> caster, cs::Ref<Spell> s, int32_t cost, bool isYou,
                                  bool monstersPresent, bool engaged, bool antiMagic) {
    cs::String name = cs::Str::TrimEnd(caster->Name);
    cs::String q = "\"" + cs::Str::TrimEnd(s->Name) + "\"";

    if (cost > caster->CurSpellPoints)
        return cs::New<Refusal>(RefusalKind::SpellPoints, isYou ? "Not enough spell points to cast " + q + "!"
                                                                : name + " doesn't have enough spell points to cast " + q + "!");

    auto use = UseOf(s);

    if (monstersPresent && engaged && use == SpellUse::NonCombat)
        return cs::New<Refusal>(RefusalKind::NonCombat, isYou ? q + " is a Non-Combat Spell!"
                                                              : q + " is a Non-Combat Spell, " + name + ".");

    // anti-magic or extinguish square: the original says the same words for either (MAINMOD2.BAS:520-525)
    if (antiMagic) return cs::New<Refusal>(RefusalKind::AntiMagic, AntiMagic);

    if (use == SpellUse::Combat || use == SpellUse::Either) {
        if (!monstersPresent) {
            if (use == SpellUse::Combat) return cs::New<Refusal>(RefusalKind::CombatOnly, q + " is a Combat Spell!");
        } else if (!engaged && s->Level > PeacedSpellLevel) {
            return cs::New<Refusal>(RefusalKind::TooLongPeaced, q + " Takes too long to throw when peaced, " + name + ".");
        }
    }
    return nullptr;
}

bool Refusal::ClearsItemSlot() {
    return Kind == RefusalKind::NonCombat || Kind == RefusalKind::AntiMagic
        || Kind == RefusalKind::CombatOnly || Kind == RefusalKind::TooLongPeaced;
}

}  // namespace mc
